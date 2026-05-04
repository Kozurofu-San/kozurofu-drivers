#pragma once

#include "interface/Network.h"

#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include <functional>
#include <optional>
#include <atomic>
#include <cstring>
#include <cstdio>

namespace driver
{

class WifiDriver : public INetwork
{
public:

    // (Logging tag used with ESP_LOGI) kept as literal to avoid ODR issues



    WifiDriver()
    {

    }

    /**
     * @brief Initialize WiFi in Station mode
     * @return true if initialization was successful
     */
    bool init() override
    {
        if (_initialized) {
            return true;
        }

        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            if (nvs_flash_erase() != ESP_OK) {
                return false;
            }
            ret = nvs_flash_init();
        }
        if (ret != ESP_OK) {
            return false;
        }

        ret = esp_netif_init();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            return false;
        }

        ret = esp_event_loop_create_default();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            return false;
        }

        _staNetif = esp_netif_create_default_wifi_sta();
        if (_staNetif == nullptr) {
            return false;
        }

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret != ESP_OK) {
            return false;
        }

        ret = esp_wifi_set_mode(WIFI_MODE_STA);
        if (ret != ESP_OK) {
            return false;
        }

        // Регистрируем обработчик событий
        ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                  &WifiDriver::eventHandler, this, &_wifiEventHandler);
        if (ret != ESP_OK) {
            return false;
        }

        ret = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                  &WifiDriver::eventHandler, this, &_ipEventHandler);
        if (ret != ESP_OK) {
            return false;
        }

        _initialized = true;
        return true;
    }

    /**
     * @brief Start WiFi interface
     * @return true if started successfully
     */
    bool start() override
    {
        if (!_initialized) {
            if (!init()) {
                return false;
            }
        }

        if (_started) {
            return true;
        }

        esp_err_t ret = esp_wifi_start();
        if (ret != ESP_OK) {
            return false;
        }

        _started = true;
        return true;
    }

    /**
     * @brief Stop WiFi interface
     * @return true if stopped successfully
     */
    bool stop() override
    {
        if (!_started) {
            return true;
        }

        esp_err_t ret = esp_wifi_stop();
        if (ret != ESP_OK) {
            return false;
        }

        _started = false;
        _connected = false;
        return true;
    }

    bool scan(std::vector<ScanResult>& results, 
        size_t max_results = 20, 
        uint32_t timeout_ms = 10000) override
    {
        if (!_started) {
            if (!start()) {
                return false;
            }
        }

        results.clear();

        wifi_scan_config_t scan_config = {};
        scan_config.show_hidden = true;

        esp_err_t ret = esp_wifi_scan_start(&scan_config, true); // blocking
        if (ret != ESP_OK) {
            return false;
        }

        uint16_t ap_count = 0;
        ret = esp_wifi_scan_get_ap_num(&ap_count);
        if (ret != ESP_OK || ap_count == 0) {
            return true; // пустой результат — нормально
        }

        if (ap_count > max_results) {
            ap_count = static_cast<uint16_t>(max_results);
        }

        std::vector<wifi_ap_record_t> ap_records(ap_count);
        ret = esp_wifi_scan_get_ap_records(&ap_count, ap_records.data());
        if (ret != ESP_OK) {
            return false;
        }

        results.reserve(ap_count);
        for (uint16_t i = 0; i < ap_count; ++i) {
            const auto& rec = ap_records[i];
            ScanResult res;
            res.ssid = std::string(reinterpret_cast<const char*>(rec.ssid));
            char bssid[18] {};
            std::snprintf(bssid, sizeof(bssid), "%02X:%02X:%02X:%02X:%02X:%02X",
                          rec.bssid[0], rec.bssid[1], rec.bssid[2],
                          rec.bssid[3], rec.bssid[4], rec.bssid[5]);
            res.bssid = bssid;
            res.rssi = rec.rssi;
            res.channel = rec.primary;
            res.isHidden = rec.ssid[0] == '\0';
            // authmode можно расширить при необходимости
            switch (rec.authmode) {
                case WIFI_AUTH_OPEN:         res.authMode = AuthMode::Open;         break;
                case WIFI_AUTH_WEP:          res.authMode = AuthMode::Wep;          break;
                case WIFI_AUTH_WPA_PSK:      res.authMode = AuthMode::WpaPsk;       break;
                case WIFI_AUTH_WPA2_PSK:     res.authMode = AuthMode::Wpa2Psk;      break;
                case WIFI_AUTH_WPA_WPA2_PSK: res.authMode = AuthMode::WpaWpa2Psk;   break;
                default:                     res.authMode = AuthMode::Unknown;      break;
            }
            results.push_back(std::move(res));
        }

        return true;
    }

    /**
     * @brief Connect to WiFi network
     * @param ssid WiFi network name
     * @param password WiFi password (can be empty for open networks)
     * @return true if connection command was accepted
     */
    bool connect(std::string_view ssid = {}, std::string_view password = {}) override
    {
        if (ssid.empty()) {
            return false;
        }

        if (!_started) {
            if (!start()) {
                return false;
            }
        }

        wifi_config_t wifi_config = {};
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid.data(), sizeof(wifi_config.sta.ssid) - 1);
        if (!password.empty()) {
            std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), password.data(), sizeof(wifi_config.sta.password) - 1);
        }

        esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        if (ret != ESP_OK) {
            return false;
        }

        _connected = false;
        ret = esp_wifi_connect();
        return (ret == ESP_OK);
    }

    /**
     * @brief Disconnect from current WiFi network
     * @return true if disconnect command was accepted
     */
    bool disconnect() override
    {
        esp_err_t ret = esp_wifi_disconnect();
        return (ret == ESP_OK);
    }

    /**
     * @brief Check if connected to AP (has IP address)
     */
    bool isConnected() const override
    {
        return _connected;
    }

    /**
     * @brief Check if WiFi interface has been started
     */
    bool isStarted() const override
    {
        return _started;
    }

    /**
     * @brief Set callback for WiFi events
     * @param cb Function pointer that receives event ID (uint32_t)
     */
    void callback(void (*cb)(uint32_t)) override
    {
        _userCallback = cb;
    }

    /**
     * @brief Get current IP address as string
     */
    std::optional<std::string> getIp() const override
    {
        if (!_staNetif || !_connected) {
            return std::nullopt;
        }

        esp_netif_ip_info_t ip_info;
        esp_err_t ret = esp_netif_get_ip_info(_staNetif, &ip_info);
        if (ret != ESP_OK || ip_info.ip.addr == 0) {
            return std::nullopt;
        }

        char ip_str[16];
        esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
        return std::string(ip_str);
    }

    /**
     * @brief Get MAC address as string (XX:XX:XX:XX:XX:XX)
     */
    std::optional<std::string> getMac() const override
    {
        uint8_t mac[6];
        esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, mac);
        if (ret != ESP_OK) {
            return std::nullopt;
        }

        char mac_str[18];
        std::snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return std::string(mac_str);
    }

private:
    static void eventHandler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
    {
        auto* self = static_cast<WifiDriver*>(arg);
        if (self == nullptr) return;

        if (event_base == WIFI_EVENT) {
            switch (event_id) {
                case WIFI_EVENT_STA_START:
                    break;

                case WIFI_EVENT_STA_CONNECTED:
                    // IP ещё не получен
                    break;

                case WIFI_EVENT_STA_DISCONNECTED:
                    self->_connected = false;
                    // Попробуем залогировать причину отключения, если она есть
                    if (event_data != nullptr) {
                        auto* disconn = static_cast<wifi_event_sta_disconnected_t*>(event_data);
                        ESP_LOGI("WifiDriver", "WiFi STA_DISCONNECTED, reason=%d", disconn->reason);
                    }
                    if (self->_userCallback) {
                        self->_userCallback(WIFI_EVENT_STA_DISCONNECTED);
                    }
                    break;

                default:
                    break;
            }
        }
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            self->_connected = true;
            if (self->_userCallback) {
                self->_userCallback(IP_EVENT_STA_GOT_IP);
            }
        }

        // Вызываем пользовательский callback для всех событий
        if (self->_userCallback) {
            self->_userCallback(static_cast<uint32_t>(event_id));
        }
    }

private:
    esp_netif_t* _staNetif = nullptr;
    esp_event_handler_instance_t _wifiEventHandler = nullptr;
    esp_event_handler_instance_t _ipEventHandler = nullptr;

    void (*_userCallback)(uint32_t) = nullptr;

    std::atomic<bool> _initialized{false};
    std::atomic<bool> _started{false};
    std::atomic<bool> _connected{false};
};

}
