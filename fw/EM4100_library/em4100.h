/*
 * em4100.h
 *
 *  Created on: 20 Nov 2017
 *      Author: raffael
 */

#ifndef FW_EM4100_LIBRARY_EM4100_H_
#define FW_EM4100_LIBRARY_EM4100_H_

uint8_t readCard();

bool loadCard(uint8_t s,uint8_t card[]);

void saveCard(uint8_t s,uint8_t card[]);

int8_t addCard(uint8_t card[]);

int8_t validateCard(uint8_t card[]);






#endif /* FW_EM4100_LIBRARY_EM4100_H_ */
