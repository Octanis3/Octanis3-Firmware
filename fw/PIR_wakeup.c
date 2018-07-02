/*
 * PIR_wakeup.h
 *
 *  Created on: 12 Mar 2018
 *      Author: Lennard
 */


/*
 * Interupt pin : P1_5
 * PWM pin      : P1_1
 */

#include <ti/drivers/GPIO.h>

#include "PIR_wakeup.h"
#include "logger.h"
#include "../Board.h"

#include <ti/sysbios/hal/Hwi.h>
#include <msp430.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

void PIR_wakeup_Task()
{
	GPIO_enableInt(PIR_pin);

	while(1)
	{
		Semaphore_pend((Semaphore_Handle)semPIRwakeup, BIOS_WAIT_FOREVER);

		//Send out data on serial port
		log_send_PIR();

		Task_sleep(1000); //avoid too many subsequent memory readouts
		GPIO_enableInt(PIR_pin);

	}
}


void PIR_wakeup_isr(unsigned int index)
{
//	button_pressed = 1;
	GPIO_disableInt(PIR_pin);

	//check interrupt source
	Semaphore_post((Semaphore_Handle)semPIRwakeup);
}
