/*
 * lightbarrier.c
 *
 *  Created on: 27 Mar 2017
 *      Author: raffael
 */

#include "lightbarrier.h"
#include "../Board.h"
#include "user_button.h"

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
	GPIO_write(Board_led_blue,1);
	GPIO_write(Board_led_green,0);

    while (1) {
        Task_sleep(100);
    }
}
