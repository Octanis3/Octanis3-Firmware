/*
 * user_button.c
 *
 *  Created on: 01 Apr 2017
 *      Author: raffael
 */

#include <ti/drivers/GPIO.h>

#include "user_button.h"
#include "logger.h"
#include "../Board.h"

#include <ti/sysbios/hal/Hwi.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

int button_pressed = 0;
int action_executing = 0;

void user_button_Task()
{
	GPIO_enableInt(Board_button);

	while(1)
	{
		action_executing = 0;
		Semaphore_pend((Semaphore_Handle)semButton, BIOS_WAIT_FOREVER);
		action_executing = 1; //to prevent another button event in a short time (due to re-bouncing)

		log_send_data_via_uart();

		Task_sleep(1000); //avoid too many subsequent memory readouts
	}
}


void user_button_isr(unsigned int index)
{
//	button_pressed = 1;

	//check interrupt source
	if(action_executing == 0)
		Semaphore_post((Semaphore_Handle)semButton);


//	Hwi_enable(); //not sure if needed here??

//	GPIO_enableInt(lp_button);//not sure if needed here??
}
