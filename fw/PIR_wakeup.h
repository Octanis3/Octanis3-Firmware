/*
 * PIR_wakeup.h
 *
 *  Created on: 12 Mar 2018
 *      Author: Lennard
 */

#ifndef FW_PIR_WAKEUP_H_
#define FW_PIR_WAKEUP_H_

void PIR_wakeup_Task();

void PIR_wakeup_isr(unsigned int index);

#endif /* FW_PIR_WAKEUP_H_ */
