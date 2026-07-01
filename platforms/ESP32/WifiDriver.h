#pragma once

#include "interface/Network.h"

#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "ping/ping_sock.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <functional>
#include <optional>
#include <atomic>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <arpa/inet.h>

namespace driver
{

class WifiDriver : public INetwork
{
public:

    WifiDriver()
    {

    }

    /**
     * @brief Initialize WiFi in Station mode
     * @return true if initialization was successful
     */
    bool init()
    {
        if (_isInit) { return true; }

        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            if (nvs_flash_erase() != ESP_OK) { return false; }
            ret = nvs_flash_init();
        }
        if (ret != ESP_OK) { return false; }

        ret = esp_netif_init();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { return false; }

        ret = esp_event_loop_create_default();
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { return false; }

        _staNetif = esp_netif_create_default_wifi_sta();
        if (_staNetif == nullptr) { return false; }

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ret = esp_wifi_init(&cfg);
        if (ret != ESP_OK) { return false; }

        ret = esp_wifi_set_mode(WIFI_MODE_STA);
        if (ret != ESP_OK) { return false; }

        // Регистрируем обработчик событий
        ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                  &WifiDriver::eventHandler, this, &_wifiEventHandler);
        if (ret != ESP_OK) { return false; }

        ret = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                  &WifiDriver::eventHandler, this, &_ipEventHandler);
        if (ret != ESP_OK) { return false; }

        // Time server
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_init();
        setenv("TZ", "UTC-3", 1);
        tzset();

        _isInit = true;
        return true;
    }

    /**
     * @brief Start WiFi interface
     * @return true if started successfully
     */
    bool start() override
    {
        if (!_isInit) { if (!init()) { return false; } }

        if (_isStarted) { return true; }

        esp_err_t ret = esp_wifi_start();
        if (ret != ESP_OK) { return false; }

        esp_wifi_set_ps(WIFI_PS_NONE);

        _isStarted = true;
        return true;
    }

    /**
     * @brief Stop WiFi interface
     * @return true if stopped successfully
     */
    bool stop() override
    {
        if (!_isStarted) { return true; }

        esp_err_t ret = esp_wifi_stop();
        if (ret != ESP_OK) { return false; }

        _isStarted = false;
        _isConnected = false;
        return true;
    }

    bool scan(std::vector<ScanResult>& results, 
        size_t max_results = 20, 
        uint32_t timeout_ms = 10000) override
    {
        if (!_isStarted)
        {
            if (!start()) { return false; }
        }

        results.clear();

        wifi_scan_config_t scan_config = {};
        scan_config.show_hidden = true;

        esp_err_t ret = esp_wifi_scan_start(&scan_config, true); // blocking
        if (ret != ESP_OK) { return false; }

        uint16_t ap_count = 0;
        ret = esp_wifi_scan_get_ap_num(&ap_count);
        if (ret != ESP_OK || ap_count == 0) { return true; }

        if (ap_count > max_results) { ap_count = static_cast<uint16_t>(max_results); }

        std::vector<wifi_ap_record_t> ap_records(ap_count);
        ret = esp_wifi_scan_get_ap_records(&ap_count, ap_records.data());
        if (ret != ESP_OK) { return false; }

        results.reserve(ap_count);
        for (uint16_t i = 0; i < ap_count; ++i)
        {
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
            switch (rec.authmode)
            {
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
        if (ssid.empty()) { return false; }

        if (!_isStarted)
        {
            if (!start()) { return false; }
        }

        wifi_config_t wifi_config = {};
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid.data(), sizeof(wifi_config.sta.ssid) - 1);
        if (!password.empty())
        {
            std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), password.data(), sizeof(wifi_config.sta.password) - 1);
        }
        wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
        wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;
        wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
        wifi_config.sta.sae_pk_mode = WPA3_SAE_PK_MODE_DISABLED;
        wifi_config.sta.disable_wpa3_compatible_mode = true;
        wifi_config.sta.failure_retry_cnt = 3;

        esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        if (ret != ESP_OK) { return false; }

        _isConnected = false;

        constexpr int max_retries = 3;
        constexpr int max_wait_ms = 15000;
        constexpr int poll_ms = 200;

        for (int attempt = 0; attempt < max_retries; ++attempt)
        {
            ret = esp_wifi_connect();
            if (ret != ESP_OK && ret != ESP_ERR_WIFI_CONN)
            {
                ESP_LOGW("WifiDriver", "connect attempt %d failed to start: %s", attempt + 1, esp_err_to_name(ret));
                return false;
            }

            int waited = 0;
            while (waited < max_wait_ms)
            {
                if (auto ip = getIp())
                {
                    ESP_LOGI("WifiDriver", "connected to %.*s, IP %s", static_cast<int>(ssid.size()), ssid.data(), ip->c_str());
                    return true;
                }

                vTaskDelay(poll_ms / portTICK_PERIOD_MS);
                waited += poll_ms;
            }

            ESP_LOGW("WifiDriver", "connect attempt %d timed out", attempt + 1);
            if (attempt + 1 < max_retries)
            {
                esp_wifi_disconnect();
                _isConnected = false;
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }

        return false;
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
        return _isConnected;
    }

    /**
     * @brief Check if WiFi interface has been started
     */
    bool isStarted() const override
    {
        return _isStarted;
    }

    /**
     * @brief Set callback for WiFi events
     * @param cb Function pointer that receives event ID (uint32_t)
     */
    void callback(void (*cb)(uint32_t)) override
    {
        _userCallback = cb;
    }

    bool isInit() const override
    {
        return _isInit;
    }

    /**
     * @brief Get current IP address as string
     */
    std::optional<std::string> getIp() const override
    {
        if (!_staNetif || !_isConnected) { return std::nullopt; }

        esp_netif_ip_info_t ip_info;
        esp_err_t ret = esp_netif_get_ip_info(_staNetif, &ip_info);
        if (ret != ESP_OK || ip_info.ip.addr == 0) {  return std::nullopt; }

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
        if (ret != ESP_OK) { return std::nullopt; }

        char mac_str[18];
        std::snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return std::string(mac_str);
    }
    
    std::time_t time() const override
    {
        std::time_t now = 0;
        std::tm localTime;
        std::time(&now);
        localtime_r(&now, &localTime);
        return std::mktime(&localTime);
    }
    
    bool ping(const std::string& host, uint32_t count = 4, uint32_t timeout_ms = 1000) const
    {
        if (!_isConnected) { ESP_LOGD("WifiDriver", "Wifi isn't connected"); return false; }

        esp_err_t ret;

        ip_addr_t target_addr;
        struct addrinfo hint;
        struct addrinfo *res = nullptr;
        memset(&hint, 0, sizeof(hint));
        memset(&target_addr, 0, sizeof(target_addr));
        hint.ai_family = AF_INET;
        ret = getaddrinfo(host.c_str(), nullptr, &hint, &res);
        if (ret != 0 || res == nullptr)
        {
            ESP_LOGE("WifiDriver", "DNS lookup failed for %s: %d", host.c_str(), ret);
            return false;
        }

        struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
        inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
        freeaddrinfo(res);
        ESP_LOGI("WifiDriver", "Ping target %s %s", host.c_str(), inet_ntoa(target_addr.u_addr.ip4));

        esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
        ping_config.target_addr = target_addr;          // target IP address
        ping_config.count = count;
        ping_config.timeout_ms = timeout_ms;

        /* set callback functions */
        esp_ping_callbacks_t cbs {};
        cbs.on_ping_success = onPingSuccess;
        cbs.on_ping_timeout = onPingTimeout;
        cbs.on_ping_end = onPingEnd;
        cbs.cb_args = nullptr;

        esp_ping_handle_t ping = nullptr;
        ret = esp_ping_new_session(&ping_config, &cbs, &ping);
        if (ret != ESP_OK)
        {
            ESP_LOGE("WifiDriver", "Ping session isn't created: %s", esp_err_to_name(ret));
            return false;
        }

        ret = esp_ping_start(ping);
        if (ret != ESP_OK)
        {
            ESP_LOGE("WifiDriver", "Ping isn't started: %s", esp_err_to_name(ret));
            esp_ping_delete_session(ping);
            return false;
        }

        return ret == ESP_OK;
    }

private:

    static void eventHandler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
    {
        auto* self = static_cast<WifiDriver*>(arg);
        if (self == nullptr) return;

        if (event_base == WIFI_EVENT)
        {
            switch (event_id) {
                case WIFI_EVENT_STA_START:
                    break;

                case WIFI_EVENT_STA_CONNECTED:
                    // IP ещё не получен
                    break;

                case WIFI_EVENT_STA_DISCONNECTED:
                    self->_isConnected = false;
                    // Попробуем залогировать причину отключения, если она есть
                    if (event_data != nullptr)
                    {
                        auto* disconn = static_cast<wifi_event_sta_disconnected_t*>(event_data);
                        ESP_LOGD("WifiDriver", "WiFi STA_DISCONNECTED, reason=%d", disconn->reason);
                    }
                    if (self->_userCallback)
                    {
                        self->_userCallback(WIFI_EVENT_STA_DISCONNECTED);
                    }
                    break;

                default:
                    break;
            }
        }
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        {
            self->_isConnected = true;
            if (self->_userCallback)
            {
                self->_userCallback(IP_EVENT_STA_GOT_IP);
            }
        }

        // Вызываем пользовательский callback для всех событий
        if (self->_userCallback)
        {
            self->_userCallback(static_cast<uint32_t>(event_id));
        }
    }
    
    static void onPingSuccess(esp_ping_handle_t hdl, void* args)
    {
        uint8_t ttl;
        uint16_t seqno;
        uint32_t elapsed_time, recv_len;
        ip_addr_t target_addr;
        esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
        esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
        esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
        esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
        esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
        ESP_LOGD("WifiDriver", "%u bytes from %s icmp_seq=%u ttl=%u time=%u ms\n",
           recv_len, inet_ntoa(target_addr.u_addr.ip4), seqno, ttl, elapsed_time);
    }

    static void onPingTimeout(esp_ping_handle_t hdl, void* args)
    {
        uint16_t seqno;
        ip_addr_t target_addr;
        esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
        esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
        ESP_LOGD("WifiDriver", "From %s icmp_seq=%u timeout\n", inet_ntoa(target_addr.u_addr.ip4), seqno);
    }

    static void onPingEnd(esp_ping_handle_t hdl, void* args)
    {
        uint32_t transmitted;
        uint32_t received;
        uint32_t total_time_ms;
        ip_addr_t target_addr;

        esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
        esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
        esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
        esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
        ESP_LOGD("WifiDriver", "%u packets transmitted, %u received, time %ums\n", transmitted, received, total_time_ms);

        esp_ping_delete_session(hdl);
    }

    esp_netif_t* _staNetif = nullptr;
    esp_event_handler_instance_t _wifiEventHandler = nullptr;
    esp_event_handler_instance_t _ipEventHandler = nullptr;

    void (*_userCallback)(uint32_t) = nullptr;

    std::atomic<bool> _isInit{false};
    std::atomic<bool> _isStarted{false};
    std::atomic<bool> _isConnected{false};
};

}
