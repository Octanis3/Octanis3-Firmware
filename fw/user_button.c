/*
 * user_button.c
 *
 *  Created on: 01 Apr 2017
 *      Author: raffael
 */

#include <ti/drivers/GPIO.h>

#include "user_button.h"
#include "../Board.h"

#include <ti/sysbios/hal/Hwi.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

int button_pressed = 0;

void user_button_isr(unsigned int index)
{
//	button_pressed = 1;
	//check interrupt source
	Semaphore_post((Semaphore_Handle)semReader);


//	Hwi_enable(); //not sure if needed here??

//	GPIO_enableInt(lp_button);//not sure if needed here??
}
