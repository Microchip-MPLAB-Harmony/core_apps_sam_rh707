/*******************************************************************************
  FLEXCOM1 USART PLIB

  Company:
    Microchip Technology Inc.

  File Name:
    plib_flexcom1_usart.c

  Summary:
    FLEXCOM1 USART PLIB Implementation File

  Description
    This file defines the interface to the FLEXCOM1 USART peripheral library. This
    library provides access to and control of the associated peripheral
    instance.

  Remarks:
    None.
*******************************************************************************/

/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
/* This section lists the other files that are included in this file.
*/
#include "plib_flexcom1_usart.h"
#include "interrupts.h"

#define FLEXCOM1_USART_HW_RX_FIFO_THRES                 4
#define FLEXCOM1_USART_HW_TX_FIFO_THRES                 4

// *****************************************************************************
// *****************************************************************************
// Section: FLEXCOM1 USART Implementation
// *****************************************************************************
// *****************************************************************************
FLEXCOM_USART_OBJECT flexcom1UsartObj;

void static FLEXCOM1_USART_ErrorClear( void )
{
    uint16_t dummyData = 0;

    if (FLEXCOM1_REGS->FLEX_US_CSR & (FLEX_US_CSR_OVRE_Msk | FLEX_US_CSR_FRAME_Msk | FLEX_US_CSR_PARE_Msk))
    {
        /* Clear the error flags */
        FLEXCOM1_REGS->FLEX_US_CR = FLEX_US_CR_RSTSTA_Msk;

        /* Flush existing error bytes from the RX FIFO */
        while( FLEXCOM1_REGS->FLEX_US_CSR & FLEX_US_CSR_RXRDY_Msk )
        {
            if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
            {
                dummyData = *((uint16_t*)&FLEXCOM1_REGS->FLEX_US_RHR) & FLEX_US_RHR_RXCHR_Msk;
            }
            else
            {
                dummyData = *((uint8_t*)&FLEXCOM1_REGS->FLEX_US_RHR);
            }
        }
    }

    /* Ignore the warning */
    (void)dummyData;

    return;
}

void static FLEXCOM1_USART_ISR_RX_Handler( void )
{
    uint32_t rxPending = 0;
    uint32_t rxThreshold = 0;
    if(flexcom1UsartObj.rxBusyStatus == true)
    {
        while( (FLEXCOM1_REGS->FLEX_US_CSR & FLEX_US_CSR_RXRDY_Msk) && (flexcom1UsartObj.rxProcessedSize < flexcom1UsartObj.rxSize) )
        {
            if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
            {
                ((uint16_t*)flexcom1UsartObj.rxBuffer)[flexcom1UsartObj.rxProcessedSize++] = *((uint16_t*)&FLEXCOM1_REGS->FLEX_US_RHR) & FLEX_US_RHR_RXCHR_Msk;
            }
            else
            {
                flexcom1UsartObj.rxBuffer[flexcom1UsartObj.rxProcessedSize++] = *((uint8_t*)&FLEXCOM1_REGS->FLEX_US_RHR);
            }
        }

        rxPending = flexcom1UsartObj.rxSize - flexcom1UsartObj.rxProcessedSize;
        if (rxPending > 0)
        {
            rxThreshold = (FLEXCOM1_REGS->FLEX_US_FMR & FLEX_US_FMR_RXFTHRES_Msk) >> FLEX_US_FMR_RXFTHRES_Pos;
            if (rxPending < rxThreshold)
            {
                FLEXCOM1_REGS->FLEX_US_FMR = (FLEXCOM1_REGS->FLEX_US_FMR & ~FLEX_US_FMR_RXFTHRES_Msk) | FLEX_US_FMR_RXFTHRES(rxPending);
            }
        }

        /* Check if the buffer is done */
        if(flexcom1UsartObj.rxProcessedSize >= flexcom1UsartObj.rxSize)
        {
            flexcom1UsartObj.rxBusyStatus = false;

            /* Disable Read, Overrun, Parity and Framing error interrupts */
            FLEXCOM1_REGS->FLEX_US_FIDR = FLEX_US_FIDR_RXFTHF_Msk;
            FLEXCOM1_REGS->FLEX_US_IDR = (FLEX_US_IDR_FRAME_Msk | FLEX_US_IDR_PARE_Msk | FLEX_US_IDR_OVRE_Msk);

            if(flexcom1UsartObj.rxCallback != NULL)
            {
                flexcom1UsartObj.rxCallback(flexcom1UsartObj.rxContext);
            }
        }
    }
    else
    {
        /* Nothing to process */
        ;
    }

    return;
}

void static FLEXCOM1_USART_ISR_TX_Handler( void )
{
    if(flexcom1UsartObj.txBusyStatus == true)
    {
        while( (FLEXCOM1_REGS->FLEX_US_CSR & FLEX_US_CSR_TXRDY_Msk) && (flexcom1UsartObj.txProcessedSize < flexcom1UsartObj.txSize))
        {
            if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
            {
                *((uint16_t*)&FLEXCOM1_REGS->FLEX_US_THR) =  ((uint16_t*)flexcom1UsartObj.txBuffer)[flexcom1UsartObj.txProcessedSize++] & FLEX_US_THR_TXCHR_Msk;
            }
            else
            {
                *((uint8_t*)&FLEXCOM1_REGS->FLEX_US_THR) =  ((uint8_t*)flexcom1UsartObj.txBuffer)[flexcom1UsartObj.txProcessedSize++];
            }
        }

        /* Check if the buffer is done */
        if(flexcom1UsartObj.txProcessedSize >= flexcom1UsartObj.txSize)
        {
            if (FLEXCOM1_REGS->FLEX_US_CSR & FLEX_US_CSR_TXEMPTY_Msk)
            {
                flexcom1UsartObj.txBusyStatus = false;

                FLEXCOM1_REGS->FLEX_US_FIDR = FLEX_US_FIDR_TXFTHF_Msk;

                FLEXCOM1_REGS->FLEX_US_IDR = FLEX_US_IDR_TXEMPTY_Msk;

                if(flexcom1UsartObj.txCallback != NULL)
                {
                    flexcom1UsartObj.txCallback(flexcom1UsartObj.txContext);
                }
            }
            else
            {
                FLEXCOM1_REGS->FLEX_US_FIDR = FLEX_US_FIDR_TXFTHF_Msk;

                FLEXCOM1_REGS->FLEX_US_IER = FLEX_US_IER_TXEMPTY_Msk;
            }
        }
    }
    else
    {
        /* Nothing to process */
        ;
    }

    return;
}

void FLEXCOM1_InterruptHandler( void )
{
    /* Channel status */
    uint32_t channelStatus = FLEXCOM1_REGS->FLEX_US_CSR;
    uint32_t interruptMask = FLEXCOM1_REGS->FLEX_US_IMR;

    /* Error status */
    uint32_t errorStatus = (channelStatus & (FLEX_US_CSR_OVRE_Msk | FLEX_US_CSR_FRAME_Msk | FLEX_US_CSR_PARE_Msk));

    if((errorStatus != 0) && (interruptMask & (FLEX_US_IMR_RXRDY_Msk | FLEX_US_IMR_FRAME_Msk | FLEX_US_IMR_PARE_Msk | FLEX_US_IMR_OVRE_Msk)))
    {
        /* Save error to report it later */
        flexcom1UsartObj.errorStatus = (FLEXCOM_USART_ERROR)errorStatus;

        /* Clear error flags and flush the error data */
        FLEXCOM1_USART_ErrorClear();

        /* Transfer complete. Clear the busy flag. */
        flexcom1UsartObj.rxBusyStatus = false;

        FLEXCOM1_REGS->FLEX_US_FIDR = FLEX_US_FIDR_RXFTHF_Msk;

        /* Disable Read, Overrun, Parity and Framing error interrupts */
        FLEXCOM1_REGS->FLEX_US_IDR = (FLEX_US_IDR_RXRDY_Msk | FLEX_US_IDR_FRAME_Msk | FLEX_US_IDR_PARE_Msk | FLEX_US_IDR_OVRE_Msk);

        /* USART errors are normally associated with the receiver, hence calling receiver context */
        if( flexcom1UsartObj.rxCallback != NULL )
        {
            flexcom1UsartObj.rxCallback(flexcom1UsartObj.rxContext);
        }
    }


    /* Clear the FIFO related interrupt flags */
    FLEXCOM1_REGS->FLEX_US_CR = FLEX_US_CR_RSTSTA_Msk;

    FLEXCOM1_USART_ISR_RX_Handler();

    FLEXCOM1_USART_ISR_TX_Handler();

}


void FLEXCOM1_USART_Initialize( void )
{
    /* Set FLEXCOM USART operating mode */
    FLEXCOM1_REGS->FLEX_MR = FLEX_MR_OPMODE_USART;

    /* Reset FLEXCOM1 USART */
    FLEXCOM1_REGS->FLEX_US_CR = (FLEX_US_CR_RSTRX_Msk | FLEX_US_CR_RSTTX_Msk | FLEX_US_CR_RSTSTA_Msk  | FLEX_US_CR_FIFOEN_Msk );

    FLEXCOM1_REGS->FLEX_US_FMR = FLEX_US_FMR_TXFTHRES(FLEXCOM1_USART_HW_TX_FIFO_THRES) | FLEX_US_FMR_RXFTHRES(FLEXCOM1_USART_HW_RX_FIFO_THRES) ;

    /* Setup transmitter timeguard register */
    FLEXCOM1_REGS->FLEX_US_TTGR = 0;

    /* Configure FLEXCOM1 USART mode */
    FLEXCOM1_REGS->FLEX_US_MR = ( FLEX_US_MR_USART_MODE_NORMAL | FLEX_US_MR_USCLKS_MCK | FLEX_US_MR_CHRL_8_BIT | FLEX_US_MR_PAR_NO | FLEX_US_MR_NBSTOP_1_BIT | (0 << FLEX_US_MR_OVER_Pos));

    /* Configure FLEXCOM1 USART Baud Rate */
    FLEXCOM1_REGS->FLEX_US_BRGR = FLEX_US_BRGR_CD(27) | FLEX_US_BRGR_FP(1);

    /* Enable FLEXCOM1 USART */
    FLEXCOM1_REGS->FLEX_US_CR = (FLEX_US_CR_TXEN_Msk | FLEX_US_CR_RXEN_Msk);


    /* Initialize instance object */
    flexcom1UsartObj.rxBuffer = NULL;
    flexcom1UsartObj.rxSize = 0;
    flexcom1UsartObj.rxProcessedSize = 0;
    flexcom1UsartObj.rxBusyStatus = false;
    flexcom1UsartObj.rxCallback = NULL;
    flexcom1UsartObj.errorStatus = FLEXCOM_USART_ERROR_NONE;
    flexcom1UsartObj.txBuffer = NULL;
    flexcom1UsartObj.txSize = 0;
    flexcom1UsartObj.txProcessedSize = 0;
    flexcom1UsartObj.txBusyStatus = false;
    flexcom1UsartObj.txCallback = NULL;

    return;
}

FLEXCOM_USART_ERROR FLEXCOM1_USART_ErrorGet( void )
{
    FLEXCOM_USART_ERROR errorStatus = flexcom1UsartObj.errorStatus;

    flexcom1UsartObj.errorStatus = FLEXCOM_USART_ERROR_NONE;

    /* All errors are cleared, but send the previous error state */
    return errorStatus;
}

static void FLEXCOM1_USART_BaudCalculate(uint32_t srcClkFreq, uint32_t reqBaud, uint8_t overSamp, uint32_t* cd, uint32_t* fp, uint32_t* baudError)
{
    uint32_t actualBaud = 0;

    *cd = srcClkFreq / (reqBaud * 8 * (2 - overSamp));

    if (*cd > 0)
    {
        *fp = ((srcClkFreq / (reqBaud * (2 - overSamp))) - ((*cd) * 8));
        actualBaud = (srcClkFreq / (((*cd) * 8) + (*fp))) / (2 - overSamp);
        *baudError = ((100 * actualBaud)/reqBaud) - 100;
    }
}

bool FLEXCOM1_USART_SerialSetup( FLEXCOM_USART_SERIAL_SETUP *setup, uint32_t srcClkFreq )
{
    uint32_t baud = 0;
    uint32_t overSampVal = 0;
    uint32_t usartMode;
    uint32_t cd0, fp0, cd1, fp1, baudError0, baudError1;
    bool status = false;

    cd0 = fp0 = cd1 = fp1 = baudError0 = baudError1 = 0;

    if((flexcom1UsartObj.rxBusyStatus == true) || (flexcom1UsartObj.txBusyStatus == true))
    {
        /* Transaction is in progress, so return without updating settings */
        return false;
    }

    if (setup != NULL)
    {
        baud = setup->baudRate;

        if(srcClkFreq == 0)
        {
            srcClkFreq = FLEXCOM1_USART_FrequencyGet();
        }

        /* Calculate baud register values for 8x/16x oversampling values */

        FLEXCOM1_USART_BaudCalculate(srcClkFreq, baud, 0, &cd0, &fp0, &baudError0);
        FLEXCOM1_USART_BaudCalculate(srcClkFreq, baud, 1, &cd1, &fp1, &baudError1);

        if ( !(cd0 > 0 && cd0 <= 65535) && !(cd1 > 0 && cd1 <= 65535) )
        {
            /* Requested baud cannot be generated with current clock settings */
            return status;
        }

        if ( (cd0 > 0 && cd0 <= 65535) && (cd1 > 0 && cd1 <= 65535) )
        {
            /* Requested baud can be generated with both 8x and 16x oversampling. Select the one with less % error. */
            if (baudError1 < baudError0)
            {
                cd0 = cd1;
                fp0 = fp1;
                overSampVal = (1 << FLEX_US_MR_OVER_Pos) & FLEX_US_MR_OVER_Msk;
            }
        }
        else
        {
            /* Requested baud can be generated with either with 8x oversampling or with 16x oversampling. Select valid one. */
            if (cd1 > 0 && cd1 <= 65535)
            {
                cd0 = cd1;
                fp0 = fp1;
                overSampVal = (1 << FLEX_US_MR_OVER_Pos) & FLEX_US_MR_OVER_Msk;
            }
        }

        /* Configure FLEXCOM1 USART mode */
        usartMode = FLEXCOM1_REGS->FLEX_US_MR;
        usartMode &= ~(FLEX_US_MR_CHRL_Msk | FLEX_US_MR_MODE9_Msk | FLEX_US_MR_PAR_Msk | FLEX_US_MR_NBSTOP_Msk | FLEX_US_MR_OVER_Msk);
        FLEXCOM1_REGS->FLEX_US_MR = usartMode | ((uint32_t)setup->dataWidth | (uint32_t)setup->parity | (uint32_t)setup->stopBits | overSampVal);

        /* Configure FLEXCOM1 USART Baud Rate */
        FLEXCOM1_REGS->FLEX_US_BRGR = FLEX_US_BRGR_CD(cd0) | FLEX_US_BRGR_FP(fp0);
        status = true;
    }

    return status;
}

bool FLEXCOM1_USART_Read( void *buffer, const size_t size )
{
    bool status = false;
    uint8_t* pBuffer = (uint8_t *)buffer;

    if(pBuffer != NULL)
    {
        /* Check if receive request is in progress */
        if(flexcom1UsartObj.rxBusyStatus == false)
        {
            /* Clear errors that may have got generated when there was no active read request pending */
            FLEXCOM1_USART_ErrorClear();

            /* Clear the errors related to pervious read requests */
            flexcom1UsartObj.errorStatus = FLEXCOM_USART_ERROR_NONE;

            flexcom1UsartObj.rxBuffer = pBuffer;
            flexcom1UsartObj.rxSize = size;
            flexcom1UsartObj.rxProcessedSize = 0;
            flexcom1UsartObj.rxBusyStatus = true;
            status = true;


            /* Clear RX FIFO */
            FLEXCOM1_REGS->FLEX_US_CR = FLEX_US_CR_RXFCLR_Msk;

            if (flexcom1UsartObj.rxSize < FLEXCOM1_USART_HW_RX_FIFO_THRES)
            {
                FLEXCOM1_REGS->FLEX_US_FMR = (FLEXCOM1_REGS->FLEX_US_FMR & ~FLEX_US_FMR_RXFTHRES_Msk) | FLEX_US_FMR_RXFTHRES(flexcom1UsartObj.rxSize);
            }
            else
            {
                FLEXCOM1_REGS->FLEX_US_FMR = (FLEXCOM1_REGS->FLEX_US_FMR & ~FLEX_US_FMR_RXFTHRES_Msk) | FLEX_US_FMR_RXFTHRES(FLEXCOM1_USART_HW_RX_FIFO_THRES);
            }

            /* Enable Read, Overrun, Parity and Framing error interrupts */
            FLEXCOM1_REGS->FLEX_US_IER = (FLEX_US_IER_FRAME_Msk | FLEX_US_IER_PARE_Msk | FLEX_US_IER_OVRE_Msk);

            /* Enable RX FIFO Threshold interrupt */
            FLEXCOM1_REGS->FLEX_US_FIER = FLEX_US_FIER_RXFTHF_Msk;

        }
    }

    return status;
}

bool FLEXCOM1_USART_Write( void *buffer, const size_t size )
{
    bool status = false;
    uint8_t* pBuffer = (uint8_t *)buffer;

    if(pBuffer != NULL)
    {
        /* Check if transmit request is in progress */
        if(flexcom1UsartObj.txBusyStatus == false)
        {
            flexcom1UsartObj.txBuffer = (uint8_t*)pBuffer;
            flexcom1UsartObj.txSize = size;
            flexcom1UsartObj.txProcessedSize = 0;
            flexcom1UsartObj.txBusyStatus = true;
            status = true;

            /* Initiate the transfer by sending first byte */
            while( (FLEXCOM1_REGS->FLEX_US_CSR & FLEX_US_CSR_TXRDY_Msk) && (flexcom1UsartObj.txProcessedSize < flexcom1UsartObj.txSize) )
            {
                if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
                {
                    *((uint16_t*)&FLEXCOM1_REGS->FLEX_US_THR) =  ((uint16_t*)flexcom1UsartObj.txBuffer)[flexcom1UsartObj.txProcessedSize++] & FLEX_US_THR_TXCHR_Msk;
                }
                else
                {
                    *((uint8_t*)&FLEXCOM1_REGS->FLEX_US_THR) =  ((uint8_t*)flexcom1UsartObj.txBuffer)[flexcom1UsartObj.txProcessedSize++];
                }
            }

            if ( flexcom1UsartObj.txProcessedSize >= flexcom1UsartObj.txSize)
            {
                FLEXCOM1_REGS->FLEX_US_IER = FLEX_US_IER_TXEMPTY_Msk;
            }
            else
            {
                FLEXCOM1_REGS->FLEX_US_FIER = FLEX_US_FIER_TXFTHF_Msk;
            }
        }
    }

    return status;
}

void FLEXCOM1_USART_WriteCallbackRegister( FLEXCOM_USART_CALLBACK callback, uintptr_t context )
{
    flexcom1UsartObj.txCallback = callback;
    flexcom1UsartObj.txContext = context;
}

void FLEXCOM1_USART_ReadCallbackRegister( FLEXCOM_USART_CALLBACK callback, uintptr_t context )
{
    flexcom1UsartObj.rxCallback = callback;
    flexcom1UsartObj.rxContext = context;
}

bool FLEXCOM1_USART_WriteIsBusy( void )
{
    return flexcom1UsartObj.txBusyStatus;
}

bool FLEXCOM1_USART_ReadIsBusy( void )
{
    return flexcom1UsartObj.rxBusyStatus;
}

size_t FLEXCOM1_USART_WriteCountGet( void )
{
    return flexcom1UsartObj.txProcessedSize;
}

size_t FLEXCOM1_USART_ReadCountGet( void )
{
    return flexcom1UsartObj.rxProcessedSize;
}

bool FLEXCOM1_USART_ReadAbort(void)
{
    if (flexcom1UsartObj.rxBusyStatus == true)
    {
        /* Disable Read, Overrun, Parity and Framing error interrupts */
        FLEXCOM1_REGS->FLEX_US_IDR = (FLEX_US_IDR_RXRDY_Msk | FLEX_US_IDR_FRAME_Msk | FLEX_US_IDR_PARE_Msk | FLEX_US_IDR_OVRE_Msk);

        flexcom1UsartObj.rxBusyStatus = false;

        /* If required application should read the num bytes processed prior to calling the read abort API */
        flexcom1UsartObj.rxSize = flexcom1UsartObj.rxProcessedSize = 0;
    }

    return true;
}


bool FLEXCOM1_USART_TransmitComplete( void )
{
    bool status = false;

    if (FLEXCOM1_REGS->FLEX_US_CSR & FLEX_US_CSR_TXEMPTY_Msk)
    {
        status = true;
    }

    return status;
}

