/*
 * PIR_wakeup.h
 *
 *  Created on: 12 Mar 2018
 *      Author: Lennard
 */


/*
 * Interupt pin 1 : P3_0
 * Interupt pin 2 : P3_1
 */

#include <ti/drivers/GPIO.h>

#include "PIR_wakeup.h"
#include "load_cell.h"
#include "logger.h"
#include "../Board.h"

#include <ti/sysbios/hal/Hwi.h>
#include <msp430.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

unsigned int int_pin = 0;

void PIR_wakeup_Task()
{
	GPIO_enableInt(PIR_pin);

	while(1)
	{
		Semaphore_pend((Semaphore_Handle)semPIRwakeup, BIOS_WAIT_FOREVER);

		//Send out data on serial port
		print_load_cell_value(int_pin, 'I');

		Task_sleep(10); //avoid too many subsequent memory readouts
		int_pin = 0;
        Semaphore_reset((Semaphore_Handle)semPIRwakeup,0);

		GPIO_enableInt(PIR_pin);

	}
}


void PIR_wakeup_isr(unsigned int index)
{
//	button_pressed = 1;
	GPIO_disableInt(PIR_pin);
	int_pin = index;
	//check interrupt source
	Semaphore_post((Semaphore_Handle)semPIRwakeup);
}
