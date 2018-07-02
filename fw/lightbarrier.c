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
#include "uart_helper.h"
#include "rfid_reader.h"

#include <time.h>
#include <ti/sysbios/hal/Seconds.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

#define T_EVENT_DURATION 5000 //minimum time in ms between two consecutive events that may be logged.
#define T_INACTIVITY	2000 	 //minimum time of no recorded activities after an event+T_EVENT_DURATION until new events are treated as such.

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
	uint8_t event_valid;
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
    // CSCTL1 = DCORSEL_L | DCOFSEL_4;           // Set DCO = 16 MHz

    // SELA__LFXTCLK needed for real time clock!!
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;// Set SMCLK=DCO, rest = default config
    // CSCTL3 |= DIVS__0;                     // Set divide by 0
    CSCTL0_H = 0;                             // Lock CS registers

    // Configure Timer0_A
    TA0CCR0 = 210;//421;               	// PWM Period --> 16MHz/421 = 38.005 kHz,
    								// which is the center frequency of the IR receiver


    TA0CCTL1 = OUTMOD_7;                      // CCR1 reset/set
    TA0CCR1 = 50;//105;                            // CCR1 PWM duty cycle: 50%. test if we can make it lower!


    TA3CTL = TASSEL__SMCLK | MC__CONTINUOUS | TACLR;  // SMCLK, up mode, clear TAR

    // reset state
	lb_status.event_counter = 0;
	lb_status.inner_trig = 0;
	lb_status.outer_trig = 0;
	lb_status.direction = UNDEF;
	lb_status.event_valid = 1; //by default, any event which follows will be considered valid.

	//enable P1.3 interrupt
	GPIO_enableInt(nbox_lightbarrier_ext);
	GPIO_enableInt(nbox_lightbarrier_int);
}

void lightBarrier_turn_off()
{
#ifdef LAUNCHPAD_PINDEF
    TA0CCTL2 = OUTMOD_0;           // output mode
    P1OUT &= ~BIT1;                // set P1.0 low.
#else
    TA0CCTL1 = OUTMOD_0;           // output mode
    P1OUT &= ~BIT0;                // set P1.0 low.
#endif
}

void lightBarrier_turn_on()
{
#ifdef LAUNCHPAD_PINDEF
    TA0CCTL2 = OUTMOD_7;                      // CCR1 reset/set
#else
    TA0CCTL1 = OUTMOD_7;                      // CCR1 reset/set
#endif
}

void lightBarrier_Task()
{
    lightBarrier_init();

	GPIO_write(Board_led_blue,0);
	GPIO_write(Board_led_green,0);

    while (1) {

		Semaphore_pend((Semaphore_Handle)semLB1, BIOS_WAIT_FOREVER);
		// first event was detected
		lb_status.timestamp = Seconds_get();
		lb_status.event_valid = 0; // any subsequent event trigger is NOT considered to be a new event, until
		uint64_t tag_id=0;

    		if(Semaphore_pend((Semaphore_Handle)semLB2, 100))
    		{
    			Semaphore_pend((Semaphore_Handle)semSerial,BIOS_WAIT_FOREVER);
    			uint8_t c = lb_status.direction+'0';
    			uart_serial_print_event('L', &c, 1);
    			Semaphore_post((Semaphore_Handle)semSerial);

    			// second event was detected
    			// --> turn off PWM!

    			if(!log_phase_two())
    			{
					lightBarrier_turn_off();

					Task_sleep(T_EVENT_DURATION);
							// timeout for event duration reached.
							// --> reset all states and turn on PWM

					//turn on light barrier again.
					lightBarrier_turn_on();

					Task_sleep(10);
				GPIO_enableInt(nbox_lightbarrier_ext);
				GPIO_enableInt(nbox_lightbarrier_int);

				//make sure no events occur anymore
				while(lb_status.event_counter>0 || GPIO_read(nbox_lightbarrier_ext) || GPIO_read(nbox_lightbarrier_int))
				{
					lb_status.event_counter = 0;
					Task_sleep(T_INACTIVITY);
				}

				if(!rfid_get_id(&tag_id))
				{
					// readout failed --> turn off reader
					rfid_stop_detection();
					GPIO_write(Board_led_green,0);
					Task_sleep(250);
					GPIO_write(Board_led_green,1);
					Task_sleep(250);
					GPIO_write(Board_led_green,0);
					Task_sleep(250);
					GPIO_write(Board_led_green,1);
					Task_sleep(250);
					GPIO_write(Board_led_green,0);
				}
    			}
			// --> create logging event
			log_write_new_entry(lb_status.timestamp, tag_id, lb_status.direction);

    		}
    		else
    		{
    			Semaphore_pend((Semaphore_Handle)semSerial,BIOS_WAIT_FOREVER);
    			uint8_t c = (lb_status.outer_trig) + (lb_status.inner_trig*2) + 64; //will print A for outer and B for inner trigger'd
    			uart_serial_print_event('U', &c, 1);
    			Semaphore_post((Semaphore_Handle)semSerial);

    			// no second event detected!
    			// disable reader
    			if(!rfid_get_id(&tag_id) && !(log_phase_two()))
			{
				// readout failed --> turn off reader
				rfid_stop_detection();
			}
			log_write_new_entry(lb_status.timestamp, tag_id, lb_status.direction);
    		}

    		// reset state

    		GPIO_disableInt(nbox_lightbarrier_ext);
		GPIO_disableInt(nbox_lightbarrier_int);

    		lb_status.event_counter = 0;
		lb_status.inner_trig = 0;
		lb_status.outer_trig = 0;
		lb_status.direction = UNDEF;
		lb_status.event_valid = 1;

		GPIO_clearInt(nbox_lightbarrier_ext);
		GPIO_clearInt(nbox_lightbarrier_int);

		Semaphore_reset((Semaphore_Handle)semLB1, 0);

		GPIO_enableInt(nbox_lightbarrier_ext);
		GPIO_enableInt(nbox_lightbarrier_int);

    }
}


void lightbarrier_input_isr(unsigned int index)
{
//	//check interrupt source
	if(index == nbox_lightbarrier_ext)
	{
		GPIO_disableInt(nbox_lightbarrier_ext);
		if(lb_status.inner_trig == 1 && lb_status.outer_trig == 0)
		{
			// second lightbarrier has triggered before --> direction is known
			lb_status.direction = _IN;
			// post second LB semaphore (has 10sec timeout)
			Semaphore_post((Semaphore_Handle)semLB2);
		}
		else
		{
			// no lightbarrier interruption has triggered before
			// --> post first LB semaphore (has no timeout) and start RFID reader
			Semaphore_post((Semaphore_Handle)semLB1);
			if(lb_status.event_valid)
				rfid_start_detection();
		}
		lb_status.outer_trig = 1;
	}
	else if(index == nbox_lightbarrier_int)
	{
		GPIO_disableInt(nbox_lightbarrier_int);
		if(lb_status.outer_trig == 1 && lb_status.inner_trig == 0)
		{
			// second lightbarrier has triggered before --> direction is known
			lb_status.direction = _OUT;
			// post second LB semaphore (has 2 sec timeout)
			Semaphore_post((Semaphore_Handle)semLB2);
		}
		else if(lb_status.inner_trig == 0)
		{
			// no lightbarrier interruption has triggered before
			// --> post first LB semaphore (has no timeout) and start RFID reader
			Semaphore_post((Semaphore_Handle)semLB1);
			if(lb_status.event_valid)
				rfid_start_detection();
		}
		lb_status.inner_trig = 1;
	}

	lb_status.event_counter = lb_status.event_counter+1;
}
