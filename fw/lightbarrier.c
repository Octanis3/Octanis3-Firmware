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
#include "logger.h"

#include <time.h>
#include <ti/sysbios/hal/Seconds.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

struct _lb_status{
	uint8_t outer_trig;
	uint8_t inner_trig;
	enum dir{
		_OUT = 0, //consistent with logger
		_IN,
		UNDEF,
	} direction;
	uint32_t timestamp;
	uint16_t event_counter;
} lb_status;

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
    CSCTL3 |= DIVS__4;                          // Set divide by 4
    CSCTL0_H = 0;                             // Lock CS registers

    // Configure Timer0_A
    TA0CCR0 = 52;               	// PWM Period --> 8MHz/4/52 = 38.46 kHz,
    								// which is the center frequency of the IR receiver
    TA0CCTL1 = OUTMOD_7;                      // CCR1 reset/set
    TA0CCR1 = 26;                            // CCR1 PWM duty cycle: 50%. test if we can make it lower!
//    TA0CCTL2 = OUTMOD_7;                      // CCR2 reset/set
//    TA0CCR2 = 250;                            // CCR2 PWM duty cycle
    TA0CTL = TASSEL__SMCLK | MC__UP | TACLR;  // SMCLK, up mode, clear TAR

    // reset state
	lb_status.event_counter = 0;
	lb_status.inner_trig = 0;
	lb_status.outer_trig = 0;
	lb_status.direction = UNDEF;

	//enable P1.3 interrupt
	GPIO_enableInt(nbox_lightbarrier_ext);
	GPIO_enableInt(nbox_lightbarrier_int);


}

void lightBarrier_turn_off()
{
    TA0CCTL1 = OUTMOD_0;           // output mode
    P1OUT &= ~BIT0;                // set P1.0 low.
}

void LightBarrier_turn_on()
{
    TA0CCTL1 = OUTMOD_7;                      // CCR1 reset/set
}

void lightBarrier_Task()
{
    lightBarrier_init();

	GPIO_write(Board_led_blue,1);
	GPIO_write(Board_led_green,0);


    while (1) {
		Semaphore_pend((Semaphore_Handle)semLB1, BIOS_WAIT_FOREVER);
		// first event was detected
		lb_status.timestamp = Seconds_get();

    		if(Semaphore_pend((Semaphore_Handle)semLB2, 10000))
    		{
    			// second event was detected
    			// --> turn off PWM (?)

    			Task_sleep(10000);
    			        // timeout for event duration reached.
    			        // --> reset all states and turn on PWM
    			// --> create logging event
    			log_write_new_entry(lb_status.timestamp, 0xFFFFFFFF, lb_status.direction);

    			Task_sleep(10);
			GPIO_enableInt(nbox_lightbarrier_ext);
			GPIO_enableInt(nbox_lightbarrier_int);

			//make sure no events occur anymore
			while(lb_status.event_counter)
			{
				lb_status.event_counter = 0;
				Task_sleep(2000);
			}
    		}
    		else
    		{
    			// no second event detected

    			// check if reader detected ID

    		}

    		// reset state
    		lb_status.event_counter = 0;
		lb_status.inner_trig = 0;
		lb_status.outer_trig = 0;
		lb_status.direction = UNDEF;
		GPIO_enableInt(nbox_lightbarrier_ext);
		GPIO_enableInt(nbox_lightbarrier_int);

    }
}


void lightbarrier_input_isr(unsigned int index)
{
//	button_pressed = 1;
//
//	//check interrupt source
	if(index == nbox_lightbarrier_ext)
	{
		GPIO_disableInt(nbox_lightbarrier_ext);
		if(lb_status.inner_trig == 1 && lb_status.outer_trig == 0)
		{
			// second lightbarrier has triggered before --> direction is known
			lb_status.direction = _OUT;
			// post second LB semaphore (has 10sec timeout)
			Semaphore_post((Semaphore_Handle)semLB2);
		}
		else
		{
			// no lightbarrier interruption has triggered before
			// --> post first LB semaphore (has no timeout) and start RFID reader
			Semaphore_post((Semaphore_Handle)semLB1);
			Semaphore_post((Semaphore_Handle)semReader);
		}
	}
	else if(index == nbox_lightbarrier_int)
	{
		GPIO_disableInt(nbox_lightbarrier_int);
		if(lb_status.outer_trig == 1 && lb_status.inner_trig == 0)
		{
			// second lightbarrier has triggered before --> direction is known
			lb_status.direction = _IN;
			// post second LB semaphore (has 10sec timeout)
			Semaphore_post((Semaphore_Handle)semLB2);
		}
		else if(lb_status.inner_trig == 0)
		{
			// no lightbarrier interruption has triggered before
			// --> post first LB semaphore (has no timeout) and start RFID reader
			Semaphore_post((Semaphore_Handle)semLB1);
			Semaphore_post((Semaphore_Handle)semReader);
		}
		lb_status.inner_trig = 1;
	}

	lb_status.event_counter = lb_status.event_counter+1;
}
