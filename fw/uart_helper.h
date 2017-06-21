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

#define UART_BUFFER_SIZE 50

extern UART_Handle debug_uart;

int uart_debug_open();

size_t uart_serial_write(UART_Handle *dev, const uint8_t *data, unsigned int n);

size_t uart_serial_read(UART_Handle *dev, uint8_t *data, unsigned int n);

int uart_serial_putc(UART_Handle *dev, uint8_t c);

int uart_serial_getc(UART_Handle *dev);




#endif /* FW_UART_HELPER_H_ */
