/*
 * rfid_reader.h
 *
 *  Created on: 27 Mar 2017
 *      Author: raffael
 */

#ifndef FW_RFID_READER_H_
#define FW_RFID_READER_H_

#include "Board.h"

#define CALIB162_UID    0x59004529B6
#define CALIB595_UID    0x580053A0AF
#define CALIB379_UID    0x5800538D57

#define UID_LENGTH 4
#define TIMESTAMP_LENGTH 4

void rfid_Task();
int rfid_get_id(uint64_t* id);

void rfid_start_detection();
void rfid_stop_detection();
void rfid_reset_detection_counts();


void lf_tag_read_isr();

#endif /* FW_RFID_READER_H_ */
