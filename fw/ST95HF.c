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

// SPI data buffers
UChar spi_transmitBuffer[16];
UChar spi_receiveBuffer[16];


void st95_init_spi()
{
	// initialize
	SPI_Params  spiParams;
	SPI_Params_init(&spiParams);
	spiParams.transferMode = SPI_MODE_BLOCKING;
	spiParams.transferCallbackFxn = NULL;
	spiParams.frameFormat = SPI_POL0_PHA0; // The ST95HF supports (CPOL = 0, CPHA = 0) and (CPOL = 1, CPHA = 1) modes.
	spiParams.mode = SPI_MASTER;
//	spiParams.bitRate = 500000; /*!< SPI bit rate in Hz */ //max can be 2 MHz.
	// spiParams.dataSize = ????; /*!< SPI data frame size in bits (default = 8) */
	// NOTE:  .bitOrder = EUSCI_B_SPI_MSB_FIRST is defined in nestbox_init.


	st95_spi = SPI_open(Board_SPI0, &spiParams);
	if (st95_spi == NULL) {
	   /* Error opening SPI */
//		GPIO_write(Board_led_blue,1);
	}



}

void st95_close_spi()
{
	SPI_close(st95_spi);
}

void spi_sel()
{GPIO_write(Board_nfc_spi_sel_n, 0);}
void spi_unsel()
{GPIO_write(Board_nfc_spi_sel_n, 1);}

#define RELEASE_SEL 	0
#define KEEP_SEL		1
int spi_transfer(unsigned int n, UChar* transmitBuffer, UChar* receiveBuffer, Bool keep_selected)
{
	SPI_Transaction  spiTransaction;
	Bool	 transferOK;

	spiTransaction.count = n;
	spiTransaction.txBuf = transmitBuffer;
	spiTransaction.rxBuf = receiveBuffer;

	spi_sel();
	transferOK = SPI_transfer(st95_spi, &spiTransaction);
	if(keep_selected == 0)
		spi_unsel();

	if (!transferOK) {
	   return 0;/* Error in SPI transfer or transfer is already in progress */
	}

	return 1;
}


//!!! NOTE that SPI_SEL has to be done externally!!
UChar spi_send_byte(UChar command)
{
	unsigned int n = 1; // number of frames to be transferred

	SPI_Transaction  spiTransaction;

	Bool	 transferOK;

	spi_transmitBuffer[0] = command;

	spiTransaction.count = n;
	spiTransaction.txBuf = spi_transmitBuffer;
	spiTransaction.rxBuf = spi_receiveBuffer;
	transferOK =  SPI_transfer(st95_spi, &spiTransaction);
	if (!transferOK) {
	   return 0;/* Error in SPI transfer or transfer is already in progress */
	}

	return spi_receiveBuffer[0];
}

void spi_poll()
{
	UChar answer = 0;
	spi_sel();
	while((answer && 0x08) == 0)
	{
		answer = spi_send_byte(0x03);
	}

	spi_unsel();
}



// inspired from http://blog.solutions-cubed.com/near-field-communication-nfc-with-the-arduino/
int st95_startup()
{
	Task_sleep(11); // 10ms minimum, t4 datasheet (time to power up CR995HF)
						// + 100us min, t0 in datasheet

	GPIO_write(Board_nfc_wakeup_n, 0);
	Task_sleep(1); // 10us minumum, t1 in datasheet
	GPIO_write(Board_nfc_wakeup_n, 1);

	Task_sleep(10); //ready in <10ms, t3 in datasheet

	spi_transmitBuffer[0] = 0x00; //send command
	spi_transmitBuffer[1] = 0x01; //command = IDN

	spi_transfer(3, spi_transmitBuffer, spi_receiveBuffer, RELEASE_SEL);

	Task_sleep(1);
	spi_poll();
	Task_sleep(1);

	spi_transmitBuffer[0] = 0x02; // SPI control byte for read
	spi_transfer(3, spi_transmitBuffer, spi_receiveBuffer, KEEP_SEL);

	UChar response_code = spi_receiveBuffer[1];
	UChar readlength = spi_receiveBuffer[2];

	if ((response_code == 0) & (readlength == 15))
	{
		spi_transfer(readlength, spi_transmitBuffer, spi_receiveBuffer, RELEASE_SEL);

		// received good ID response
		GPIO_write(Board_led_green,1);
	}
	else
	{
	    //BAD RESPONSE TO IDN COMMAND
	}

	return 0;
}

int st95_echo()
{
	return 0;

}


int st95hf_comm_test()
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


