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
int log_write_new_entry(uint32_t uid);

int log_check_ID(uint32_t uid);

void log_send_data_via_uart();

void log_open_door();

#endif /* FW_LOGGER_H_ */
