/*
 * logger.c
 *
 *  Created on: 08 Jul 2017
 *      Author: raffael
 */

#include "logger.h"
#include "uart_helper.h"
#include "../Board.h"
#include <ti/sysbios/hal/Seconds.h>

#define LOG_POS_VALID_PW		0x1234 	// write this value to the LOG_NEXT_POS_VALID space in memory
									// at the first time we make a log entry to this FRAM

#define LOG_BACKUP_PERIOD	2		// seconds between two time stamp back-ups

#define LOG_NEXT_POS_VALID	0x11FFC // store the 16bit "password" (type unsigned int == uint16_t)
#define LOG_NEXT_POS_OFS		0x11FFE // store the 16bit offset (type unsigned int == uint16_t)
#define LOG_TIMESTAMP		0x11FF8 // store the 32bit timestamp (type unsigned int == uint16_t)
							// ^--- RESERVED SPACE STARTS HERE!! CHANGE nestbox_memory_map.cmd FILE IF MODIFYING THIS VALUE!
#define LOG_START_POS		0x00012000
#define LOG_END_POS			0x00013FF0 // this is the last byte position to write to; conservative...
/* Note: the allocated storage space is 8 kB large, which is enough for 819 entries */


#define LOG_UID_64b_OFS				0x0 // 64 bit UID
#define LOG_UID_32b_MSB_OFS			0x0 // 64 bit UID
#define LOG_UID_32b_LSB_OFS			0x1 // 64 bit UID

#define LOG_TIME_32b_OFS				0x2 // 32 bit epoch timestamp
#define LOG_DIR_8b_OFS				0xC // 1 byte "I", "O" or "U" in/out/unknown
#define LOG_CRC_8b_OFS				0xD // 1 byte CRC (for int16 alignment)

#define LOG_ENTRY_8b_LEN		0xE
#define LOG_ENTRY_16b_LEN	0x7


#define OUTPUT_BUF_LEN		LOG_ENTRY_8b_LEN+2+2 // adding two ',' and two digits for decimal time stamp rep.

unsigned int* FRAM_offset_ptr;
unsigned int* FRAM_read_ptr;

unsigned int log_initialized = 0;

void log_startup()
{
	FRAM_offset_ptr = (unsigned int*)LOG_NEXT_POS_OFS;

	unsigned int* FRAM_pw = (unsigned int*)LOG_NEXT_POS_VALID;
	if(((*FRAM_pw) != LOG_POS_VALID_PW) || (GPIO_read(Board_button)==0))
	{
		// new initialization
		*FRAM_offset_ptr = 0x0000;

		// store correct password
		(*FRAM_pw) = LOG_POS_VALID_PW;

		//reset time stamp:
		(*(uint32_t*)LOG_TIMESTAMP) = Seconds_get();
	}
	else
	{
		//recover time stamp, except if user button is pressed --> full reset.
		Seconds_set((*(uint32_t*)LOG_TIMESTAMP) + (LOG_BACKUP_PERIOD/2));
		//LOG_BACKUP_PERIOD/2: add the average time difference that got lost due to backup interval
	}
	log_initialized = 1;
}

int log_write_new_entry(uint32_t timestamp, uint64_t uid, uint8_t inout)
{
	FRAM_offset_ptr = (unsigned int*)LOG_NEXT_POS_OFS; // pointer should be already initialized at startup, but just to be sure...
	unsigned int* FRAM_write_ptr = (unsigned int*)(LOG_START_POS + *FRAM_offset_ptr); // = base address plus *FRAM_offset_ptr


	// Make sure we are not going to exceeding the reserved memory region. If we do we
	// will not write to the memory
	if (FRAM_write_ptr > (unsigned int*)LOG_END_POS-LOG_ENTRY_8b_LEN)
	{
		return 0; //no available space --> 0 bytes written!
	}
	// else, continue...

	*((uint32_t*)FRAM_write_ptr+LOG_UID_32b_MSB_OFS) = uid>>32;
	*((uint32_t*)FRAM_write_ptr+LOG_UID_32b_LSB_OFS) = 0xffffffff & uid;

	*((uint32_t*)FRAM_write_ptr+LOG_TIME_32b_OFS) = timestamp;

	unsigned char inout_c = 'U';
	if(inout == 1)
		inout_c = 'I';
	else if(inout == 0)
		inout_c = 'O';

	*((unsigned char*)FRAM_write_ptr+LOG_DIR_8b_OFS) = inout_c;
	//TODO: calculate some sort of CRC.

	*FRAM_offset_ptr += LOG_ENTRY_8b_LEN;                 // Increment write index

	return LOG_ENTRY_8b_LEN;
}

void log_send_lb_state()
{
	if(GPIO_read(nbox_lightbarrier_int)==1)
		uart_serial_putc(&debug_uart, '1');
	else
		uart_serial_putc(&debug_uart, '0');

}


void log_send_data_via_uart()
{

	/********* possible example code for fast DMA transfer **********
	// Set up DMA0, Repeated single transfer, length = index, UART trigger, transmit bytes
	DMA_initParam dma_param = {0};
	dma_param.channelSelect = DMA_CHANNEL_0;
	dma_param.transferModeSelect = DMA_TRANSFER_REPEATED_SINGLE;
	dma_param.transferSize = *FRAM_write_index;
	dma_param.triggerSourceSelect = DMA_TRIGGERSOURCE_15;
	dma_param.transferUnitSelect = DMA_SIZE_SRCBYTE_DSTBYTE;
	dma_param.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;
	DMA_init(&dma_param);

	// Transfer from ADC_results in FRAM, increment
	DMA_setSrcAddress(DMA_CHANNEL_0, (unsigned long)FRAM_ADC_RESULTS + 2,
					  DMA_DIRECTION_INCREMENT);

	// Transfer to TX buffer for UART (UCA0), unchanged
	DMA_setDstAddress(DMA_CHANNEL_0, (unsigned long)&UCA0TXBUF,
					  DMA_DIRECTION_UNCHANGED);
	DMA_enableTransfers(DMA_CHANNEL_0);
	DMA_enableInterrupt(DMA_CHANNEL_0);

	// Go to sleep until transfer finishes
	__bis_SR_register(LPM0_bits + GIE);
	__no_operation();

	EUSCI_A_UART_disable(__MSP430_BASEADDRESS_EUSCI_A0__);                      // Stop UART (UCA0)
	DMA_disableTransfers(DMA_CHANNEL_0);                                        // disable DMA


	************************ end example *************************/

	uart_debug_open();

	unsigned int* FRAM_read_end_ptr = (unsigned int*)(LOG_START_POS + *FRAM_offset_ptr); //points to the end of the valid stored data
	FRAM_read_ptr = (unsigned int*)LOG_START_POS; // points to start of logged data.

	uint8_t outbuffer[OUTPUT_BUF_LEN];
	while(FRAM_read_ptr < FRAM_read_end_ptr)
	{
		//send out time stamp:
		int strlen = ui2a(*((uint32_t*)FRAM_read_ptr+LOG_TIME_32b_OFS), 10, 1, HIDE_LEADING_ZEROS, outbuffer);
		uart_serial_write(&debug_uart, outbuffer, strlen);
		uart_serial_putc(&debug_uart, ',');

		//send out UID and I/O:
		strlen = ui2a(*((uint32_t*)FRAM_read_ptr+LOG_UID_32b_MSB_OFS), 16, 1,HIDE_LEADING_ZEROS, outbuffer); //the first 32 bits
		uart_serial_write(&debug_uart, outbuffer, strlen);
		strlen = ui2a(*((uint32_t*)FRAM_read_ptr+LOG_UID_32b_LSB_OFS), 16, 1,PRINT_LEADING_ZEROS, outbuffer); //the second 32 bits
		outbuffer[strlen] = ',';
		outbuffer[strlen+1] = *((uint8_t*)FRAM_read_ptr+LOG_DIR_8b_OFS);
		uart_serial_write(&debug_uart, outbuffer, strlen+2);
		uart_serial_putc(&debug_uart, '\n');

		//increment pointer to next memory location
		FRAM_read_ptr += LOG_ENTRY_16b_LEN;
	}

	//TODO: optional, reset the *LOG_NEXT_POS_OFS to zero.

}

//called periodically
Void cron_quick_clock(UArg arg){
	// flash led
//	GPIO_toggle(Board_led_green); // use red led for user inputs

	// back up time stamp
	if(log_initialized)
		(*(uint32_t*)LOG_TIMESTAMP) = Seconds_get();
}


