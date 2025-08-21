#pragma once

#include "interface/Communication.h"

#include "stm32f4xx.h"

namespace driver
{

class UsbDriver: public ICommunication
{
    public:

    UsbDriver(USB_OTG_GlobalTypeDef *usb)
        : _usb(usb) {}

    bool init()
    {
        // Clock
        RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
        _speed = HSE_VALUE
            * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos)
            / ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos)
            / ((RCC->PLLCFGR & RCC_PLLCFGR_PLLQ) >> RCC_PLLCFGR_PLLQ_Pos)
            ;

        _usb->GAHBCFG = 0;
        _usb->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;
        while (_usb->GRSTCTL & USB_OTG_GRSTCTL_CSRST);
        _usb->GUSBCFG |= USB_OTG_GUSBCFG_FDMOD;
        _usb->GUSBCFG = (_usb->GUSBCFG & ~USB_OTG_GUSBCFG_TRDT) | (9 << USB_OTG_GUSBCFG_TRDT_Pos);
        _usb->GCCFG |= USB_OTG_GCCFG_PWRDWN;
        _usb->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
        _usb->GCCFG &= ~(USB_OTG_GCCFG_VBUSASEN | USB_OTG_GCCFG_VBUSBSEN);
        _dev->DCFG &= ~USB_OTG_DCFG_DAD;
        _dev->DCFG |= USB_OTG_DCFG_DSPD;    // Full speed
        _usb->GAHBCFG |= USB_OTG_GAHBCFG_GINT;
        

        return true;
    }

    void write(uint8_t *data, size_t len, size_t bytes = 1) override
    {

    }

    void read (uint8_t *data, size_t len, size_t bytes = 1) override
    {

    }

    uint32_t sendCommand(uint32_t cmd) override
    {
        return 0;
    }

    void enable() override
    {
        
    }

    void disable() override
    {
        
    }

    uint32_t getSpeed() const override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    USB_OTG_GlobalTypeDef *_usb;
    USB_OTG_DeviceTypeDef *_dev = reinterpret_cast<USB_OTG_DeviceTypeDef*> (USB_OTG_DEVICE_BASE);
    USB_OTG_HostTypeDef *_host = reinterpret_cast<USB_OTG_HostTypeDef*> (USB_OTG_HOST_BASE);
    uint32_t _speed = 0;
    bool _isInit = false;
};

}