/*
 * ST95HF.H
 *
 *  Created on: 04 Apr 2017
 *      Author: raffael
 */

#ifndef FW_ST95HF_H_
#define FW_ST95HF_H_

#include "../Board.h"

void st95_init_spi();
int st95_startup();


int st95_echo();

void spi_sel();
void spi_unsel();

UChar spi_send_byte(UChar command);

UChar spi_poll();

#endif /* FW_ST95HF_H_ */
