/*******************************************************************************
  FLEXCOM1 USART PLIB

  Company:
    Microchip Technology Inc.

  File Name:
    plib_flexcom1_usart.c

  Summary:
    FLEXCOM1 USART PLIB Implementation File

  Description
    This file defines the interface to the FLEXCOM1 USART
    peripheral library. This library provides access to and control of the
    associated peripheral instance.

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

#include "plib_flexcom1_usart.h"
#include "interrupts.h"

#define FLEXCOM1_USART_READ_BUFFER_SIZE             16
#define FLEXCOM1_USART_READ_BUFFER_SIZE_9BIT        (16 >> 1)


/* Disable Read, Overrun, Parity and Framing error interrupts */
#define FLEXCOM1_USART_RX_INT_DISABLE()      FLEXCOM1_REGS->FLEX_US_IDR = (FLEX_US_IDR_RXRDY_Msk | FLEX_US_IDR_FRAME_Msk | FLEX_US_IDR_PARE_Msk | FLEX_US_IDR_OVRE_Msk)

/* Enable Read, Overrun, Parity and Framing error interrupts */
#define FLEXCOM1_USART_RX_INT_ENABLE()       FLEXCOM1_REGS->FLEX_US_IER = (FLEX_US_IER_RXRDY_Msk | FLEX_US_IER_FRAME_Msk | FLEX_US_IER_PARE_Msk | FLEX_US_IER_OVRE_Msk)

#define FLEXCOM1_USART_TX_INT_DISABLE()      FLEXCOM1_REGS->FLEX_US_IDR = FLEX_US_IDR_TXRDY_Msk
#define FLEXCOM1_USART_TX_INT_ENABLE()       FLEXCOM1_REGS->FLEX_US_IER = FLEX_US_IER_TXRDY_Msk

static uint8_t FLEXCOM1_USART_ReadBuffer[FLEXCOM1_USART_READ_BUFFER_SIZE];

#define FLEXCOM1_USART_WRITE_BUFFER_SIZE            512
#define FLEXCOM1_USART_WRITE_BUFFER_SIZE_9BIT       (512 >> 1)

static uint8_t FLEXCOM1_USART_WriteBuffer[FLEXCOM1_USART_WRITE_BUFFER_SIZE];

// *****************************************************************************
// *****************************************************************************
// Section: FLEXCOM1 USART Ring Buffer Implementation
// *****************************************************************************
// *****************************************************************************

FLEXCOM_USART_RING_BUFFER_OBJECT flexcom1UsartObj;

void FLEXCOM1_USART_Initialize( void )
{
    /* Set FLEXCOM USART operating mode */
    FLEXCOM1_REGS->FLEX_MR = FLEX_MR_OPMODE_USART;

    /* Reset FLEXCOM1 USART */
    FLEXCOM1_REGS->FLEX_US_CR = (FLEX_US_CR_RSTRX_Msk | FLEX_US_CR_RSTTX_Msk | FLEX_US_CR_RSTSTA_Msk);


    FLEXCOM1_REGS->FLEX_US_TTGR = 0;

    /* Enable FLEXCOM1 USART */
    FLEXCOM1_REGS->FLEX_US_CR = (FLEX_US_CR_TXEN_Msk | FLEX_US_CR_RXEN_Msk);

    /* Configure FLEXCOM1 USART mode */
    FLEXCOM1_REGS->FLEX_US_MR = ( FLEX_US_MR_USART_MODE_NORMAL | FLEX_US_MR_USCLKS_MCK | FLEX_US_MR_CHRL_8_BIT | FLEX_US_MR_PAR_NO | FLEX_US_MR_NBSTOP_1_BIT | (0 << FLEX_US_MR_OVER_Pos));

    /* Configure FLEXCOM1 USART Baud Rate */
    FLEXCOM1_REGS->FLEX_US_BRGR = FLEX_US_BRGR_CD(27) | FLEX_US_BRGR_FP(1);

    flexcom1UsartObj.rdCallback = NULL;
    flexcom1UsartObj.rdInIndex = 0;
    flexcom1UsartObj.rdOutIndex = 0;
    flexcom1UsartObj.isRdNotificationEnabled = false;
    flexcom1UsartObj.isRdNotifyPersistently = false;
    flexcom1UsartObj.rdThreshold = 0;
    flexcom1UsartObj.rdBufferSize = FLEXCOM1_USART_READ_BUFFER_SIZE;
    flexcom1UsartObj.wrCallback = NULL;
    flexcom1UsartObj.wrInIndex = 0;
    flexcom1UsartObj.wrOutIndex = 0;
    flexcom1UsartObj.isWrNotificationEnabled = false;
    flexcom1UsartObj.isWrNotifyPersistently = false;
    flexcom1UsartObj.wrThreshold = 0;
    flexcom1UsartObj.wrBufferSize = FLEXCOM1_USART_WRITE_BUFFER_SIZE;
    flexcom1UsartObj.errorStatus = FLEXCOM_USART_ERROR_NONE;

    if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
    {
        flexcom1UsartObj.rdBufferSize = FLEXCOM1_USART_READ_BUFFER_SIZE_9BIT;
        flexcom1UsartObj.wrBufferSize = FLEXCOM1_USART_WRITE_BUFFER_SIZE_9BIT;
    }
    else
    {
        flexcom1UsartObj.rdBufferSize = FLEXCOM1_USART_READ_BUFFER_SIZE;
        flexcom1UsartObj.wrBufferSize = FLEXCOM1_USART_WRITE_BUFFER_SIZE;
    }

    FLEXCOM1_USART_RX_INT_ENABLE();
}

void static FLEXCOM1_USART_ErrorClear( void )
{
    uint16_t dummyData = 0u;

    /* Clear the error flags */
    FLEXCOM1_REGS->FLEX_US_CR = FLEX_US_CR_RSTSTA_Msk;

    /* Flush existing error bytes from the RX FIFO */
    while( FLEX_US_CSR_RXRDY_Msk == (FLEXCOM1_REGS->FLEX_US_CSR& FLEX_US_CSR_RXRDY_Msk) )
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

    /* Ignore the warning */
    (void)dummyData;

    return;
}

FLEXCOM_USART_ERROR FLEXCOM1_USART_ErrorGet( void )
{
    FLEXCOM_USART_ERROR errors = flexcom1UsartObj.errorStatus;

    flexcom1UsartObj.errorStatus = FLEXCOM_USART_ERROR_NONE;

    /* All errors are cleared, but send the previous error state */
    return errors;
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

        if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
        {
            flexcom1UsartObj.rdBufferSize = FLEXCOM1_USART_READ_BUFFER_SIZE_9BIT;
            flexcom1UsartObj.wrBufferSize = FLEXCOM1_USART_WRITE_BUFFER_SIZE_9BIT;
        }
        else
        {
            flexcom1UsartObj.rdBufferSize = FLEXCOM1_USART_READ_BUFFER_SIZE;
            flexcom1UsartObj.wrBufferSize = FLEXCOM1_USART_WRITE_BUFFER_SIZE;
        }

        status = true;
    }

    return status;
}

/* This routine is only called from ISR. Hence do not disable/enable USART interrupts. */
static bool FLEXCOM1_USART_TxPullByte(uint16_t* pWrByte)
{
    bool isSuccess = false;
    uint32_t wrOutIndex = flexcom1UsartObj.wrOutIndex;
    uint32_t wrInIndex = flexcom1UsartObj.wrInIndex;

    if (wrOutIndex != wrInIndex)
    {
        if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
        {
            *pWrByte = ((uint16_t*)&FLEXCOM1_USART_WriteBuffer)[wrOutIndex++];
        }
        else
        {
            *pWrByte = FLEXCOM1_USART_WriteBuffer[wrOutIndex++];
        }

        if (wrOutIndex >= flexcom1UsartObj.wrBufferSize)
        {
            wrOutIndex = 0;
        }

        flexcom1UsartObj.wrOutIndex = wrOutIndex;

        isSuccess = true;
    }

    return isSuccess;
}

static inline bool FLEXCOM1_USART_TxPushByte(uint16_t wrByte)
{
    uint32_t tempInIndex;
    uint32_t wrOutIndex;
    uint32_t wrInIndex;
    bool isSuccess = false;

    /* Take a snapshot of indices to avoid creation of critical section */
    wrOutIndex = flexcom1UsartObj.wrOutIndex;
    wrInIndex = flexcom1UsartObj.wrInIndex;

    tempInIndex = wrInIndex + 1;

    if (tempInIndex >= flexcom1UsartObj.wrBufferSize)
    {
        tempInIndex = 0;
    }
    if (tempInIndex != wrOutIndex)
    {
        if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
        {
            ((uint16_t*)&FLEXCOM1_USART_WriteBuffer)[wrInIndex] = wrByte;
        }
        else
        {
            FLEXCOM1_USART_WriteBuffer[wrInIndex] = (uint8_t)wrByte;
        }

        flexcom1UsartObj.wrInIndex = tempInIndex;
        isSuccess = true;
    }
    else
    {
        /* Queue is full. Report Error. */
    }

    return isSuccess;
}

/* This routine is only called from ISR. Hence do not disable/enable USART interrupts. */
static void FLEXCOM1_USART_WriteNotificationSend(void)
{
    uint32_t nFreeWrBufferCount;

    if (flexcom1UsartObj.isWrNotificationEnabled == true)
    {
        nFreeWrBufferCount = FLEXCOM1_USART_WriteFreeBufferCountGet();

        if(flexcom1UsartObj.wrCallback != NULL)
        {
            if (flexcom1UsartObj.isWrNotifyPersistently == true)
            {
                if (nFreeWrBufferCount >= flexcom1UsartObj.wrThreshold)
                {
                    flexcom1UsartObj.wrCallback(FLEXCOM_USART_EVENT_WRITE_THRESHOLD_REACHED, flexcom1UsartObj.wrContext);
                }
            }
            else
            {
                if (nFreeWrBufferCount == flexcom1UsartObj.wrThreshold)
                {
                    flexcom1UsartObj.wrCallback(FLEXCOM_USART_EVENT_WRITE_THRESHOLD_REACHED, flexcom1UsartObj.wrContext);
                }
            }
        }
    }
}

static size_t FLEXCOM1_USART_WritePendingBytesGet(void)
{
    size_t nPendingTxBytes;

    /* Take a snapshot of indices to avoid creation of critical section */

    uint32_t wrOutIndex = flexcom1UsartObj.wrOutIndex;
    uint32_t wrInIndex = flexcom1UsartObj.wrInIndex;

    if ( wrInIndex >=  wrOutIndex)
    {
        nPendingTxBytes =  wrInIndex -  wrOutIndex;
    }
    else
    {
        nPendingTxBytes =  (flexcom1UsartObj.wrBufferSize -  wrOutIndex) + wrInIndex;
    }

    return nPendingTxBytes;
}

size_t FLEXCOM1_USART_WriteCountGet(void)
{
    size_t nPendingTxBytes;

    nPendingTxBytes = FLEXCOM1_USART_WritePendingBytesGet();

    return nPendingTxBytes;
}

size_t FLEXCOM1_USART_Write(uint8_t* pWrBuffer, const size_t size )
{
    size_t nBytesWritten  = 0;

    while (nBytesWritten < size)
    {
        if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
        {
            if (FLEXCOM1_USART_TxPushByte(((uint16_t*)pWrBuffer)[nBytesWritten]) == true)
            {
                nBytesWritten++;
            }
            else
            {
                /* Queue is full, exit the loop */
                break;
            }
        }
        else
        {
            if (FLEXCOM1_USART_TxPushByte(pWrBuffer[nBytesWritten]) == true)
            {
                nBytesWritten++;
            }
            else
            {
                /* Queue is full, exit the loop */
                break;
            }
        }
    }

    /* Check if any data is pending for transmission */
    if (FLEXCOM1_USART_WritePendingBytesGet() > 0)
    {
        /* Enable TX interrupt as data is pending for transmission */
        FLEXCOM1_USART_TX_INT_ENABLE();
    }

    return nBytesWritten;
}

size_t FLEXCOM1_USART_WriteFreeBufferCountGet(void)
{
    return (flexcom1UsartObj.wrBufferSize - 1) - FLEXCOM1_USART_WriteCountGet();
}

size_t FLEXCOM1_USART_WriteBufferSizeGet(void)
{
    return (flexcom1UsartObj.wrBufferSize - 1);
}

bool FLEXCOM1_USART_TransmitComplete(void)
{
    bool status = false;

    if (FLEXCOM1_REGS->FLEX_US_CSR & FLEX_US_CSR_TXEMPTY_Msk)
    {
        status = true;
    }

    return status;
}

bool FLEXCOM1_USART_WriteNotificationEnable(bool isEnabled, bool isPersistent)
{
    bool previousStatus = flexcom1UsartObj.isWrNotificationEnabled;

    flexcom1UsartObj.isWrNotificationEnabled = isEnabled;

    flexcom1UsartObj.isWrNotifyPersistently = isPersistent;

    return previousStatus;
}

void FLEXCOM1_USART_WriteThresholdSet(uint32_t nBytesThreshold)
{
    if (nBytesThreshold > 0)
    {
        flexcom1UsartObj.wrThreshold = nBytesThreshold;
    }
}

void FLEXCOM1_USART_WriteCallbackRegister( FLEXCOM_USART_RING_BUFFER_CALLBACK callback, uintptr_t context)
{
    flexcom1UsartObj.wrCallback = callback;

    flexcom1UsartObj.wrContext = context;
}

/* This routine is only called from ISR. Hence do not disable/enable USART interrupts. */
static inline bool FLEXCOM1_USART_RxPushByte(uint16_t rdByte)
{
    uint32_t tempInIndex;
    bool isSuccess = false;

    tempInIndex = flexcom1UsartObj.rdInIndex + 1;

    if (tempInIndex >= flexcom1UsartObj.rdBufferSize)
    {
        tempInIndex = 0;
    }

    if (tempInIndex == flexcom1UsartObj.rdOutIndex)
    {
        /* Queue is full - Report it to the application. Application gets a chance to free up space by reading data out from the RX ring buffer */
        if(flexcom1UsartObj.rdCallback != NULL)
        {
            flexcom1UsartObj.rdCallback(FLEXCOM_USART_EVENT_READ_BUFFER_FULL, flexcom1UsartObj.rdContext);

            /* Read the indices again in case application has freed up space in RX ring buffer */
            tempInIndex = flexcom1UsartObj.rdInIndex + 1;

            if (tempInIndex >= flexcom1UsartObj.rdBufferSize)
            {
                tempInIndex = 0;
            }
        }
    }

    /* Attempt to push the data into the ring buffer */
    if (tempInIndex != flexcom1UsartObj.rdOutIndex)
    {
        if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
        {
            ((uint16_t*)&FLEXCOM1_USART_ReadBuffer)[flexcom1UsartObj.rdInIndex] = rdByte;
        }
        else
        {
            FLEXCOM1_USART_ReadBuffer[flexcom1UsartObj.rdInIndex] = (uint8_t)rdByte;
        }

        flexcom1UsartObj.rdInIndex = tempInIndex;
        isSuccess = true;
    }
    else
    {
        /* Queue is full. Data will be lost. */
    }

    return isSuccess;
}

/* This routine is only called from ISR. Hence do not disable/enable USART interrupts. */
static void FLEXCOM1_USART_ReadNotificationSend(void)
{
    uint32_t nUnreadBytesAvailable;

    if (flexcom1UsartObj.isRdNotificationEnabled == true)
    {
        nUnreadBytesAvailable = FLEXCOM1_USART_ReadCountGet();

        if(flexcom1UsartObj.rdCallback != NULL)
        {
            if (flexcom1UsartObj.isRdNotifyPersistently == true)
            {
                if (nUnreadBytesAvailable >= flexcom1UsartObj.rdThreshold)
                {
                    flexcom1UsartObj.rdCallback(FLEXCOM_USART_EVENT_READ_THRESHOLD_REACHED, flexcom1UsartObj.rdContext);
                }
            }
            else
            {
                if (nUnreadBytesAvailable == flexcom1UsartObj.rdThreshold)
                {
                    flexcom1UsartObj.rdCallback(FLEXCOM_USART_EVENT_READ_THRESHOLD_REACHED, flexcom1UsartObj.rdContext);
                }
            }
        }
    }
}

size_t FLEXCOM1_USART_Read(uint8_t* pRdBuffer, const size_t size)
{
    size_t nBytesRead = 0;
    uint32_t rdOutIndex = 0;
    uint32_t rdInIndex = 0;


    /* Take a snapshot of indices to avoid creation of critical section */
    rdOutIndex = flexcom1UsartObj.rdOutIndex;
    rdInIndex = flexcom1UsartObj.rdInIndex;

    while (nBytesRead < size)
    {
        if (rdOutIndex != rdInIndex)
        {
            if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
            {
                ((uint16_t*)pRdBuffer)[nBytesRead++] = ((uint16_t*)&FLEXCOM1_USART_ReadBuffer)[rdOutIndex++];
            }
            else
            {
                pRdBuffer[nBytesRead++] = FLEXCOM1_USART_ReadBuffer[rdOutIndex++];
            }

            if (rdOutIndex >= flexcom1UsartObj.rdBufferSize)
            {
                rdOutIndex = 0;
            }
        }
        else
        {
            /* No more data available in the RX buffer */
            break;
        }
    }

    flexcom1UsartObj.rdOutIndex = rdOutIndex;

    return nBytesRead;
}

size_t FLEXCOM1_USART_ReadCountGet(void)
{
    size_t nUnreadBytesAvailable;
    uint32_t rdOutIndex;
    uint32_t rdInIndex;


    /* Take a snapshot of indices to avoid creation of critical section */
    rdOutIndex = flexcom1UsartObj.rdOutIndex;
    rdInIndex = flexcom1UsartObj.rdInIndex;

    if ( rdInIndex >=  rdOutIndex)
    {
        nUnreadBytesAvailable =  rdInIndex -  rdOutIndex;
    }
    else
    {
        nUnreadBytesAvailable =  (flexcom1UsartObj.rdBufferSize -  rdOutIndex) + rdInIndex;
    }

    return nUnreadBytesAvailable;
}

size_t FLEXCOM1_USART_ReadFreeBufferCountGet(void)
{
    return (flexcom1UsartObj.rdBufferSize - 1) - FLEXCOM1_USART_ReadCountGet();
}

size_t FLEXCOM1_USART_ReadBufferSizeGet(void)
{
    return (flexcom1UsartObj.rdBufferSize - 1);
}


bool FLEXCOM1_USART_ReadNotificationEnable(bool isEnabled, bool isPersistent)
{
    bool previousStatus = flexcom1UsartObj.isRdNotificationEnabled;

    flexcom1UsartObj.isRdNotificationEnabled = isEnabled;

    flexcom1UsartObj.isRdNotifyPersistently = isPersistent;


    return previousStatus;
}

void FLEXCOM1_USART_ReadThresholdSet(uint32_t nBytesThreshold)
{
    if (nBytesThreshold > 0)
    {
        flexcom1UsartObj.rdThreshold = nBytesThreshold;

    }
}

void FLEXCOM1_USART_ReadCallbackRegister( FLEXCOM_USART_RING_BUFFER_CALLBACK callback, uintptr_t context)
{
    flexcom1UsartObj.rdCallback = callback;

    flexcom1UsartObj.rdContext = context;
}

void static FLEXCOM1_USART_ISR_RX_Handler( void )
{
    uint16_t rdData = 0;

    /* Keep reading until there is a character availabe in the RX FIFO */
    while(FLEX_US_CSR_RXRDY_Msk == (FLEXCOM1_REGS->FLEX_US_CSR & FLEX_US_CSR_RXRDY_Msk))
    {
        if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
        {
            rdData = *((uint16_t*)&FLEXCOM1_REGS->FLEX_US_RHR) & FLEX_US_RHR_RXCHR_Msk;
        }
        else
        {
            rdData = *((uint8_t*)&FLEXCOM1_REGS->FLEX_US_RHR);
        }

        if (FLEXCOM1_USART_RxPushByte( rdData ) == true)
        {
            FLEXCOM1_USART_ReadNotificationSend();
        }
        else
        {
            /* UART RX buffer is full */
        }
    }


}

void static FLEXCOM1_USART_ISR_TX_Handler( void )
{
    uint16_t wrByte;

    /* Keep writing to the TX FIFO as long as there is space */
    while(FLEX_US_CSR_TXRDY_Msk == (FLEXCOM1_REGS->FLEX_US_CSR & FLEX_US_CSR_TXRDY_Msk))
    {
        if (FLEXCOM1_USART_TxPullByte(&wrByte) == true)
        {
            if (FLEXCOM1_REGS->FLEX_US_MR & FLEX_US_MR_MODE9_Msk)
            {
                *((uint16_t*)&FLEXCOM1_REGS->FLEX_US_THR) = wrByte & FLEX_US_THR_TXCHR_Msk;
            }
            else
            {
                *((uint8_t*)&FLEXCOM1_REGS->FLEX_US_THR) = (uint8_t)wrByte;
            }

            /* Send notification */
            FLEXCOM1_USART_WriteNotificationSend();
        }
        else
        {
            /* Nothing to transmit. Disable the data register empty/fifo Threshold interrupt. */
            FLEXCOM1_USART_TX_INT_DISABLE();
            break;
        }
    }

}

void FLEXCOM1_InterruptHandler( void )
{
    /* Channel status */
    uint32_t channelStatus = FLEXCOM1_REGS->FLEX_US_CSR;

    /* Error status */
    uint32_t errorStatus = (channelStatus & (FLEX_US_CSR_OVRE_Msk | FLEX_US_CSR_FRAME_Msk | FLEX_US_CSR_PARE_Msk));


    if(errorStatus != 0)
    {
        /* Save the error so that it can be reported when application calls the FLEXCOM1_USART_ErrorGet() API */
        flexcom1UsartObj.errorStatus = (FLEXCOM_USART_ERROR)errorStatus;

        /* Clear the error flags and flush out the error bytes */
        FLEXCOM1_USART_ErrorClear();

        /* USART errors are normally associated with the receiver, hence calling receiver context */
        if( flexcom1UsartObj.rdCallback != NULL )
        {
            flexcom1UsartObj.rdCallback(FLEXCOM_USART_EVENT_READ_ERROR, flexcom1UsartObj.rdContext);
        }
    }

    /* Receiver status. RX interrupt is never disabled. */
    if (channelStatus & FLEX_US_CSR_RXRDY_Msk)
    {
        FLEXCOM1_USART_ISR_RX_Handler();
    }

    /* Transmitter status */
    if( (channelStatus & FLEX_US_CSR_TXRDY_Msk) && (FLEXCOM1_REGS->FLEX_US_IMR & FLEX_US_IMR_TXRDY_Msk) )
    {
        FLEXCOM1_USART_ISR_TX_Handler();
    }
}
