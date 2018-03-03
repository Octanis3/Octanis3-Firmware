/*
 * load_cell.c
 *
 *  Created on: 03 Mar 2018
 *      Author: raffael
 */

#include "HX711/HX711.h"
#include "../Board.h"
#include "uart_helper.h"


void load_cell_Task()
{
	Task_sleep(1000); //wait until things are settled...

	hx711_begin(nbox_loadcell_data, nbox_loadcell_clk, 128);

	hx711_power_up();

	Task_sleep(1000); //wait until things are settled...
	hx711_tare(20);
	float value;

	while(1)
	{


		value = hx711_get_units(10);

		char sign = '+';
		if(value<0)
		{
			value = -value;
			sign = '-';
		}

		uart_serial_print_event('W', &sign, 1);

		uint8_t strlen;
		uint8_t sec_buf[7];
		strlen = ui2a((unsigned long)value, 10, 1, HIDE_LEADING_ZEROS, sec_buf);
		sec_buf[strlen]='\n';
		UART_write(debug_uart, sec_buf, strlen+1);

		Task_sleep(1000);
	}

}


