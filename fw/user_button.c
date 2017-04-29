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



void user_button_isr(unsigned int index)
{

	GPIO_toggle(Board_led_IR);
	//check interrupt source

//	Hwi_enable(); //not sure if needed here??

//	GPIO_enableInt(lp_button);//not sure if needed here??
}
