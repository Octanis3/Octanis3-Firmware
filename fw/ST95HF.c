/*
 * ST95HF.c
 *
 *  Created on: 04 Apr 2017
 *      Author: raffael
 */

#include "ST95HF.h"
#include "Board.h"


// global handle, to be used to perform a spi_read/write action
SPI_Handle  st95_spi;


void init_st95_spi()
{
	// initialize
	SPI_Params  spiParams;
	SPI_Params_init(&spiParams);
	spiParams.transferMode = SPI_MODE_BLOCKING;
	spiParams.transferCallbackFxn = NULL;
	spiParams.frameFormat = SPI_POL0_PHA0; // The ST95HF supports (CPOL = 0, CPHA = 0) and (CPOL = 1, CPHA = 1) modes.
	spiParams.mode = SPI_MASTER;
	// spiParams.bitRate = ????; (take default?) /*!< SPI bit rate in Hz */
	// spiParams.dataSize = ????; /*!< SPI data frame size in bits */

	st95_spi = SPI_open(Board_SPI0, &spiParams);
	if (st95_spi == NULL) {
	   /* Error opening SPI */
	}



}

void close_st95_spi()
{
	SPI_close(st95_spi);
}

int init_st95hf()
{
	// I just put an example on how to initialize a transfer. Adapt code accordingly:
	// example for Transferring n 4-8 bit SPI frames:

	unsigned int n = 2; // number of frames to be transferred

	SPI_Transaction  spiTransaction;
	UChar 	transmitBuffer[n];
	UChar 	receiveBuffer[n];
	Bool	 transferOK;

	// if needed, redefine the data size. CLOSE the spi first by calling close_spi()
//	SPI_Params_init(&spiParams);
//	spiParams.dataSize = 6;  /* dataSize can range from 4 to 8 bits */
//	spi = SPI_open(peripheralNum, &spiParams);

	//TODO: fill in frames:
	//transmitBuffer[0] = SOME_COMMAND;
	//transmitBuffer[1] = 0x00; // dummy (if we need to wait for reply, SPI still needs to send something empty at the same time.)

	spiTransaction.count = n;
	spiTransaction.txBuf = transmitBuffer;
	spiTransaction.rxBuf = receiveBuffer;
	transferOK =  SPI_transfer(st95_spi, &spiTransaction);
	if (!transferOK) {
	   /* Error in SPI transfer or transfer is already in progress */
	}



	return 0;
}

