/*******************************************************************************
  Board Support Package Header File.

  Company:
    Microchip Technology Inc.

  File Name:
    bsp.h

  Summary:
    Board Support Package Header File 

  Description:
    This file contains constants, macros, type definitions and function
    declarations 
*******************************************************************************/

// DOM-IGNORE-BEGIN
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
// DOM-IGNORE-END

#ifndef _BSP_H
#define _BSP_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "device.h"

// *****************************************************************************
// *****************************************************************************
// Section: BSP Macros
// *****************************************************************************
// *****************************************************************************
/*PIOA base address */
#define PIOA_REGS   ((pio_group_registers_t*)(&(PIO_REGS->PIO_GROUP[0])))
/*PIOB base address */
#define PIOB_REGS   ((pio_group_registers_t*)(&(PIO_REGS->PIO_GROUP[1])))
/*PIOC base address */
#define PIOC_REGS   ((pio_group_registers_t*)(&(PIO_REGS->PIO_GROUP[2])))
/*PIOD base address */
#define PIOD_REGS   ((pio_group_registers_t*)(&(PIO_REGS->PIO_GROUP[3])))

/*** LED Macros for LED0 ***/
#define LED0_Toggle() do { PIOB_REGS->PIO_MSKR = (1U<<11); (PIOB_REGS->PIO_ODSR ^= (1U<<11)); } while (0)
#define LED0_Get() ((PIOB_REGS->PIO_PDSR >> 11) & 0x1)
#define LED0_On() (PIOB_REGS->PIO_SODR = (1U<<11))
#define LED0_Off() (PIOB_REGS->PIO_CODR = (1U<<11))
/*** LED Macros for LED1 ***/
#define LED1_Toggle() do { PIOB_REGS->PIO_MSKR = (1U<<12); (PIOB_REGS->PIO_ODSR ^= (1U<<12)); } while (0)
#define LED1_Get() ((PIOB_REGS->PIO_PDSR >> 12) & 0x1)
#define LED1_On() (PIOB_REGS->PIO_SODR = (1U<<12))
#define LED1_Off() (PIOB_REGS->PIO_CODR = (1U<<12))
/*** LED Macros for LED2 ***/
#define LED2_Toggle() do { PIOB_REGS->PIO_MSKR = (1U<<13); (PIOB_REGS->PIO_ODSR ^= (1U<<13)); } while (0)
#define LED2_Get() ((PIOB_REGS->PIO_PDSR >> 13) & 0x1)
#define LED2_On() (PIOB_REGS->PIO_SODR = (1U<<13))
#define LED2_Off() (PIOB_REGS->PIO_CODR = (1U<<13))
/*** LED Macros for LED3 ***/
#define LED3_Toggle() do { PIOB_REGS->PIO_MSKR = (1U<<14); (PIOB_REGS->PIO_ODSR ^= (1U<<14)); } while (0)
#define LED3_Get() ((PIOB_REGS->PIO_PDSR >> 14) & 0x1)
#define LED3_On() (PIOB_REGS->PIO_SODR = (1U<<14))
#define LED3_Off() (PIOB_REGS->PIO_CODR = (1U<<14))
/*** LED Macros for LED4 ***/
#define LED4_Toggle() do { PIOA_REGS->PIO_MSKR = (1U<<17); (PIOA_REGS->PIO_ODSR ^= (1U<<17)); } while (0)
#define LED4_Get() ((PIOA_REGS->PIO_PDSR >> 17) & 0x1)
#define LED4_On() (PIOA_REGS->PIO_SODR = (1U<<17))
#define LED4_Off() (PIOA_REGS->PIO_CODR = (1U<<17))
/*** LED Macros for LED5 ***/
#define LED5_Toggle() do { PIOA_REGS->PIO_MSKR = (1U<<18); (PIOA_REGS->PIO_ODSR ^= (1U<<18)); } while (0)
#define LED5_Get() ((PIOA_REGS->PIO_PDSR >> 18) & 0x1)
#define LED5_On() (PIOA_REGS->PIO_SODR = (1U<<18))
#define LED5_Off() (PIOA_REGS->PIO_CODR = (1U<<18))
/*** SWITCH Macros for PB1 ***/
#define PB1_Get() ((PIOB_REGS->PIO_PDSR >> 0) & 0x1)
#define PB1_STATE_PRESSED 0
#define PB1_STATE_RELEASED 1
/*** SWITCH Macros for PB2 ***/
#define PB2_Get() ((PIOB_REGS->PIO_PDSR >> 1) & 0x1)
#define PB2_STATE_PRESSED 0
#define PB2_STATE_RELEASED 1
/*** SWITCH Macros for PB3 ***/
#define PB3_Get() ((PIOB_REGS->PIO_PDSR >> 2) & 0x1)
#define PB3_STATE_PRESSED 0
#define PB3_STATE_RELEASED 1




// *****************************************************************************
// *****************************************************************************
// Section: Interface Routines
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Function:
    void BSP_Initialize(void)

  Summary:
    Performs the necessary actions to initialize a board

  Description:
    This function initializes the LED and Switch ports on the board.  This
    function must be called by the user before using any APIs present on this
    BSP.

  Precondition:
    None.

  Parameters:
    None

  Returns:
    None.

  Example:
    <code>
    //Initialize the BSP
    BSP_Initialize();
    </code>

  Remarks:
    None
*/

void BSP_Initialize(void);

#endif // _BSP_H

/*******************************************************************************
 End of File
*/
