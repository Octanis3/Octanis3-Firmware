/*
 * logger.h
 *
 *  Created on: 08 Jul 2017
 *      Author: raffael
 */

#ifndef FW_LOGGER_H_
#define FW_LOGGER_H_

#include <stdint.h>

void log_startup();
int log_write_new_entry(uint32_t timestamp, uint64_t uid, uint8_t inout);

void log_send_data_via_uart();
void log_send_lb_state();

uint8_t log_phase_two();

void log_Task();

#endif /* FW_LOGGER_H_ */
