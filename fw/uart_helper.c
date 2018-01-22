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

	static int uart_initialized = 0;

	if(uart_initialized == 0)
	{
		/* Create a UART with data processing off. */
		UART_Params_init(&uartParams);
		uartParams.writeDataMode = UART_DATA_BINARY;
		uartParams.readDataMode = UART_DATA_BINARY;
		uartParams.readReturnMode = UART_RETURN_FULL;
		uartParams.readEcho = UART_ECHO_OFF;
		uartParams.baudRate = 115200;
		//uartParams.readMode = UART_MODE_BLOCKING;
		//uartParams.readTimeout = 10;
		//uartParams.dataLength = UART_LEN_8;

		//Correct port for the mainboard
		debug_uart = UART_open(Board_UART_debug, &uartParams);

		if (debug_uart == NULL)
			return 0;


		const char test_string[] = "nestbox UART initialized\n";
		uart_serial_write(&debug_uart, (uint8_t*)test_string, sizeof(test_string));

		uart_initialized = 1;
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

//leading zeros only works for hexadecimal base!!!
int ui2a(unsigned long num, unsigned long base, int uc, int leading_zeros,uint8_t* buffer)
{
    int n=0;
    unsigned long d=1;
    while (num/d >= base)
        d*=base;
    if(leading_zeros)
    {
    		unsigned long tmp = num;
    		while((!(tmp & 0xf0000000)) && (n<sizeof(long)*2-1))
    		{
    			tmp = tmp << 4;
    			*buffer++ = '0';
    			++n;
    		}
    }
    while (d!=0) {
        unsigned long dgt = num / d;
        num%= d;
        d/=base;
        if (n || dgt>0 || d==0) {
            *buffer++ = dgt+(dgt<10 ? '0' : (uc ? 'A' : 'a')-10);
            ++n;
            }
        }
    return n;
}

// Converts a given integer x to string str[].  d is the number
// of digits required in output. If d is more than the number
// of digits in x, then 0s are added at the beginning.
int intToStr(unsigned long x, uint8_t* buffer, int d)
{
	buffer += d;
	while (x)
	{
		*buffer-- = (x%10) + '0';
		x = x/10;

		d--;
	}

   // If number of digits required is more, then
   // add 0s at the beginning
   while (d > 0)
   {
	   *buffer-- = '0';
	   d--;
   }

	return x;
}




