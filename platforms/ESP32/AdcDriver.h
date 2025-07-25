#pragma once

#include "interface/Adc.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <cstdint>
#include <cstddef>

const static char *TAG = "ADC";

namespace driver
{

class AdcDriver : public IAdc
{
    public:

    AdcDriver(adc_unit_t adc, adc_channel_t channel)
        : _adc(adc), _channel(channel)
    {
    }

    void init(adc_bitwidth_t width, adc_atten_t atten)
    {
        //-------------ADC1 Init---------------//
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = _adc,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &_adc_oneshot));

        //-------------ADC1 Config---------------//
        adc_oneshot_chan_cfg_t config = {
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(_adc_oneshot, _channel, &config));

        //-------------ADC1 Calibration Init---------------//
        _calibrated = calibration_init(_adc, _channel, atten, &_cali_handle);
    }

    static bool calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
    {
        adc_cali_handle_t handle = NULL;
        esp_err_t ret = ESP_FAIL;
        bool calibrated = false;

    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        if (!calibrated) {
            ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
            adc_cali_curve_fitting_config_t cali_config = {
                .unit_id = unit,
                .chan = channel,
                .atten = atten,
                .bitwidth = ADC_BITWIDTH_DEFAULT,
            };
            ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
            if (ret == ESP_OK) {
                calibrated = true;
            }
        }
    #endif

    #if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        if (!calibrated) {
            ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
            adc_cali_line_fitting_config_t cali_config = {
                .unit_id = unit,
                .atten = atten,
                .bitwidth = ADC_BITWIDTH_DEFAULT,
            };
            ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
            if (ret == ESP_OK) {
                calibrated = true;
            }
        }
    #endif

        *out_handle = handle;
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Calibration Success");
        } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
            ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
        } else {
            ESP_LOGE(TAG, "Invalid arg or no memory");
        }

        return calibrated;
    }

    void deinit()
    {
    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
        ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(_cali_handle));

    #elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
        ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(_cali_handle));
    #endif
    }

    // Get current voltage in volts
    float getVoltage() override
    {
        int value;
        int voltage;
        adc_oneshot_read(_adc_oneshot, _channel, &value);
        if (_calibrated) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(_cali_handle, value, &voltage));
        }
        return voltage;
    }
    int32_t getRawValue() override
    {
        int value;
        adc_oneshot_read(_adc_oneshot, _channel, &value);
        return 0;
    }

    // Set callback for voltage change
    void onVoltageChange(void (*cb)(uint32_t)) override
    {

    }

    private:

    adc_unit_t _adc;
    adc_channel_t _channel;
    void (*_cb)(uint32_t) = nullptr;

    adc_oneshot_unit_handle_t _adc_oneshot;
    bool _calibrated = false;
    adc_cali_handle_t _cali_handle = NULL;
};

}