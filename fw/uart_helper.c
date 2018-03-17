/*
 * uart_helper.c
 *
 *  Created on: 17 Jun 2017
 *      Author: raffael
 */

#include "../Board.h"
#include "uart_helper.h"
#include <xdc/runtime/Timestamp.h>
#include <ti/sysbios/hal/Seconds.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

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

		Semaphore_post((Semaphore_Handle)semSerial);

		uart_initialized = 1;
	}

	return 1;
}

int debug_prints_allowed = 0;
void uart_start_debug_prints()
{
	debug_prints_allowed = 1;
}

void uart_stop_debug_prints()
{
	debug_prints_allowed = 0;
}


const char newline = '\n';
const char zero = '0';
void uart_serial_print_event(char type, const uint8_t* data, unsigned int n)
{
	if(debug_prints_allowed)
	{
		uint32_t t = Timestamp_get32();
		uint32_t seconds = t >> 15;
		uint32_t msecs = (t & 0x7fff) * 1000 /32768;

		uint32_t rtc_sec = Seconds_get();

		uint8_t strlen;
		uint8_t sec_buf[7];
		strlen = ui2a(rtc_sec, 10, 1, HIDE_LEADING_ZEROS, sec_buf);
		sec_buf[strlen]=',';
		UART_write(debug_uart, sec_buf, strlen+1);

		strlen = ui2a(seconds, 10, 1, HIDE_LEADING_ZEROS, sec_buf);
		if(strlen>6)
				strlen = 6;
		sec_buf[strlen]='.';
		UART_write(debug_uart, sec_buf, strlen+1);
		strlen = ui2a(msecs, 10, 1, HIDE_LEADING_ZEROS, sec_buf);
		if(strlen<3)
			UART_write(debug_uart, &zero, 1);
		if(strlen<2)
			UART_write(debug_uart, &zero, 1);
		if(strlen>4)
			strlen = 4;
		sec_buf[strlen]=',';
		sec_buf[strlen+1]=type;
		sec_buf[strlen+2]=',';
		UART_write(debug_uart, sec_buf, strlen+3);
		UART_write(debug_uart, data, n);
		UART_write(debug_uart, &newline, 1);
	}
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




