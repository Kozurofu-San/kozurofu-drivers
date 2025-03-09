#pragma once

#include <cstdint>

class Logs
{
    public:

    enum class Turn: bool
    {
        On = true,
        Off = false
    };

    virtual ~Logs() = default;

    /*
    * @brief Initialize the Logs
    */
    virtual void init() = 0;

    /*
    * @brief  Log Info function
    * @param  msg: message to log like printf
    */
    virtual void LOGI(const char* message, ...) = 0;

    /*
    * @brief  Log Warning function
    * @param  msg: message to log like printf
    */
    virtual void LOGW(const char* message, ...) = 0;
    
    /*
    * @brief  Log Error function
    * @param  msg: message to log like printf
    */
    virtual void LOGE(const char* message, ...) = 0;
    
    /*
    * @brief  Log Value function
    * @param  channel: 0 to 31
    * @param  value: value to log
    * @note   This function is used for the plotter
    */
    virtual void LOGV(uint32_t channel, int32_t value) = 0;
};