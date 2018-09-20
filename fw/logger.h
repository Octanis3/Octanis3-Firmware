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
int log_write_new_entry(uint8_t logchar, uint16_t value);
int log_write_new_rfid_entry(uint64_t uid);
int log_write_new_weight_entry(uint8_t logchar, uint32_t weight, uint16_t stdev);

void log_send_data_via_uart(unsigned int* FRAM_read_end_ptr);
void log_send_lb_state();
void log_send_PIR(unsigned int pin);

uint8_t log_phase_two();

void log_Task();

#endif /* FW_LOGGER_H_ */
