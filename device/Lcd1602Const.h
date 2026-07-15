#pragma once

class Lcd1602
{
    public:

        // Commands
        static constexpr uint8_t ScreenClear     = 0x01;
        static constexpr uint8_t CursorReturn    = 0x02;
        static constexpr uint8_t InputSet        = 0x04;
        static constexpr uint8_t DisplaySwitch   = 0x08;
        static constexpr uint8_t Shift           = 0x10;
        static constexpr uint8_t FunctionSet     = 0x20;
        static constexpr uint8_t CgramAdSet      = 0x40;
        static constexpr uint8_t DdramAdSet      = 0x80;
        
        // Pins
        static constexpr uint8_t RS              = 0x01;    // RS = 0 cmd, RS = 1 data
        static constexpr uint8_t RW              = 0x02;    // RW = 0 write, RW = 1 read
        static constexpr uint8_t EN              = 0x04;    // EN = 0 -> 1 -> 0 latch data
        static constexpr uint8_t BL              = 0x08;    // BL = 1 backlight on
};