/*
 * uart_helper.h
 *
 *  Created on: 17 Jun 2017
 *      Author: raffael
 */

#ifndef FW_UART_HELPER_H_
#define FW_UART_HELPER_H_

#include <ti/drivers/UART.h>

#define SERIAL_EOF      -1

//leading zeros only works for hexadecimal base!!!
#define PRINT_LEADING_ZEROS 1
#define HIDE_LEADING_ZEROS 0

#define UART_BUFFER_SIZE 50

extern UART_Handle debug_uart;
extern UART_Handle wifi_uart;

int uart_debug_open();
void uart_debug_close();

int wifi_debug_open();
void wifi_debug_close();

size_t uart_serial_write(UART_Handle *dev, const uint8_t *data, unsigned int n);

size_t uart_serial_read(UART_Handle *dev, uint8_t *data, unsigned int n);

int uart_serial_putc(UART_Handle *dev, uint8_t c);

int uart_serial_getc(UART_Handle *dev);

void uart_start_debug_prints();
void uart_stop_debug_prints();

void uart_serial_print_event(char type, const uint8_t* data, unsigned int n);

int ui2a(unsigned long num, unsigned long base, int uc, int leading_zeros,uint8_t* buffer);
int intToStr(unsigned long x, uint8_t* buffer, int d);



#endif /* FW_UART_HELPER_H_ */
