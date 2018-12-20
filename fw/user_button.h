/*
 * user_button.h
 *
 *  Created on: 02 Apr 2017
 *      Author: raffael
 */

#ifndef FW_USER_BUTTON_H_
#define FW_USER_BUTTON_H_

void user_button_Task();
int user_wifi_enabled();

void wifi_sense_isr(unsigned int index);

void user_button_isr(unsigned int index);

#endif /* FW_USER_BUTTON_H_ */
