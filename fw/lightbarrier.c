/*
 * lightbarrier.c
 *
 *  Created on: 27 Mar 2017
 *      Author: raffael
 */

#include "lightbarrier.h"
#include "../Board.h"
#include <ti/sysbios/hal/Hwi.h>
#include <msp430.h>
#include "user_button.h"

void lightBarrier_init()
{
	//configure IR LED port as PWM output with ca. 1 kHz (adapt to something binary) frequency and
	//				. duty cycle 1 % [note: rise & fall time are ca. 12 ns]
	// make sure IR LED port is off

    // Configure GPIO
    P1DIR |= BIT0;                     // P1.0 and P1.1 output
    P1SEL0 |= BIT0;                    // P1.0 and P1.1 options select

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings

    CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
    CSCTL1 = DCOFSEL_6;                       // Set DCO = 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;// Set ACLK=VLO SMCLK=DCO
    CSCTL3 |= DIVS_5;                          // Set divide by 5
    CSCTL0_H = 0;                             // Lock CS registers

    // Configure Timer0_A
    TA0CCR0 = 42;               	// PWM Period --> 8MHz/5/42 = 38.1 kHz,
    								// which is the center frequency of the IR receiver
    TA0CCTL1 = OUTMOD_7;                      // CCR1 reset/set
    TA0CCR1 = 21;                            // CCR1 PWM duty cycle: 50%. test if we can make it lower!
//    TA0CCTL2 = OUTMOD_7;                      // CCR2 reset/set
//    TA0CCR2 = 250;                            // CCR2 PWM duty cycle
    TA0CTL = TASSEL__SMCLK | MC__UP | TACLR;  // SMCLK, up mode, clear TAR


	//configure P1.3 and P1.4 as analog inputs
		// first, read the analog value of both inputs at ambient light conditions
		// (i.e. IR LED off). Store value as IR_off_input

		// turn on IR LED
		// now, read the analog value of both inputs at LED ON condition
		// Store value as IR_on_input
		// turn off IR LED

	//configure P1.3 and P1.4 as comparator inputs with interrupt on falling edge.
		// set threshold to (IR_on_input + IR_off_input)/2
}

void lightBarrier_Task()
{


    lightBarrier_init();

	GPIO_write(Board_led_blue,1);
	GPIO_write(Board_led_green,0);


    while (1) {
        Task_sleep(100);
    }
}

//// Comp_A interrupt service routine -- toggles LED
////#pragma vector=COMP_E_VECTOR
//void __attribute__((interrupt(COMP_E_VECTOR))) Comp_A_ISR (void)
//{
//	CEINT &= ~(CEIFG + CEIE); // Clear Interrupt flag and disable interrupt
//
//	//check if button is still switched on.
//	if((PORT_DIGITAL_IN & PIN_BUTTON) == 0)
//	{
//		wakeup_source = WAKEUP_FROM_COMPARATOR;
//
//		//restart previously stopped timer:
//		timer0_A_start();
//		LPM4_EXIT;
//	}
//}

