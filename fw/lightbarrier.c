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


void lightBarrier_init()
{
	//configure IR LED port as PWM output with ca. 1 kHz (adapt to something binary) frequency and
	//				. duty cycle 1 % [note: rise & fall time are ca. 12 ns]
	// make sure IR LED port is off
    /*
    int main(void)
    {*/
    WDTCTL = WDTPW | WDTHOLD;                 // Stop WDT

    // Configure GPIO
    P1DIR |= BIT0 | BIT1;                     // P1.0 and P1.1 output
    P1SEL0 |= BIT0 | BIT1;                    // P1.0 and P1.1 options select

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
    CSCTL1 = DCOFSEL_6;                       // Set DCO = 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;// Set ACLK=VLO SMCLK=DCO
    CSCTL3 = DIVA__8 | DIVS__8 | DIVM__8;     // Set all dividers
    CSCTL0_H = 0;                             // Lock CS registers

    // Configure Timer0_A
    TA0CCR0 = 1000-1;                         // PWM Period
    TA0CCTL1 = OUTMOD_7;                      // CCR1 reset/set
    TA0CCR1 = 12;                            // CCR1 PWM duty cycle
    TA0CCTL2 = OUTMOD_7;                      // CCR2 reset/set
    TA0CCR2 = 250;                            // CCR2 PWM duty cycle
    TA0CTL = TASSEL__SMCLK | MC__UP | TACLR;  // SMCLK, up mode, clear TAR

    __bis_SR_register(LPM0_bits);             // Enter LPM0
    __no_operation();                         // For debugger
    //}

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

void lightBarrier_run()
{
    //stub
}

void lightBarrier_Task()
{
//	Hwi_enable(); //not sure if needed here??
//
//	GPIO_enableInt(lp_button);

    lightBarrier_init();
//    GPIO_toggle(Board_led_green);

	GPIO_write(Board_led_blue,1);
	GPIO_write(Board_led_green,1);


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
