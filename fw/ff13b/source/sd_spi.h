/*
 * sd_spi.h
 *
 *  Created on: 20 Aug 2018
 *      Author: raffael
 */

#ifndef FW_FF13B_SOURCE_SD_SPI_H_
#define FW_FF13B_SOURCE_SD_SPI_H_

#include "diskio.h"

Bool sd_spi_send_command(unsigned char cmd, uint32_t arg, uint8_t crc, uint8_t* response, unsigned int readlen);



#endif /* FW_FF13B_SOURCE_SD_SPI_H_ */
