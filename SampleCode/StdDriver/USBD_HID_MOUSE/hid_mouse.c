/**************************************************************************//**
 * @file     hid_mouse.c
 * @version  V1.00
 * $Date: 14/11/17 5:36p $
 * @brief    NUC505 USBD driver Sample file
 *
 * @note
 * Copyright (C) 2013 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include <string.h>
#include "NUC505Series.h"
#include "hid_mouse.h"

signed char mouse_table[] = {-16, -16, -16, 0, 16, 16, 16, 0};
uint8_t mouse_idx = 0;
uint8_t move_len, mouse_mode = 1;

uint8_t volatile g_u8EPAReady = 0;

volatile uint8_t g_hid_report = 0;
volatile uint8_t g_hid_protocol = 0;
volatile uint8_t g_hid_idle = 0;

#ifdef __ICCARM__
#pragma data_alignment=32
uint8_t buf[4] ;       /* Working buffer */
#else
uint8_t buf[4] __attribute__((aligned(32)));       /* Working buffer */
#endif

void USBD_IRQHandler(void)
{
    __IO uint32_t IrqStL, IrqSt;

    IrqStL = USBD->GINTSTS & USBD->GINTEN;    /* get interrupt status */

    if (!IrqStL)    return;

    /* USB interrupt */
    if (IrqStL & USBD_GINTSTS_USBIF_Msk) {
        IrqSt = USBD->BUSINTSTS & USBD->BUSINTEN;

        if (IrqSt & USBD_BUSINTSTS_SOFIF_Msk)
            USBD_CLR_BUS_INT_FLAG(USBD_BUSINTSTS_SOFIF_Msk);

        if (IrqSt & USBD_BUSINTSTS_RSTIF_Msk) {
            USBD_SwReset();

            USBD_ResetDMA();
            USBD->EP[EPA].EPRSPCTL = USBD_EPRSPCTL_FLUSH_Msk;
            USBD->EP[EPB].EPRSPCTL = USBD_EPRSPCTL_FLUSH_Msk;

            if (USBD->OPER & 0x04)  /* high speed */
                HID_InitForHighSpeed();
            else                    /* full speed */
                HID_InitForFullSpeed();
            USBD_ENABLE_CEP_INT(USBD_CEPINTEN_SETUPPKIEN_Msk);
            USBD_SET_ADDR(0);
            USBD_ENABLE_BUS_INT(USBD_BUSINTEN_RSTIEN_Msk|USBD_BUSINTEN_RESUMEIEN_Msk|USBD_BUSINTEN_SUSPENDIEN_Msk);
            USBD_CLR_BUS_INT_FLAG(USBD_BUSINTSTS_RSTIF_Msk);
            USBD_CLR_CEP_INT_FLAG(0x1ffc);
        }

        if (IrqSt & USBD_BUSINTSTS_RESUMEIF_Msk) {
            USBD_ENABLE_BUS_INT(USBD_BUSINTEN_RSTIEN_Msk|USBD_BUSINTEN_SUSPENDIEN_Msk);
            USBD_CLR_BUS_INT_FLAG(USBD_BUSINTSTS_RESUMEIF_Msk);
        }

        if (IrqSt & USBD_BUSINTSTS_SUSPENDIF_Msk) {
            USBD_ENABLE_BUS_INT(USBD_BUSINTEN_RSTIEN_Msk | USBD_BUSINTEN_RESUMEIEN_Msk);
            USBD_CLR_BUS_INT_FLAG(USBD_BUSINTSTS_SUSPENDIF_Msk);
        }

        if (IrqSt & USBD_BUSINTSTS_HISPDIF_Msk) {
            USBD_ENABLE_CEP_INT(USBD_CEPINTEN_SETUPPKIEN_Msk);
            USBD_CLR_BUS_INT_FLAG(USBD_BUSINTSTS_HISPDIF_Msk);
        }

        if (IrqSt & USBD_BUSINTSTS_DMADONEIF_Msk) {
            g_usbd_DmaDone = 1;
            USBD_CLR_BUS_INT_FLAG(USBD_BUSINTSTS_DMADONEIF_Msk);

            if (USBD->DMACTL & USBD_DMACTL_DMARD_Msk) {
                if (g_usbd_ShortPacket == 1) {
                    USBD->EP[EPA].EPRSPCTL = USBD->EP[EPA].EPRSPCTL & 0x10 | USB_EP_RSPCTL_SHORTTXEN;    /* packet end */
                    g_usbd_ShortPacket = 0;
                }
            }
        }

        if (IrqSt & USBD_BUSINTSTS_PHYCLKVLDIF_Msk)
            USBD_CLR_BUS_INT_FLAG(USBD_BUSINTSTS_PHYCLKVLDIF_Msk);

        if (IrqSt & USBD_BUSINTSTS_VBUSDETIF_Msk) {
            if (USBD_IS_ATTACHED()) {
                /* USB Plug In */
                USBD_ENABLE_USB();
            } else {
                /* USB Un-plug */
                USBD_DISABLE_USB();
            }
            USBD_CLR_BUS_INT_FLAG(USBD_BUSINTSTS_VBUSDETIF_Msk);
        }
    }

    if (IrqStL & USBD_GINTSTS_CEPIF_Msk) {
        IrqSt = USBD->CEPINTSTS & USBD->CEPINTEN;

        if (IrqSt & USBD_CEPINTSTS_SETUPTKIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_SETUPTKIF_Msk);
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_SETUPPKIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_SETUPPKIF_Msk);
            USBD_ProcessSetupPacket();
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_OUTTKIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_OUTTKIF_Msk);
            USBD_ENABLE_CEP_INT(USBD_CEPINTEN_STSDONEIEN_Msk);
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_INTKIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_INTKIF_Msk);
            if (!(IrqSt & USBD_CEPINTSTS_STSDONEIF_Msk)) {
                USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_TXPKIF_Msk);
                USBD_ENABLE_CEP_INT(USBD_CEPINTEN_TXPKIEN_Msk);
                USBD_CtrlIn();
            } else {
                USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_TXPKIF_Msk);
                USBD_ENABLE_CEP_INT(USBD_CEPINTEN_TXPKIEN_Msk|USBD_CEPINTEN_STSDONEIEN_Msk);
            }
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_PINGIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_PINGIF_Msk);
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_TXPKIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_STSDONEIF_Msk);
            USBD_SET_CEP_STATE(USB_CEPCTL_NAKCLR);
            if (g_usbd_CtrlInSize) {
                USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_INTKIF_Msk);
                USBD_ENABLE_CEP_INT(USBD_CEPINTEN_INTKIEN_Msk);
            } else {
                if (g_usbd_CtrlZero == 1)
                    USBD_SET_CEP_STATE(USB_CEPCTL_ZEROLEN);
                USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_STSDONEIF_Msk);
                USBD_ENABLE_CEP_INT(USBD_CEPINTEN_SETUPPKIEN_Msk|USBD_CEPINTEN_STSDONEIEN_Msk);
            }
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_TXPKIF_Msk);
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_RXPKIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_RXPKIF_Msk);
            USBD_SET_CEP_STATE(USB_CEPCTL_NAKCLR);
            USBD_ENABLE_CEP_INT(USBD_CEPINTEN_SETUPPKIEN_Msk|USBD_CEPINTEN_STSDONEIEN_Msk);
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_NAKIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_NAKIF_Msk);
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_STALLIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_STALLIF_Msk);
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_ERRIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_ERRIF_Msk);
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_STSDONEIF_Msk) {
            USBD_UpdateDeviceState();
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_STSDONEIF_Msk);
            USBD_ENABLE_CEP_INT(USBD_CEPINTEN_SETUPPKIEN_Msk);
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_BUFFULLIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_BUFFULLIF_Msk);
            return;
        }

        if (IrqSt & USBD_CEPINTSTS_BUFEMPTYIF_Msk) {
            USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_BUFEMPTYIF_Msk);
            return;
        }
    }

    /* interrupt in */
    if (IrqStL & USBD_GINTSTS_EPAIF_Msk) {
        IrqSt = USBD->EP[EPA].EPINTSTS & USBD->EP[EPA].EPINTEN;

        USBD_ENABLE_EP_INT(EPA, 0);
        EPA_Handler();
        USBD_CLR_EP_INT_FLAG(EPA, IrqSt);
    }
    /* interrupt out */
    if (IrqStL & USBD_GINTSTS_EPBIF_Msk) {
        IrqSt = USBD->EP[EPB].EPINTSTS & USBD->EP[EPB].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPB, IrqSt);
    }

    if (IrqStL & USBD_GINTSTS_EPCIF_Msk) {
        IrqSt = USBD->EP[EPC].EPINTSTS & USBD->EP[EPC].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPC, IrqSt);
    }

    if (IrqStL & USBD_GINTSTS_EPDIF_Msk) {
        IrqSt = USBD->EP[EPD].EPINTSTS & USBD->EP[EPD].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPD, IrqSt);
    }

    if (IrqStL & USBD_GINTSTS_EPEIF_Msk) {
        IrqSt = USBD->EP[EPE].EPINTSTS & USBD->EP[EPE].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPE, IrqSt);
    }

    if (IrqStL & USBD_GINTSTS_EPFIF_Msk) {
        IrqSt = USBD->EP[EPF].EPINTSTS & USBD->EP[EPF].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPF, IrqSt);
    }

    if (IrqStL & USBD_GINTSTS_EPGIF_Msk) {
        IrqSt = USBD->EP[EPG].EPINTSTS & USBD->EP[EPG].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPG, IrqSt);
    }

    if (IrqStL & USBD_GINTSTS_EPHIF_Msk) {
        IrqSt = USBD->EP[EPH].EPINTSTS & USBD->EP[EPH].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPH, IrqSt);
    }

    if (IrqStL & USBD_GINTSTS_EPIIF_Msk) {
        IrqSt = USBD->EP[EPI].EPINTSTS & USBD->EP[EPI].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPI, IrqSt);
    }

    if (IrqStL & USBD_GINTSTS_EPJIF_Msk) {
        IrqSt = USBD->EP[EPJ].EPINTSTS & USBD->EP[EPJ].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPJ, IrqSt);
    }

    if (IrqStL & USBD_GINTSTS_EPKIF_Msk) {
        IrqSt = USBD->EP[EPK].EPINTSTS & USBD->EP[EPK].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPK, IrqSt);
    }

    if (IrqStL & USBD_GINTSTS_EPLIF_Msk) {
        IrqSt = USBD->EP[EPL].EPINTSTS & USBD->EP[EPL].EPINTEN;
        USBD_CLR_EP_INT_FLAG(EPL, IrqSt);
    }
}

void EPA_Handler(void)  /* Interrupt IN handler */
{
    g_u8EPAReady = 1;
}


/*--------------------------------------------------------------------------*/
/**
  * @brief  USBD Endpoint Config.
  * @param  None.
  * @retval None.
  */
void HID_InitForHighSpeed(void)
{
    /* EPA ==> Interrupt IN endpoint, address 1 */
    USBD_SetEpBufAddr(EPA, EPA_BUF_BASE, EPA_BUF_LEN);
    USBD_SET_MAX_PAYLOAD(EPA, EPA_MAX_PKT_SIZE);
    USBD_ConfigEp(EPA, INT_IN_EP_NUM, USB_EP_CFG_TYPE_INT, USB_EP_CFG_DIR_IN);
}

void HID_InitForFullSpeed(void)
{
    /* EPA ==> Interrupt IN endpoint, address 1 */
    USBD_SetEpBufAddr(EPA, EPA_BUF_BASE, EPA_BUF_LEN);
    USBD_SET_MAX_PAYLOAD(EPA, EPA_OTHER_MAX_PKT_SIZE);
    USBD_ConfigEp(EPA, INT_IN_EP_NUM, USB_EP_CFG_TYPE_INT, USB_EP_CFG_DIR_IN);
}

void HID_Init(void)
{
    /* Configure USB controller */
    /* Enable USB BUS, CEP and EPA global interrupt */
    USBD_ENABLE_USB_INT(USBD_GINTEN_USBIEN_Msk|USBD_GINTEN_CEPIEN_Msk|USBD_GINTEN_EPAIEN_Msk);
    /* Enable BUS interrupt */
    USBD_ENABLE_BUS_INT(USBD_BUSINTEN_DMADONEIEN_Msk|USBD_BUSINTEN_RESUMEIEN_Msk|USBD_BUSINTEN_RSTIEN_Msk|USBD_BUSINTEN_VBUSDETIEN_Msk);
    /* Reset Address to 0 */
    USBD_SET_ADDR(0);

    /*****************************************************/
    /* Control endpoint */
    USBD_SetEpBufAddr(CEP, CEP_BUF_BASE, CEP_BUF_LEN);
    USBD_ENABLE_CEP_INT(USBD_CEPINTEN_SETUPPKIEN_Msk|USBD_CEPINTEN_STSDONEIEN_Msk);

    /*****************************************************/
    HID_InitForHighSpeed();

    /* start to IN data */
    g_u8EPAReady = 1;
}

void HID_ClassRequest(void)
{
    if (gUsbCmd.bmRequestType & 0x80) 
    {   /* request data transfer direction */
        /* Device to host */
        switch (gUsbCmd.bRequest) 
        {
            case GET_IDLE:
            {
                USBD_PrepareCtrlIn((uint8_t *)&g_hid_idle, 1);
                USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_INTKIF_Msk);
                USBD_ENABLE_CEP_INT(USBD_CEPINTEN_INTKIEN_Msk);
                break;
            }        
            case GET_PROTOCOL:
            {
                USBD_PrepareCtrlIn((uint8_t *)&g_hid_protocol, 1);
                USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_INTKIF_Msk);
                USBD_ENABLE_CEP_INT(USBD_CEPINTEN_INTKIEN_Msk);
                break;
            }            
            case GET_REPORT:
//            {
//                break;
//            }
            default: {
                /* Setup error, stall the device */
                USBD_SET_CEP_STATE(USBD_CEPCTL_STALLEN_Msk);
                break;
            }
        }
    }
    else 
    {
        /* Host to device */
        switch (gUsbCmd.bRequest) 
        {
            case SET_REPORT:
            {
                if (((gUsbCmd.wValue >> 8) & 0xff) == 3)
                {
                    /* Request Type = Feature */
                    USBD_CtrlOut((uint8_t*)&g_hid_report, gUsbCmd.wLength);
                    USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_STSDONEIF_Msk);
                    USBD_SET_CEP_STATE(USB_CEPCTL_NAKCLR);
                    USBD_ENABLE_CEP_INT(USBD_CEPINTEN_STSDONEIEN_Msk);
//                     printf("SET_REPORT %d\n",g_hid_report);
                }
                break;
            }
            case SET_IDLE: 
            {
                g_hid_idle = (gUsbCmd.wValue >> 8) & 0xff;
                /* Status stage */
                USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_STSDONEIF_Msk);
                USBD_SET_CEP_STATE(USB_CEPCTL_NAKCLR);
                USBD_ENABLE_CEP_INT(USBD_CEPINTEN_STSDONEIEN_Msk);
//                printf("Set Idle\n");
                break;
            }
            case SET_PROTOCOL:
            {
                g_hid_protocol = gUsbCmd.wValue;
                /* Status stage */
                USBD_CLR_CEP_INT_FLAG(USBD_CEPINTSTS_STSDONEIF_Msk);
                USBD_SET_CEP_STATE(USB_CEPCTL_NAKCLR);
                USBD_ENABLE_CEP_INT(USBD_CEPINTEN_STSDONEIEN_Msk);
//                printf("Set Protocol\n");
                break;
            }        
            default: {
                /* Stall */
                /* Setup error, stall the device */
                USBD_SET_CEP_STATE(USBD_CEPCTL_STALLEN_Msk);
                break;
            }
        }
    }
}

void HID_VendorRequest(void)
{
    if (gUsbCmd.bmRequestType & 0x80) 
    { /* request data transfer direction */
        /* Device to host */
        switch (gUsbCmd.bRequest) 
        {
            default: {
                /* Setup error, stall the device */
                USBD_SET_CEP_STATE(USBD_CEPCTL_STALLEN_Msk);
                break;
            }
        }
    }
    else 
    {
        /* Host to device */
        switch (gUsbCmd.bRequest) 
        {
            default: {
                /* Stall */
                /* Setup error, stall the device */
                USBD_SET_CEP_STATE(USBD_CEPCTL_STALLEN_Msk);
                break;
            }
        }
    }
}

void HID_UpdateMouseData(void)
{
    if (g_u8EPAReady) 
    {
        mouse_mode ^= 1;

        if (mouse_mode) 
        {
            if (move_len > 14)
            {
                /* Update new report data */
                buf[0] = 0x00;
                buf[1] = mouse_table[mouse_idx & 0x07];
                buf[2] = mouse_table[(mouse_idx + 2) & 0x07];
                buf[3] = 0x00;
                mouse_idx++;
                move_len = 0;
            }
        } 
        else 
        {
            buf[0] = buf[1] = buf[2] = buf[3] = 0;
        }
        move_len++;
        g_u8EPAReady = 0;
        /* Set DMA transfer length and trigger DMA transfer */
        USBD_SET_DMA_READ(INT_IN_EP_NUM);
        USBD_SET_DMA_ADDR((uint32_t)buf);
        USBD_SET_DMA_LEN(4);
        g_usbd_DmaDone = 0;

        USBD_ENABLE_DMA();
        while(1) 
        {
            if (!(USBD->DMACTL & USBD_DMACTL_DMAEN_Msk))
                break;
            else
                if (!USBD_IS_ATTACHED())
                    break;
        }
                
        /* Trigger Short Packet to transfer data */
        USBD->EP[EPA].EPRSPCTL = USB_EP_RSPCTL_SHORTTXEN;
        USBD_ENABLE_EP_INT(EPA, USBD_EPINTEN_INTKIEN_Msk);
    }
}


