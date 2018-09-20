/*
 * user_button.c
 *
 *  Created on: 01 Apr 2017
 *      Author: raffael
 */

#include <ti/drivers/GPIO.h>

#include "user_button.h"
#include "logger.h"
#include "rfid_reader.h"
#include "../Board.h"

#include <ti/sysbios/hal/Hwi.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

void user_button_Task()
{
	GPIO_enableInt(Board_button);

	while(1)
	{
		Semaphore_pend((Semaphore_Handle)semButton, BIOS_WAIT_FOREVER);

		/* Turn on data LED  */
		GPIO_write(Board_led_blue, Board_LED_ON);

		log_restart();

		Task_sleep(1000); //avoid too many subsequent memory readouts
        GPIO_write(Board_led_blue, Board_LED_OFF);

		GPIO_clearInt(Board_button);
		GPIO_enableInt(Board_button);
	}
}


void user_button_isr(unsigned int index)
{
//	button_pressed = 1;
	GPIO_disableInt(Board_button);

	//check interrupt source
	Semaphore_post((Semaphore_Handle)semButton);
}
