/*
 * rfid_reader.h
 *
 *  Created on: 27 Mar 2017
 *      Author: raffael
 */

#ifndef FW_RFID_READER_H_
#define FW_RFID_READER_H_

#include "Board.h"

#define UID_LENGTH 4
#define TIMESTAMP_LENGTH 4

void rfid_Task();
uint32_t rfid_get_id();


void nfc_wakeup_isr();

void lf_data_read();

#endif /* FW_RFID_READER_H_ */
