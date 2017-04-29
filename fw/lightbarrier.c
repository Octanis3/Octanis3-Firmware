/*
 * lightbarrier.c
 *
 *  Created on: 27 Mar 2017
 *      Author: raffael
 */

#include "lightbarrier.h"
#include "../Board.h"

#include <ti/sysbios/hal/Hwi.h>


void lightBarrier_init()
{
	//configure IR LED port as PWM output with ca. 1 kHz (adapt to something binary) frequency and
	//				duty cycle 1 % [note: rise & fall time are ca. 12 ns]
	// make sure IR LED port is off

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
//	Hwi_enable(); //not sure if needed here??
//
//	GPIO_enableInt(lp_button);

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
