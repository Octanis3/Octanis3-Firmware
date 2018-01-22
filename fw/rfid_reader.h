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
int rfid_get_id(uint64_t* id);

void rfid_start_detection();
void rfid_stop_detection();

void nfc_wakeup_isr();

void lf_tag_read_isr();

#endif /* FW_RFID_READER_H_ */
