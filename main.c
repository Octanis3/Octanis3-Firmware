/* --COPYRIGHT--,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//*****************************************************************************
//! USCI_B0, SPI 3-Wire Master Incremented Data
//! This example shows how SPI master talks to SPI slave using 3-wire mode.
//! Incrementing data is sent by the master starting at 0x01. Received data is
//! expected to be same as the previous transmission.  eUSCI RX ISR is used to
//! handle communication with the CPU, normally in LPM0. If high, P1.0 indicates
//! valid data reception.  Because all execution after LPM0 is in ISRs,
//! initialization waits for DCO to stabilize against ACLK.
//! ACLK = ~32.768kHz, MCLK = SMCLK = DCO ~ 1048kHz.  BRCLK = SMCLK/2
//!
//! Use with SPI Slave Data Echo code example.  If slave is in debug mode, P1.1
//! slave reset signal conflicts with slave's JTAG; to work around, use IAR's
//! "Release JTAG on Go" on slave device.  If breakpoints are set in
//! slave RX ISR, master must stopped also to avoid overrunning slave
//! RXBUF.
//!
//!             Tested on MSP430FR5969
//!                 -----------------
//!            /|\ |                 |
//!             |  |                 |
//!    Master---+->|RST              |
//!                |                 |
//!                |             P1.6|-> Data Out (UCB0SIMO)
//!                |                 |
//!                |             P1.7|<- Data In (UCB0SOMI)
//!                |                 |
//!                |             P2.2|-> Serial Clock Out (UCB0CLK)
//!
//!
//! This example uses the following peripherals and I/O signals.  You must
//! review these and change as needed for your own board:
//! - SPI peripheral
//! - GPIO Port peripheral (for SPI pins)
//! - UCB0SIMO
//! - UCB0SOMI
//! - UCB0CLK
//!
//! This example uses the following interrupt handlers.  To use this example
//! in your own application you must add these interrupt handlers to your
//! vector table.
//! - USCI_B0_VECTOR
//!


// This program uses the MSP430 library to use the SPI protocol communication.
//Master and slave are in the same microcontroller (MSP430FR5969 LAUNCHPAD)
//The SPI mode used is 3 wire, there are only one Slave
//Project inspired from: eusci_b_spi_ex1_master and eusci_a_spi_ex1_slave projects from MSPWARE library examples
//Developed by Jorge Zarate, 04.01.2017
//*****************************************************************************

#include "driverlib.h"
#include "myGpio.h"
#include "myClocks.h"
#include "mySPI.h"
#include "CR95HF_Communication.h"
#include <stdio.h>


extern int spfr;

void main(void)
{


//	volatile uint16_t i;   // What is the use/purporse of i??

    //Stop watchdog timer
	WDT_A_hold(WDT_A_BASE);

    // Initialize GPIO
     initGPIO();

     // Initialize clocks
     initClocks();

     // Initialize timers
     initSPI();



//     __bis_SR_register(LPM0_bits + GIE);      // CPU off, enable globally interrupts; may be turn off LPM0_bits (what is the purporse of LPM0?)
     __bis_SR_register( GIE );                                                   // Enable interrupts globally




     while(1) {
	 CR95HFstartCommunication();
//         __no_operation();                                                       // Placeholder for while loop (not required)
     }
 }

    /*
     * Disable the GPIO power-on default high-impedance mode to activate
     * previously configured port settings and before before clearing and enabling GPIO port interrupts
     */
    //PMM_unlockLPM5();





//    __no_operation();                       // Remain in LPM0,


