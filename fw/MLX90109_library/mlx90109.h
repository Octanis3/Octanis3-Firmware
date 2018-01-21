/*
 * Copyright (C) 2017 Jonas Radtke <jonas.radtke@haw-hamburg.de> <jonas@radtke.dk>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
 
 /**
 * @defgroup    drivers_MLX90109 Read RFID Animal Tags
 * @ingroup     drivers
 * @brief       Driver for reading ISO 11784 & 11785 FDX-B Animal Tags
 * @{
 *
 * @file
 * @brief       Public interface for the MLX90109 driver.
 *
 * @author      Jonas Radtke <jonas.radtke@haw-hamburg.de> <jonas@radtke.dk>
 */
 
#ifndef MLX90109_H_
#define MLX90109_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

/**
 * @brief   MLX90109 return values
 */
enum {
    MLX90109_OK    = 0,			/**< all went as expected */
    MLX90109_GPIO_FAULT = -1,	/**< error GPIO configuation */
    MLX90109_CRC_NOT_OK = -2,	/**< crc is wrong */
	MLX90109_DATA_OK = -3		/**< Data read complete */
};

/**
 * @brief   MLX90109 device params
 */
typedef struct {
	unsigned int clock;       		/**< GPIO Pin Clock */
	unsigned int data;   		 	  	/**< GPIO Pin Data */
	unsigned int mode;				/**< GPIO Pin Mode */
	unsigned int dataSelect;			/**< GPIO Pin dataSelect / Speed */
	unsigned int modu;				/**< GPIO Pin Modu, RF on/off */
	unsigned int tag_select;		/** is either MLX_TAG_EM4100 or MLX_TAG_FDX */
} mlx90109_params_t;	

/**
 * @brief   MLX90109 device descriptor
 */
typedef struct {
    mlx90109_params_t p; 		/**< device configuation parameter */
	uint8_t counter;	  		/**< counter for data bits*/
	uint8_t counter_header;		/**< counter for Header bits "10000000000"*/
	uint8_t nibble_counter;		/**< counter for the 4-bit groups of the EM4100 */
	uint8_t data[128];			/**< raw data*/
	uint16_t timediff[128];
	uint16_t int_time[128];

	uint16_t last_timestamp;
	uint8_t tagId[10];			/**< EM4100 only: 2x4 version bits + 8x4 data bits*/
	uint8_t id_counter;
} mlx90109_t;

/**
 * @brief   MLX90109 Tag Data
 */
typedef struct {
	uint64_t tagId;				/**< Tag ID */
	uint16_t countryCode;		/**< Country Code of Tag */
	uint16_t dataBlock;			/**< Data Block is used */
	uint16_t animalTag;			/**< For Animal Identification */
	uint8_t checksumData[8];	/**< 64bit for checksum calculation */
	uint16_t checksumArr[2];	/**< Checksum 2byte of 64bit block, from Tag */
	uint16_t checksum16;		/**< 16bit checksum, calcuated */
	uint8_t dataB[3];			/**< for Data on Tag, if DataBlock is 1 */
	uint8_t newTag;
} tagdata;

/**
 * @brief              Initializes the MLX90109 with the given parameters in mlx90109_t structure.
 *                     The init procedure also takes care of initializing the GPIO Pins and Interrupts.
 *
 * @param[out] dev     the device descriptor
 * @param[in] params   parameters for this device (pins and interrupts are initialized by this driver)
 * @param[in] cb	   callback of Interrupt Handler
 *
 * @return             0 initialized successfully
 * @return             false if an error occured while initializing
 */
int16_t mlx90109_init(mlx90109_t *dev, const mlx90109_params_t *params);

/**
 * @brief              Reads the Raw Data from FDX Tag, must used in the GPIO Interrupt.
 *
 * @param[out] dev     the device descriptor
 *
 * @return             0 if a bit could read successfully
 * @return             -3 if the raw data is complete
 */

void mlx90109_activate_reader(mlx90109_t *dev);

void mlx90109_disable_reader(mlx90109_t *dev, tagdata *tag);

int16_t mlx90109_read(mlx90109_t *dev);

/**
 * @brief              Extract the Data from the Raw data of the FDX tag and performs a CRC.
 *
 * @param[out] dev     the device descriptor
 * @param[out] tag     the Tag Data
 *
 * @return             0 if CRC is ok
 * @return             -2 if CRC is not ok
 */
int16_t mlx90109_format(mlx90109_t *dev, tagdata *tag);

/************************************************************************/

/**
 * @brief              Reads the Raw Data from and EM4100 Tag, must used in the GPIO Interrupt.
 *
 * @param[out] dev     the device descriptor
 *
 * @return             0 if a bit could read successfully
 * @return             -3 if the raw data is complete
 */
int16_t em4100_read(mlx90109_t *dev);

#endif /* MLX90109_H_ */
/** @} */
