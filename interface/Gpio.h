#pragma once

class Gpio
{
    public:

    virtual ~Gpio() = default;

    /*
    * @brief Initialize the GPIO
    */
    virtual void init() = 0;

    /*
    * @brief Write a state to the GPIO
    * @param state The state to write
    */
    virtual void write(bool state) = 0;

    /*
    * @brief Read the state of the GPIO
    * @return The state of the GPIO
    */
    virtual bool read() = 0;
};