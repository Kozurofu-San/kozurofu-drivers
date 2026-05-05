#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <vector>
#include <ctime>

namespace driver
{

class INetwork
{
    public:
    
    /**
      * @brief Wi-Fi authmode type
      * Strength of authmodes
      * Personal Networks   : OPEN < WEP < WPA_PSK < OWE < WPA2_PSK = WPA_WPA2_PSK < WAPI_PSK < WPA3_PSK = WPA2_WPA3_PSK = DPP
      * Enterprise Networks : WIFI_AUTH_WPA_ENTERPRISE < WIFI_AUTH_WPA2_ENTERPRISE < WIFI_AUTH_WPA3_ENTERPRISE = WIFI_AUTH_WPA2_WPA3_ENTERPRISE < WIFI_AUTH_WPA3_ENT_192
      */
    enum class AuthMode: uint8_t
    {
        Open = 0,         /**< Open */
        Wep,              /**< WEP */
        WpaPsk,           /**< WPA_PSK */
        Wpa2Psk,          /**< WPA2_PSK */
        WpaWpa2Psk,       /**< WPA_WPA2_PSK */
        Wpa3Psk,          /**< WPA3_PSK */
        Wpa2Wpa3Psk,      /**< WPA2_WPA3_PSK */
        Unknown
    };
    
    struct ScanResult {
        std::string ssid;
        std::string bssid;           // MAC address of AP
        int8_t      rssi;            // Signal strength in dBm
        uint8_t     channel;
        AuthMode    authMode;       // WPA2, WPA3, Open и т.д.
        bool        isHidden;
    };

    virtual ~INetwork() = default;

    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool scan(std::vector<ScanResult>& results, 
        size_t max_results = 20, 
        uint32_t timeout_ms = 10000) = 0;
    virtual bool connect(std::string_view ssid = {}, std::string_view password = {}) = 0;
    virtual bool disconnect() = 0;

    virtual bool isConnected() const = 0;
    virtual bool isStarted() const = 0;

    virtual bool isInit() const = 0;
    virtual void callback(void (*cb)(uint32_t)) = 0;
    
    virtual std::optional<std::string> getIp() const = 0;
    virtual std::optional<std::string> getMac() const = 0;

    virtual std::time_t time() const = 0;
    virtual bool ping(const std::string& host, uint32_t count, uint32_t timeout_ms) const = 0;
};

}
