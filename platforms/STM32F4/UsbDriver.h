#pragma once

#include "interface/Communication.h"
#include "interface/Timer.h"
#include <stdio.h>

#include "stm32f4xx.h"

namespace driver
{

    constexpr uint8_t loByte(uint16_t x) noexcept {
        return static_cast<uint8_t>(x & 0xFF);
    }
    
    constexpr uint8_t hiByte(uint16_t x) noexcept {
        return static_cast<uint8_t>((x >> 8) & 0xFF);
    }
    
class UsbDriver
{
    public:

    UsbDriver(USB_OTG_GlobalTypeDef* usb, ITimer &timer)
        : _usb(usb),
            _timer(timer)
    {}

    bool init()
    {
        __disable_irq ();
        device_state = DEVICE_STATE_DEFAULT;

        // Clock
        RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

        _speed = HSE_VALUE
                * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos)
                / ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos)
                / ((RCC->PLLCFGR & RCC_PLLCFGR_PLLQ) >> RCC_PLLCFGR_PLLQ_Pos);
                
        _usb->GAHBCFG = USB_OTG_GAHBCFG_GINT; // Enable Global Interrupt
    
        _usb->GINTMSK = USB_OTG_GINTMSK_USBRST |
                        // USB_OTG_GINTMSK_ENUMDNEM |
                        // USB_OTG_GINTMSK_SOFM   |
                        USB_OTG_GINTMSK_OEPINT |
                        USB_OTG_GINTMSK_IEPINT |
                        USB_OTG_GINTSTS_RXFLVL;
        
        _usb->GCCFG = USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_NOVBUSSENS; // Power up
        _dev->DCTL = USB_OTG_DCTL_SDIS;             // Soft disconnect
        _pwr = 0;
        _usb->GUSBCFG =  USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_PHYSEL; // Force device mode
        _usb->GUSBCFG &= ~USB_OTG_GUSBCFG_TRDT;     // USB turnaround time
        _usb->GUSBCFG |= 0x6 << USB_OTG_GUSBCFG_TRDT_Pos;
        
        // FIFO sizes
        _usb->GRXFSIZ = RxFifoSize;     // All EPs RX FIFO RAM size
        _usb->DIEPTXF0_HNPTXFSIZ = (TxEp0FifoSize << USB_OTG_DIEPTXF_INEPTXFD_Pos) | (RxFifoSize << USB_OTG_DIEPTXF_INEPTXSA_Pos);
        _usb->DIEPTXF[Ep1 - 1] =   (TxEp1FifoSize) << USB_OTG_DIEPTXF_INEPTXFD_Pos | ((RxFifoSize + TxEp0FifoSize) << USB_OTG_DIEPTXF_INEPTXSA_Pos);
        _usb->DIEPTXF[Ep2 - 1] = 0;
        _usb->DIEPTXF[Ep3 - 1] = 0;
        // _usb->DIEPTXF[Ep2 - 1] =   ((TxEp2FifoSize) << USB_OTG_DIEPTXF_INEPTXFD_Pos) | (RxFifoSize+TxEp0FifoSize+TxEp1FifoSize);
        // _usb->DIEPTXF[Ep3 - 1] =   ((TxEp3FifoSize) << USB_OTG_DIEPTXF_INEPTXFD_Pos) | (RxFifoSize+TxEp0FifoSize+TxEp1FifoSize+TxEp2FifoSize); 

        /* Init  EP0: 1 Packet, 3*8 bytes */
        epOut(Ep0)->DOEPTSIZ = 1 << USB_OTG_DOEPTSIZ_PKTCNT_Pos // This field is decremented to zero after a packet is written into the RxFIFO
            | CdcMaxPacketSize << USB_OTG_DOEPTSIZ_XFRSIZ_Pos   // Set in descriptor
            | USB_OTG_DOEPTSIZ_STUPCNT;                         // STUPCNT==0x11 means, EP can recieve 3 packets. RM says to set STUPCNT = 3
        epOut(Ep0)->DOEPCTL |= USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA; // Clear NAK and enable EP0
    
        _dev->DCFG |= USB_OTG_DCFG_DSPD_Msk;    // Device speed - FS
        _usb->GINTSTS = 0xFFFFFFFF;             // Reset Global Interrupt status
        _dev->DCTL &= ~USB_OTG_DCTL_SDIS;       // Soft connect

        // Init EP
        for (uint32_t i = 0; i < EpCount; i++)
        {
            EndPoint[i].statusRx = EpReady;
            EndPoint[i].statusTx = EpReady;
            EndPoint[i].rxCounter = 0;
            EndPoint[i].txCounter = 0;
        }
        EndPoint[Ep0].rxBuffer_ptr = rxBufferEp0;     // RX Buffer for EP0
        EndPoint[Ep1].rxBuffer_ptr = rxBufferEp1;	  // RX Buffer for EP1

        // Interrupts
        NVIC_SetPriority(OTG_FS_IRQn, 6);
        NVIC_EnableIRQ(OTG_FS_IRQn);
        __enable_irq();

        _isInit = true;
        return true;
    }
    
    unsigned int cnt = 0;
    void interrupt()
    {
        // Reset
        if (_usb->GINTSTS & USB_OTG_GINTSTS_USBRST)
        {
            _usb->GINTSTS = USB_OTG_GINTSTS_USBRST;
            enumerateReset();
            return;
        }

        // IN endpoint
        if (_usb->GINTSTS & USB_OTG_GINTSTS_IEPINT)
        {
            uint32_t epnums  = _dev->DAINT;         // Read out EndPoint INTerrupt bits
            
            if (epnums & (Ep0Mask << USB_OTG_DAINT_IEPINT_Pos))
            {
                uint16_t intr = epIn(Ep0)->DIEPINT;   // Read out EP interrupt bit
                printf("\nI0(%04X)", intr);
                if(intr & USB_OTG_DIEPINT_XFRC)     // Transfer completed interrupt
                {
                    transferTxCallback(Ep0);          // Process TX transmission (if TX buffer is not empty)
                }
                epIn(Ep0)->DIEPINT = intr;
            }

            if (epnums & (Ep1Mask << USB_OTG_DAINT_IEPINT_Pos))
            {
                uint32_t intr = epIn(Ep1)->DIEPINT;
                printf("\nI1(%04X)", intr);
                if (intr & USB_OTG_DIEPINT_XFRC)     // Transfer completed interrupt
                {
                    transferTxCallback(Ep1);			// Process TX transmission (if TX buffer is not empty)
                    epIn(Ep1)->DIEPINT = USB_OTG_DIEPINT_XFRC;	
                }
            }
            return;
        }
        
        // OUT endpoint
        if(_usb->GINTSTS & USB_OTG_GINTSTS_OEPINT)
        {
            uint32_t epnums  = _dev->DAINT;    // Read out EndPoint INTerrupt bits
            
            if (epnums & (Ep0Mask << USB_OTG_DAINT_OEPINT_Pos))
            {
                uint32_t intr = epOut(Ep0)->DOEPINT; // Read out Endpoint Interrupt register for EP0
                printf("\nO0(%04X)", intr);
                if (intr & USB_OTG_DOEPINT_STUP)    // Setup packet recieved
                {
                    enumerate_Setup();
                }
                if (intr & USB_OTG_DOEPINT_XFRC){
                    transferRXCallbackEp0(Ep0);
                    // CNAK and EPENA must be set again after every interrupt to let this EP recieve upcoming data	
                    epOut(Ep0)->DOEPCTL |= (USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);	
                }
                epOut(Ep0)->DOEPINT = intr;
            }

            if (epnums & (Ep1Mask << USB_OTG_DAINT_OEPINT_Pos))
            {
                uint32_t epint = epOut(Ep1)->DOEPINT;
                printf("\nO1(%04X)", epint);
                if (epint & USB_OTG_DOEPINT_XFRC)
                {
                    transferRXCallbackEp1(EpOk);
                    // CNAK and EPENA must be set again after every interrupt to let this EP recieve upcoming data
                    epOut(Ep1)->DOEPCTL |= (USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
                }
                epOut(Ep1)->DOEPINT = epint;
            }
            return;
        }

        // RX FIFO not empty
        if (_usb->GINTSTS & USB_OTG_GINTSTS_RXFLVL)
        {
            uint32_t temp = _usb->GRXSTSP;   // Read out packet status

            if (((temp & USB_OTG_GRXSTSP_PKTSTS) >> USB_OTG_GRXSTSP_PKTSTS_Pos) ==  StsDataUpdt) // 0010: OUT data packet received
            {
                printf("\n%d d", cnt++); 
                if (temp & USB_OTG_GRXSTSP_BCNT)     // Byte count > 0
                {
                    uint16_t length = ((temp & USB_OTG_GRXSTSP_BCNT) >> USB_OTG_GRXSTSP_BCNT_Pos);
                    uint8_t EpNum =	((temp & USB_OTG_GRXSTSP_EPNUM) >> USB_OTG_GRXSTSP_EPNUM_Pos);
                    readFifo(EpNum, length);   //  Read data from DFIFO
                }
            }
            else if (((temp & USB_OTG_GRXSTSP_PKTSTS) >> USB_OTG_GRXSTSP_PKTSTS_Pos) ==  StsSetupUpdt)  // 0110: SETUP data packet received
            {
                // Read FFIO
                setup_pkt_data.raw_data[0] = *fifo(0);
                setup_pkt_data.raw_data[1] = *fifo(0);
            }
            printf("\n"); 
            return;
        }
    }
    
    uint32_t write(uint8_t *txBuff, uint16_t len)
    {
        if (len == 0) return EpOk;
        
        if (!(device_state & DEVICE_STATE_TX_PR) &
            !(epIn(Ep1)->DIEPTSIZ & USB_OTG_DIEPTSIZ_XFRSIZ) &
            ((epIn(Ep1)->DIEPTSIZ & USB_OTG_HCTSIZ_PKTCNT) == 0) &
            !(epIn(Ep1)->DIEPCTL & USB_OTG_DIEPCTL_EPENA)  &
            ((epIn(Ep1)->DIEPINT & USB_OTG_DIEPINT_TXFE) != 0))
        {
            setTxBuffer(Ep1, txBuff, len);
            return EpOk;
        }
        else return EpFailed;
    }
    
    uint32_t read(uint16_t length){
        uint8_t *data = EndPoint[Ep1].rxBuffer_ptr;
        for (uint32_t i = 0; i < length; i++)
        {
            data[i] = data[i] + 1;
        }
        
        write(data, length);
        return length;
    }

    inline bool isInit() const { return _isInit; }
    inline uint32_t getSpeed() const { return _speed; }

    USB_OTG_GlobalTypeDef* getUsb() { return _usb; }
    
    private:

    USB_OTG_GlobalTypeDef   *_usb;
    USB_OTG_DeviceTypeDef   *_dev = reinterpret_cast<USB_OTG_DeviceTypeDef*>(USB_OTG_FS_PERIPH_BASE + USB_OTG_DEVICE_BASE);
    USB_OTG_HostTypeDef     *_host = reinterpret_cast<USB_OTG_HostTypeDef*>(USB_OTG_FS_PERIPH_BASE + USB_OTG_HOST_BASE);
    __IO uint32_t           *_pwr = reinterpret_cast<__IO uint32_t*>(USB_OTG_FS_PERIPH_BASE + USB_OTG_PCGCCTL_BASE);

    ITimer &_timer;
    uint32_t _speed = 0;
    bool _isInit = false;
    
    static constexpr uint32_t Ep0 = 0;
    static constexpr uint32_t Ep1 = 1;
    static constexpr uint32_t Ep2 = 2;
    static constexpr uint32_t Ep3 = 3;
    
    static constexpr uint32_t Ep0Mask = 1 << 0;
    static constexpr uint32_t Ep1Mask = 1 << 1;
    static constexpr uint32_t Ep2Mask = 1 << 2;
    static constexpr uint32_t Ep3Mask = 1 << 3;

    static constexpr uint8_t CdcMaxPacketSize   = 64;
    static constexpr uint8_t CdcCmdPacketSize   = 8;
    static constexpr uint8_t Ep0Size            = 64;
    static constexpr uint8_t EpCount            = 3;

    static constexpr uint16_t Vid = 0x0483;
    static constexpr uint16_t Pid = 0x5740;

    static constexpr uint16_t LangIdString = 1033;

    static constexpr uint8_t DeviceDescriptorLength         = 18;
    static constexpr uint8_t ConfigurationDescriptorLength  = 67;
    static constexpr uint8_t LangDescriptorLength           = 4;
    static constexpr uint8_t MfcDescriptorLength            = 38;
    static constexpr uint8_t ProductDescriptorLength        = 44;
    static constexpr uint8_t SerialDescriptorLength         = 26;
    static constexpr uint8_t DeviceQualifierLength          = 10;
    static constexpr uint8_t InterfaceStringLength          = 28;
    static constexpr uint8_t ConfigStringLength             = 22;
    static constexpr uint8_t CdcLineCodingLength            = 7;

    static constexpr uint16_t FlushFifoTimeout  = 2000;
    static constexpr uint16_t DtfXstsTimeout    = 1024;
    static constexpr uint16_t RxFifoSize        = 36;
    static constexpr uint16_t TxEp0FifoSize     = 16;
    static constexpr uint16_t TxEp1FifoSize     = 320-(RxFifoSize+TxEp0FifoSize);
    static constexpr uint16_t TxEp2FifoSize     = 0;
    static constexpr uint16_t TxEp3FifoSize     = 0;
    static constexpr uint16_t Ep1DtfXstsSize    = TxEp1FifoSize;
    static constexpr uint16_t Ep1MinDtfXstsLvl  = 16;
    static constexpr uint16_t MaxCdcEp0TxSiz    = 64;
    static constexpr uint16_t MaxCdcEp1TxSiz    = 256;
    static constexpr uint8_t DoeptTransferSize  = 0x40;
    static constexpr uint8_t DoeptTransferPct   = 0x01;

    static constexpr uint8_t EpReady    = 0;
    static constexpr uint8_t EpBusy     = 1;
    static constexpr uint8_t EpZlp      = 2;

    static constexpr uint8_t EpOk       = 1;
    static constexpr uint8_t EpFailed   = 0;

    static constexpr uint16_t ReqTypeHostToDeviceGetDeviceDecriptor = 0x0680;
    static constexpr uint16_t ReqTypeDeviceToHostSetAddress         = 0x0500;
    static constexpr uint16_t ReqTypeDeviceToHostSetConfiguration   = 0x0900;
    static constexpr uint16_t DescriptorTypeDevice                  = 0x0100;
    static constexpr uint16_t DescriptorTypeConfiguration           = 0x0200;
    static constexpr uint16_t DescriptorTypeLangString              = 0x0300;
    static constexpr uint16_t DescriptorTypeMfcString               = 0x0301;
    static constexpr uint16_t DescriptorTypeProdString              = 0x0302;
    static constexpr uint16_t DescriptorTypeSerialString            = 0x0303;
    static constexpr uint16_t DescriptorTypeConfigurationString     = 0x0304;
    static constexpr uint16_t DescriptorTypeInterfaceString         = 0x0305;
    static constexpr uint16_t DescriptorTypeDeviceQualifier         = 0x0600;
    static constexpr uint16_t CdcGetLineCoding                      = 0x21A1;
    static constexpr uint16_t CdcSetLineCoding                      = 0x2021;
    static constexpr uint16_t CdcSetControlLineState                = 0x2221;
    static constexpr uint16_t ClearFeatureEndp                      = 0x0102;

    static constexpr uint8_t StsDataUpdt    = 2;
    static constexpr uint8_t StsSetupUpdt   = 6;

    static constexpr uint16_t RxBufferEp0Size = 8;
    static constexpr uint16_t RxBufferEp1Size = 128;

    typedef enum
    {
        DEVICE_STATE_DEFAULT    = 0,
        DEVICE_STATE_RESET      = 1,
        DEVICE_STATE_ADDRESSED  = 2,
        DEVICE_STATE_LINECODED  = 4,
        DEVICE_STATE_TX_PR      = 8, /* TX transmission active */
    } eDeviceState;
    
    
    static constexpr inline USB_OTG_OUTEndpointTypeDef* epOut(uint32_t i)
    {
        return reinterpret_cast<USB_OTG_OUTEndpointTypeDef*>(
            USB_OTG_FS_PERIPH_BASE + USB_OTG_OUT_ENDPOINT_BASE + (i * USB_OTG_EP_REG_SIZE));
    }
    
    static constexpr inline USB_OTG_INEndpointTypeDef* epIn(uint32_t i)
    {
        return reinterpret_cast<USB_OTG_INEndpointTypeDef*>(
            USB_OTG_FS_PERIPH_BASE + USB_OTG_IN_ENDPOINT_BASE + (i * USB_OTG_EP_REG_SIZE));
    }
    
    static constexpr inline uint32_t* fifo(uint32_t i)
    {
        return reinterpret_cast<uint32_t*>(
            USB_OTG_FS_PERIPH_BASE  + USB_OTG_FIFO_BASE + (i) * USB_OTG_FIFO_SIZE);
    }

    typedef struct EndPointStruct
    {
        uint16_t statusRx;
        uint16_t statusTx;
    
        uint16_t rxCounter;
        uint16_t txCounter;
        
        uint8_t *rxBuffer_ptr;
        uint8_t *txBuffer_ptr;
    } EndPointStruct;
    EndPointStruct EndPoint[EpCount];	/* All the Enpoints are included in this array */

    typedef struct
    {
        uint16_t  wRequest;
        uint16_t  wValue;
        uint16_t  wIndex;
        uint16_t  wLength;
    } USB_setup_req;	/* SETUP packet buffer. Always 8 bytes */

    typedef union
    {
        USB_setup_req setup_pkt;
        uint32_t raw_data[2];
    } USB_setup_req_data;

    uint8_t lineCoding[CdcLineCodingLength] =
    {
        0x00, 
        0x00,   // 0x01,
        0x00,   // 0xC2,
        0x00,   // 0X0001C200 ~= 115200 Kb/s
        0x00,
        0x00,
        0x00    // 0x08
    };

    uint32_t device_state = DEVICE_STATE_DEFAULT; /* Device state */

    uint32_t sendZlp(uint8_t EPnum)
    {
        epIn(EPnum)->DIEPTSIZ = 1 << USB_OTG_DIEPTSIZ_PKTCNT_Pos   // One Packet
            | 0 << USB_OTG_DIEPTSIZ_XFRSIZ_Pos;          // Zero Length
        epIn(EPnum)->DIEPCTL |= USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA;
    
        while (epIn(EPnum)->DIEPTSIZ);  // Make sure zlp is gone
    
        epOut(EPnum)->DOEPCTL |= (USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
        device_state &= ~DEVICE_STATE_TX_PR;
    
        EndPoint[EPnum].statusTx = EpReady;
        epIn(EPnum)->DIEPCTL |= USB_OTG_DIEPCTL_SNAK;
    
        if (isEpStuck(EPnum) == EpOk)
        {
            return EpOk;
        }
        else return EpFailed;
    }
    
    void togglRxEpStatus(uint8_t EPnum, uint8_t param)
    {
        if (EndPoint[EPnum].statusRx == param) return;
        EndPoint[EPnum].statusRx = param;      // Toggle status
    
        if (param == EpReady)
        {
            epOut(EPnum)->DOEPCTL |= USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA;
        }
        else
        {
            epOut(EPnum)->DOEPCTL |= USB_OTG_DOEPCTL_SNAK;
        }
    }
    
    void readFifo(uint8_t dfifo, uint16_t len)
    {
        uint16_t residue = (len % sizeof(uint32_t)) ? 1 : 0;
        uint32_t block_cnt = (len / sizeof(uint32_t)) + residue;
        uint8_t *tmp_ptr = EndPoint[dfifo].rxBuffer_ptr;
    
        // If unprocessed data length exceeds Max buffer length, it has to be rewritten
        if (dfifo & ((EndPoint[dfifo].rxCounter + len) > RxBufferEp1Size))
        {
            EndPoint[dfifo].rxBuffer_ptr = rxBufferEp1;
            EndPoint[dfifo].rxCounter = 0;
        }

        uint32_t *ptr = reinterpret_cast<uint32_t *>(EndPoint[dfifo].rxBuffer_ptr);
        for (uint32_t i = 0; i < block_cnt; i++)
        {
            *ptr++ = *fifo(0);
            // printf(" r%08X", *ptr);
        }
    
        if (dfifo)
        {
            epOut(dfifo)->DOEPTSIZ = 0;			
            epOut(dfifo)->DOEPTSIZ |= (USB_OTG_DOEPTSIZ_PKTCNT & (DoeptTransferPct << USB_OTG_DOEPTSIZ_PKTCNT_Pos)); 
            epOut(dfifo)->DOEPTSIZ |= DoeptTransferSize; 
            epOut(dfifo)->DOEPCTL |= (USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
        }
        
        EndPoint[dfifo].rxBuffer_ptr = tmp_ptr + len;
        EndPoint[dfifo].rxCounter = EndPoint[dfifo].rxCounter + len;
    }
    
    uint32_t writeFifo(uint8_t dfifo, uint8_t *src, uint16_t len)
    {
        uint16_t residue = (len % sizeof(uint32_t)) ? 1 : 0;
        uint32_t block_cnt = (len / sizeof(uint32_t)) + residue;
        uint32_t status = epIn(dfifo)->DTXFSTS;
        
        uint32_t *ptr = reinterpret_cast<uint32_t *>(src);
        for (uint32_t i = 0; (i < block_cnt) ; i++)
        {
            // printf(" w%08X", *ptr);
            *fifo(dfifo) = *ptr++;
        }
        
        if (dfifo == 0)     // Check status
        {
            volatile uint32_t count = 0;
            do
            {
                if (++count > DtfXstsTimeout)
                {
                    return EpFailed;
                }
            } while (!(epIn(dfifo)->DTXFSTS == status));
        }

        EndPoint[dfifo].txBuffer_ptr = EndPoint[dfifo].txBuffer_ptr + len;
        EndPoint[dfifo].txCounter =EndPoint[dfifo].txCounter - len;

        return EpOk;
    }
    
    uint32_t transferTxCallback(uint8_t EPnum)
    {
        if (EndPoint[EPnum].statusTx == EpBusy)
        {
            epOut(EPnum)->DOEPCTL |= USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA;	
            return EpFailed;
        }	

        if (!EndPoint[EPnum].txCounter)        // No data in TX Buffer
        {
            if(EndPoint[EPnum].statusTx == EpZlp)
            {			
                if(sendZlp(EPnum) == EpOk)
                {
                    return EpOk;
                }
                else return EpFailed;
            }
            epOut(EPnum)->DOEPCTL |= USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA;	
            device_state &= ~DEVICE_STATE_TX_PR;			
            EndPoint[EPnum].statusTx = EpReady;
            epIn(EPnum)->DIEPCTL |= USB_OTG_DIEPCTL_SNAK;
            return EpOk;
        }
    
        uint16_t max_tx_sz = (EPnum==0) ? MaxCdcEp0TxSiz : MaxCdcEp1TxSiz; // Max transfer size considering TX FIFO size

        uint32_t len;
        uint32_t residue;
        uint32_t pct_cnt;
            
        if (EndPoint[EPnum].txCounter < CdcMaxPacketSize)
        {       /* if FIFO size is 64 bytes, but transaction is 67 bytes (DevDescriptor) transaction will be split into two parts: 1 - with 64 bytes length and 2 - with 3 bytes length */
            len = EndPoint[EPnum].txCounter;
            residue = 0;
            pct_cnt = 1;
        }
        else
        {
            len = (EndPoint[EPnum].txCounter > max_tx_sz) ? max_tx_sz : EndPoint[EPnum].txCounter; 
            residue = (len % CdcMaxPacketSize) ? 1 : 0;
            pct_cnt = len / CdcMaxPacketSize + residue;
        }

        EndPoint[EPnum].statusTx = EpBusy;      //Set Busy flag

        epIn(EPnum)->DIEPTSIZ = pct_cnt << USB_OTG_DIEPTSIZ_PKTCNT_Pos
            | len << USB_OTG_DIEPTSIZ_XFRSIZ_Pos;
        epIn(EPnum)->DIEPCTL |= USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA;
        
        while (writeFifo(EPnum, EndPoint[EPnum].txBuffer_ptr, len) == EpFailed)
        {
            if ((EPnum == Ep1) & (isEpStuck(Ep1) == EpFailed))       // Recovery Routine EP IN
            {
                epIn(EPnum)->DIEPCTL |= USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK;
                epIn(EPnum)->DIEPTSIZ = 0;
            }
            epIn(EPnum)->DIEPTSIZ = pct_cnt << USB_OTG_DIEPTSIZ_PKTCNT_Pos
                | len << USB_OTG_DIEPTSIZ_XFRSIZ_Pos;
            epIn(EPnum)->DIEPCTL |= USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA;
            
            if ((EPnum == Ep1) & (epIn(EPnum)->DTXFSTS >= Ep1MinDtfXstsLvl))  // Check Fifo Free Space
            {
                // flushTxFifo(Ep1, FlushFifoTimeout);
                
                uint32_t count = 0;
                _usb->GRSTCTL = USB_OTG_GRSTCTL_TXFFLSH | (EPnum << USB_OTG_GRSTCTL_TXFNUM_Pos);
                do
                {
                    if (++count > FlushFifoTimeout)
                    {
                        break;
                    }
                }
                while (_usb->GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH);
            
               return EpOk;

            }
        }

        if (EndPoint[EPnum].txCounter >= CdcMaxPacketSize)
        {
            if (!EndPoint[EPnum].txCounter && !residue)  // Was this packet of MaxSize the last one in the queue ? Is ZLP required?
            {
                EndPoint[EPnum].statusTx = EpZlp;        // Change EP TX status to ZLP, thereafter ZLP will be sent in sequential function call
                while (isEpStuck(EPnum) != EpOk);
                sendZlp(EPnum);
                return EpOk;
            }
        }
        else
        {
            device_state &= ~DEVICE_STATE_TX_PR;		
        }

        epOut(EPnum)->DOEPCTL |= USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA;  // Finish transmission
        EndPoint[EPnum].statusTx = EpReady;
        device_state &= ~DEVICE_STATE_TX_PR;

        return EpOk;
    }
    
    
    uint32_t setTxBuffer(uint8_t EPnum, uint8_t *txBuff, uint16_t len)
    {
        if (EndPoint[EPnum].txCounter || (EndPoint[EPnum].statusTx == EpZlp) || (device_state & DEVICE_STATE_TX_PR))
        {
            return EpFailed;    // Previous transaction is not finished
        }
    
        uint32_t max_transfer_sz = CdcMaxPacketSize - 1;
        max_transfer_sz += (EPnum == 0) ? MaxCdcEp0TxSiz : MaxCdcEp1TxSiz;
        if (len > max_transfer_sz) return EpFailed;
        
        if (len)
        {
            EndPoint[EPnum].txBuffer_ptr = txBuff;
            EndPoint[EPnum].txCounter = len;
            device_state |= DEVICE_STATE_TX_PR;
            transferTxCallback(EPnum);  // Send data
            
            return EpOk;
        }
        
        else    // ZLP
        {
            epIn(EPnum)->DIEPTSIZ = 0;
            epIn(EPnum)->DIEPTSIZ = 1 << USB_OTG_DIEPTSIZ_PKTCNT_Pos    // One Packet
                | 0 << USB_OTG_DIEPTSIZ_XFRSIZ_Pos;	                    //  Size 0
            epIn(EPnum)->DIEPCTL |= USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA;
        
            epOut(EPnum)->DOEPCTL |= USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA;
            return EpOk;
        }
    }
    
    uint32_t transferRXCallbackEp0(uint32_t param)
    {
        uint16_t len = EndPoint[Ep0].rxCounter;	
        if(!len) return EpOk;
        if(EndPoint[Ep0].statusRx == EpBusy) return EpFailed;
        
        if(param == CdcSetLineCoding)
        {
            togglRxEpStatus(0, EpBusy);
            
            uint8_t *data = EndPoint[Ep0].rxBuffer_ptr - EndPoint[Ep0].rxCounter;
            EndPoint[Ep0].rxBuffer_ptr = data;
            uint8_t new_linecoding_settings[CdcLineCodingLength];  
            
            for(int i = 0; i < len; i++)
            {
                new_linecoding_settings[i] = *data++;
                EndPoint[Ep0].rxCounter--;
            }
                
            // line coding 00 C2 01 00000000 000000000000 08
            for(uint32_t i = 0; i < CdcLineCodingLength; i++)
            {
                if(i==6 && lineCoding[i]==0x08)
                {
                    lineCoding[i] = 0x08;
                }
                else
                {
                    lineCoding[i] = new_linecoding_settings[i];
                }
            }	
            togglRxEpStatus(0, EpReady);
        }
        return EpOk;
    }
    
    uint32_t transferRXCallbackEp1(uint32_t param)
    {
        if (EndPoint[Ep1].statusRx == EpBusy) return EpFailed;
        uint16_t len = EndPoint[Ep1].rxCounter;
        
        EndPoint[Ep1].rxBuffer_ptr -= EndPoint[Ep1].rxCounter;      // Reset RX counter and buffer pointer
        EndPoint[Ep1].rxCounter = 0;	
        
        read(len);
    
        epOut(1)->DOEPTSIZ = DoeptTransferPct << USB_OTG_DOEPTSIZ_PKTCNT_Pos
            | DoeptTransferSize << USB_OTG_DOEPTSIZ_XFRSIZ_Pos;
        epOut(1)->DOEPCTL |= USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA;
        
        return param;
    }
    
    void enumerateReset()
    {
        device_state = DEVICE_STATE_RESET;  
        _usb->GINTSTS &= ~0xFFFFFFFF;
        
        _dev->DAINTMSK = 
            Ep0Mask << USB_OTG_DAINTMSK_IEPM_Pos |
            Ep1Mask << USB_OTG_DAINTMSK_IEPM_Pos |
            Ep0Mask << USB_OTG_DAINTMSK_OEPM_Pos |
            Ep1Mask << USB_OTG_DAINTMSK_OEPM_Pos ;

        _dev->DOEPMSK = USB_OTG_DOEPMSK_STUPM | USB_OTG_DOEPMSK_XFRCM;  // Unmask SETUP Phase done Mask, Transfer Completed interrupt for OUT
        _dev->DIEPMSK = USB_OTG_DIEPMSK_XFRCM;                          // TransfeR Completed interrupt for IN
        _dev->DCFG &= ~USB_OTG_DCFG_DAD_Msk;                            // Before Enumeration set address 0
    
        epIn(1)->DIEPCTL =
            USB_OTG_DIEPCTL_SNAK    |
            USB_OTG_DIEPCTL_TXFNUM_0|   // TX Number 1
            USB_OTG_DIEPCTL_EPTYP_1 |   // Eptype 10 means Bulk
            USB_OTG_DIEPCTL_USBAEP  |   // Set Endpoint active
            CdcMaxPacketSize;           // Max Packet size (bytes)
        
        epOut(1)->DOEPCTL =
            USB_OTG_DOEPCTL_EPENA   |   // Enable Endpoint
            USB_OTG_DOEPCTL_CNAK    |   // Clear NAK
            USB_OTG_DOEPCTL_EPTYP_1 |   // Eptype 10 means Bulk
            USB_OTG_DOEPCTL_USBAEP  |   // Set Endpoint active
            CdcMaxPacketSize;           // CHK MPSIZ The application must program this field with the maximum packet size for the current logical endpoint. This value is in bytes
    	
        epOut(1)->DOEPTSIZ = DoeptTransferPct << USB_OTG_DOEPTSIZ_PKTCNT_Pos   // RM quote: Indicates the total number of USB packets that constitute the Transfer Size amount of data for this endpoint. This field is decremented every time a packet (maximum size or short packet) is written to the RxFIFO
            | DoeptTransferSize << USB_OTG_DOEPTSIZ_XFRSIZ_Pos;                // Transfer size. If you set transfer size = max. packet, the core will interrupt the application at the end of each packet
    
        epIn(2)->DIEPCTL =
            USB_OTG_DIEPCTL_SNAK    |
            USB_OTG_DIEPCTL_TXFNUM_1|
            USB_OTG_DIEPCTL_EPTYP   |     // Eptype 11 means Interrupt EP 
            USB_OTG_DIEPCTL_USBAEP  |
            8 << USB_OTG_DIEPCTL_MPSIZ_Pos;                                                       
    }
    
    void enumerate_Setup()
    {
        uint16_t len = setup_pkt_data.setup_pkt.wLength;
        uint8_t *ptr;
        // printf(" s%04Xv%04X[%d]", setup_pkt_data.setup_pkt.wRequest, setup_pkt_data.setup_pkt.wValue, len);
        switch(setup_pkt_data.setup_pkt.wRequest)
        {
            case ReqTypeHostToDeviceGetDeviceDecriptor:
                switch(setup_pkt_data.setup_pkt.wValue)
                {
                    case DescriptorTypeDevice:                  // Request 0x0680  Value 0x0100 
                        if(DeviceDescriptorLength < len) len = DeviceDescriptorLength;
                        ptr = const_cast<uint8_t*>(deviceDescriptor);
                        break;
                    case DescriptorTypeConfiguration:           // Request 0x0680  Value 0x0200
                        if(ConfigurationDescriptorLength < len) len = ConfigurationDescriptorLength;
                        ptr = const_cast<uint8_t*>(configurationDescriptor);
                        break;    
                    case DescriptorTypeDeviceQualifier:         // Request 0x0680  Value 0x0600
                        if(DeviceQualifierLength < len) len = DeviceQualifierLength; 
                        ptr = const_cast<uint8_t*>(deviceQualifierDescriptor);
                        break;          
                    case DescriptorTypeLangString:              // Request 0x0680  Value 0x0300
                        if(LangDescriptorLength < len) len = LangDescriptorLength;   
                        ptr = const_cast<uint8_t*>(languageStringDescriptor);
                        break; 
                    case DescriptorTypeMfcString:               // Request 0x0680  Value 0x0301
                        if(MfcDescriptorLength < len) len = MfcDescriptorLength;
                        ptr = const_cast<uint8_t*>(manufactorStringDescriptor);
                        break;
                    case DescriptorTypeProdString:              // Request 0x0680  Value 0x0302
                        if(ProductDescriptorLength < len) len = ProductDescriptorLength;
                        ptr = const_cast<uint8_t*>(productStringDescriptor);
                        break;                     
                    case DescriptorTypeSerialString:            // Request 0x0680  Value 0x0303
                        if(SerialDescriptorLength < len) len = SerialDescriptorLength;
                        ptr = const_cast<uint8_t*>(serialNumberStringDescriptor);
                        break;
                    case DescriptorTypeConfigurationString:     // Request 0x0680  Value 0x0304
                        if(ConfigStringLength < len) len = ConfigStringLength;
                        ptr = const_cast<uint8_t*>(configurationStringDescriptor);
                        break;
                    case DescriptorTypeInterfaceString:         // Request 0x0680  Value 0x0305
                        if(InterfaceStringLength < len) len = InterfaceStringLength;
                        ptr = const_cast<uint8_t*>(stringInterface);
                        break;
                    default:
                        return;
                }
                break;
                
            case ReqTypeDeviceToHostSetAddress:                 // Request 0x0500
                _dev->DCFG |= setup_pkt_data.setup_pkt.wValue << 4;
                device_state |= DEVICE_STATE_ADDRESSED;
                break;
            case ReqTypeDeviceToHostSetConfiguration:           // Request 0x0900
                len = 0;    // ZLP
                // TODO: set configuration
                break;     
            
            case CdcGetLineCoding:                              // Request 0x21A1
                if(CdcLineCodingLength < len) len = CdcLineCodingLength;
                ptr = const_cast<uint8_t*>(lineCoding);
                device_state |= DEVICE_STATE_LINECODED;
                break;
            
            case CdcSetLineCoding:                              // Request 0x2021
                len = 0;		
                transferRXCallbackEp0(CdcSetLineCoding); // TODO
                break;       
            case CdcSetControlLineState:                        // Request 0x2221
                len = 0;	
                break;	
            case ClearFeatureEndp:                              // Request 0x0201
                return;
            default:
                break;
        } 
        
        setTxBuffer(Ep0, ptr, len);
    }
    
    uint32_t isEpStuck(uint8_t EPnum)
    {
        if ((epIn(EPnum)->DIEPCTL & USB_OTG_DIEPCTL_EPENA)  &       // EPENA stuck
            !(epIn(EPnum)->DIEPTSIZ & USB_OTG_DIEPTSIZ_XFRSIZ) &    // No data pending
            (epIn(EPnum)->DIEPTSIZ & USB_OTG_HCTSIZ_PKTCNT))        // Packet count pending
        {
            return EpFailed;
        }
        else return EpOk;
    }
    
    uint8_t rxBufferEp0[RxBufferEp0Size];   // Recieved data is stored here after application reads DFIFO. RX FIFO is shared
    uint8_t rxBufferEp1[RxBufferEp1Size];   // Recieved data is stored here after application reads DFIFO. RX FIFO is shared
    
    USB_setup_req_data setup_pkt_data;      // Setup Packet var

    /* Device string descriptor */
    static constexpr uint8_t deviceDescriptor[DeviceDescriptorLength] = {
        DeviceDescriptorLength,     
        0x01,                       /* Descriptor type - device */
        0x00, 0x02,                 /*  0x0110 = usb 1.1 ; 0x0200 = usb 2.0 */
        0x02,                       /* CDC */
        0x02,                       /*  Abstract Control Model subclass */
        0x00,                       /* protocol */
        Ep0Size,                    /* EP0 size */
        loByte(Vid), hiByte(Vid),   /* VID */
        loByte(Pid), hiByte(Pid),   /* PID */
        0x00,                       /* ver. (BCD) */
        0x02,                       /* ver. (BCD) */	
        0x01,                       /* Manufactor string index */
        0x02,                       /* Product string index */
        0x03,                       /* Serial number string index */
        1                           /* configuration count */
    };

    /* Configuration descriptor */
    static constexpr uint8_t configurationDescriptor[ConfigurationDescriptorLength] = {
        /* Configuration descriptor */
        0x09,                           /* bLength: Configuration Descriptor size */
        0x02,                           /* bDescriptorType: Configuration */
        ConfigurationDescriptorLength,  /* wTotalLength:no of returned bytes */
        0x00,
        0x02,                           /* bNumInterfaces: 2 interface */
        0x01,                           /* bConfigurationValue: Configuration value */
        0x00,                           /* iConfiguration: Index of string descriptor describing the configuration */
        0xC0,                           /* bmAttributes: self powered */
        0x32,                           /* MaxPower 0 mA */

        /*Interface Descriptor */
        0x09,           /* bLength: Interface Descriptor size */
        0x04,           /* bDescriptorType: Interface */
        /* Interface descriptor type */
        0x00,           /* bInterfaceNumber: Number of Interface */
        0x00,           /* bAlternateSetting: Alternate setting */
        0x01,           /* bNumEndpoints: One endpoints used */
        0x02,           /* bInterfaceClass: Communication Interface Class */
        0x02,           /* bInterfaceSubClass: Abstract Control Model */
        0x01,           /* bInterfaceProtocol: Common AT commands */
        0x00,           /* iInterface: */

        /*Header Functional Descriptor*/
        0x05,           /* bLength: Endpoint Descriptor size */
        0x24,           /* bDescriptorType: CS_INTERFACE */
        0x00,           /* bDescriptorSubtype: Header Func Desc */
        0x10,           /* bcdCDC: spec release number */
        0x01,

        /*Call Management Functional Descriptor*/
        0x05,           /* bFunctionLength */
        0x24,           /* bDescriptorType: CS_INTERFACE */
        0x01,           /* bDescriptorSubtype: Call Management Func Desc */
        0x00,           /* bmCapabilities: D0+D1 */
        0x01,           /* bDataInterface: 1 */

        /*ACM Functional Descriptor*/
        0x04,           /* bFunctionLength */
        0x24,           /* bDescriptorType: CS_INTERFACE */
        0x02,           /* bDescriptorSubtype: Abstract Control Management desc */
        0x02,           /* bmCapabilities */

        /*Union Functional Descriptor*/
        0x05,           /* bFunctionLength */
        0x24,           /* bDescriptorType: CS_INTERFACE */
        0x06,           /* bDescriptorSubtype: Union func desc */
        0x00,           /* bMasterInterface: Communication class interface */
        0x01,           /* bSlaveInterface0: Data Class Interface */

        /*Endpoint 2 Descriptor*/
        0x07,           /* bLength: Endpoint Descriptor size */
        0x05,           /* bDescriptorType: Endpoint */
        0x82,           /* bEndpointAddress */
        0x03,           /* bmAttributes: Interrupt */
        loByte(CdcCmdPacketSize), hiByte(CdcCmdPacketSize),    /* wMaxPacketSize: */
        0x10,           /* bInterval: */

        /*Data class interface descriptor*/
        0x09,           /* bLength: Endpoint Descriptor size */
        0x04,           /* bDescriptorType: */
        0x01,           /* bInterfaceNumber: Number of Interface */
        0x00,           /* bAlternateSetting: Alternate setting */
        0x02,           /* bNumEndpoints: Two endpoints used */
        0x0A,           /* bInterfaceClass: CDC */
        0x00,           /* bInterfaceSubClass: */
        0x00,           /* bInterfaceProtocol: */
        0x00,           /* iInterface: */

        /*Endpoint OUT Descriptor*/
        0x07,           /* bLength: Endpoint Descriptor size */
        0x05,           /* bDescriptorType: Endpoint */
        0x01,           /* bEndpointAddress */
        0x02,           /* bmAttributes: Bulk */
        loByte(CdcMaxPacketSize), hiByte(CdcMaxPacketSize),  /* wMaxPacketSize: */
        0x00,           /* bInterval: ignore for Bulk transfer */

        /*Endpoint IN Descriptor*/
        0x07,           /* bLength: Endpoint Descriptor size */
        0x05,           /* bDescriptorType: Endpoint */
        0x81,           /* bEndpointAddress */
        0x02,           /* bmAttributes: Bulk */
        loByte(CdcMaxPacketSize), hiByte(CdcMaxPacketSize), /* wMaxPacketSize: */
        0x00            /* bInterval: ignore for Bulk transfer */
    };

    /* Language string descriptor */
    static constexpr uint8_t languageStringDescriptor[LangDescriptorLength] = {
        LangDescriptorLength,   /* USB_LEN_LANGID_STR_DESC */
        0x03,                   /* USB_DESC_TYPE_STRING */
        loByte(LangIdString),
        hiByte(LangIdString)
    };

    /* Manufactor string descriptor */
    static constexpr uint8_t manufactorStringDescriptor[MfcDescriptorLength] = {
        MfcDescriptorLength,				 
        0x03,	                /* USB_DESC_TYPE_STRING */
        'S', 0x00,
        'T', 0x00, 
        'M', 0x00, 
        'i', 0x00, 
        'c', 0x00, 
        'r', 0x00, 
        'o', 0x00, 
        'e', 0x00, 
        'l', 0x00, 
        'e', 0x00, 
        'c', 0x00, 
        't', 0x00, 
        'r', 0x00, 
        'o', 0x00,			
        'n', 0x00, 
        'i', 0x00, 
        'c', 0x00, 
        's', 0x00
    };

    /* Product string descriptor */
    static constexpr uint8_t productStringDescriptor[ProductDescriptorLength] = {
        ProductDescriptorLength, 
        0x03,                   /* USB_DESC_TYPE_STRING */
        'S', 0x00, 
        'T', 0x00, 
        'M', 0x00, 
        '3', 0x00, 
        '2', 0x00, 
        ' ', 0x00, 
        'V', 0x00, 
        'i', 0x00, 
        'r', 0x00, 
        't', 0x00, 
        'u', 0x00, 
        'a', 0x00, 
        'l', 0x00, 
        ' ', 0x00,
        'C', 0x00, 
        'o', 0x00, 
        'm', 0x00, 
        'P', 0x00, 
        'o', 0x00, 
        'r', 0x00, 
        't', 0x00
    };

    /* Serial number string descriptor */
    static constexpr uint8_t serialNumberStringDescriptor[SerialDescriptorLength] = {
        SerialDescriptorLength,	 
        0x03,               /* USB_DESC_TYPE_STRING */
        0x34, 0x00,
        0x38, 0x00, 
        0x00, 0x00, 
        0x45, 0x00, 
        0x37, 0x00, 
        0x34, 0x00, 
        0x46, 0x00, 
        0x37, 0x00, 
        0x36, 0x00, 
        0x33, 0x00, 
        0x30, 0x00, 
        0x38, 0x00
    };

    /* Device qualifier string descriptor */
    static constexpr uint8_t deviceQualifierDescriptor[DeviceQualifierLength] = {
        DeviceQualifierLength,
        0x06,               /* Device Qualifier */
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x40,
        0x01,
        0x00
    };

    static constexpr uint8_t stringInterface[InterfaceStringLength] = {
        InterfaceStringLength, 
        0x03,               /* USB_DESC_TYPE_STRING */
        'C', 0x00, 
        'D', 0x00, 
        'C', 0x00, 
        ' ', 0x00, 
        'I', 0x00, 
        'n', 0x00, 
        't', 0x00, 
        'e', 0x00, 
        'r', 0x00, 
        'f', 0x00, 
        'a', 0x00, 
        'c', 0x00, 
        'e', 0x00
    };

    static constexpr uint8_t configurationStringDescriptor[ConfigStringLength] = {
        ConfigStringLength, 
        0x03,               /* USB_DESC_TYPE_STRING */
        'C', 0x00, 
        'D', 0x00, 
        'C', 0x00, 
        ' ', 0x00, 
        'C', 0x00, 
        'o', 0x00, 
        'n', 0x00, 
        'f', 0x00, 
        'i', 0x00, 
        'g', 0x00
    };

};
}