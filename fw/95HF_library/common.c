/*
 * common.c
 *
 *  Created on: 27 May 2017
 *      Author: raffael
 */

#include "common.h"
#include "../../Board.h"

void delay_ms(uint16_t delay)
{
	Task_sleep(delay);
}


/**
 *	@brief  Time delay in microsecond
 *  @param  delay : delay in us.
 *  @retval none
 */
void delay_us(uint16_t delay)
{
	Task_sleep(1);
}
