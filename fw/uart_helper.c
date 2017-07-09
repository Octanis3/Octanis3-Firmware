/*
 * uart_helper.c
 *
 *  Created on: 17 Jun 2017
 *      Author: raffael
 */

#include "../Board.h"
#include "uart_helper.h"

UART_Handle debug_uart;

int uart_debug_open(){
	static UART_Params uartParams;

	static int initialized = 0;

	if(initialized == 0)
	{
		/* Create a UART with data processing off. */
		UART_Params_init(&uartParams);
		uartParams.writeDataMode = UART_DATA_BINARY;
		uartParams.readDataMode = UART_DATA_BINARY;
		uartParams.readReturnMode = UART_RETURN_FULL;
		uartParams.readEcho = UART_ECHO_OFF;
		uartParams.baudRate = 9600;
		//uartParams.readMode = UART_MODE_BLOCKING;
		//uartParams.readTimeout = 10;
		//uartParams.dataLength = UART_LEN_8;

		//Correct port for the mainboard
		debug_uart = UART_open(Board_UART_debug, &uartParams);

		if (debug_uart == NULL)
			return 0;

		initialized = 1;
	}

	return 1;
}

size_t uart_serial_write(UART_Handle *dev, const uint8_t *data, unsigned int n)
{
	return UART_write(debug_uart, data, n);
}

size_t uart_serial_read(UART_Handle *dev, uint8_t *data, unsigned int n)
{
	return UART_read(*dev, data, n);
}

int uart_serial_putc(UART_Handle *dev, uint8_t c)
{
	UART_write(*dev, &c, 1);
	return 0;
}

int uart_serial_getc(UART_Handle *dev)
{
	uint8_t c;
	if (UART_read(*dev, &c, 1) == 1) {
		return c;
	}
	return SERIAL_EOF;
}



