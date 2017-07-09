/*
 * logger.c
 *
 *  Created on: 08 Jul 2017
 *      Author: raffael
 */

#include "logger.h"
#include "uart_helper.h"

#define LOG_POS_VALID_PW		0x1234 	// write this value to the LOG_NEXT_POS_VALID space in memory
									// at the first time we make a log entry to this FRAM

#define LOG_NEXT_POS_VALID	0x11FFC // store the 16bit "password" (type unsigned int == uint16_t)
#define LOG_NEXT_POS_OFS		0x11FFE // store the 16bit offset (type unsigned int == uint16_t)
#define LOG_START_POS		0x12000
#define LOG_END_POS			0x13FFE // this is the last byte position to write to
/* Note: the allocated storage space is 8 kB large, which is enough for 819 entries */


#define LOG_TIME_OFS			0x0 // 32 bit epoch timestamp
#define LOG_UID_OFS			0x4 // 32 bit UID
#define LOG_DIR_OFS			0x8 // 1 byte "I", "O" or "U" in/out/unknown
#define LOG_CRC_OFS			0x9 // 1 byte CRC (for int16 alignment)

#define LOG_ENTRY_LEN		0xA


#define OUTPUT_BUF_LEN		10 // to send a 32bit value as string
#define OUTPUT_HEX_LEN		8  // to send a 32bit value as hex string

static int ui2a(unsigned long num, unsigned long base, int uc,uint8_t* buffer);
int intToStr(unsigned long x, uint8_t* buffer, int d);

unsigned int* FRAM_offset_ptr;

void log_startup()
{
	FRAM_offset_ptr = (unsigned int*)LOG_NEXT_POS_OFS;

	unsigned int* FRAM_pw = (unsigned int*)LOG_NEXT_POS_VALID;
	if((*FRAM_pw) != LOG_POS_VALID_PW)
	{
		// new initialization
		*FRAM_offset_ptr = 0x0000;

		// store correct password
		(*FRAM_pw) = LOG_POS_VALID_PW;
	}

}

int log_write_new_entry(uint32_t timestamp, uint32_t uid, uint8_t inout)
{
	FRAM_offset_ptr = (unsigned int*)LOG_NEXT_POS_OFS; // pointer should be already initialized at startup, but just to be sure...
	unsigned int* FRAM_write_ptr = (unsigned int*)(LOG_START_POS + *FRAM_offset_ptr); // = base address plus *FRAM_offset_ptr


	// Make sure we are not going to exceeding the reserved memory region. If we do we
	// will not write to the memory
	if (FRAM_write_ptr > (unsigned int*)LOG_END_POS-LOG_ENTRY_LEN)
	{
		return 0; //no available space --> 0 bytes written!
	}
	// else, continue...

	*((uint32_t*)FRAM_write_ptr+LOG_TIME_OFS) = timestamp;
	*((uint32_t*)FRAM_write_ptr+LOG_UID_OFS) = uid;

	unsigned char inout_c = 'U';
	if(inout == 1)
		inout_c = 'I';
	else if(inout == 0)
		inout_c = 'O';

	*((unsigned char*)FRAM_write_ptr+LOG_DIR_OFS) = inout_c;
	//TODO: calculate some sort of CRC.

	*FRAM_offset_ptr += LOG_ENTRY_LEN;                 // Increment write index

	return LOG_ENTRY_LEN;
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
	unsigned int* FRAM_read_ptr = (unsigned int*)(LOG_START_POS); // points to start of logged data.

	uint8_t outbuffer[OUTPUT_BUF_LEN];
	while(FRAM_read_ptr < FRAM_read_end_ptr)
	{
		//send out time stamp:
		intToStr(*((uint32_t*)FRAM_read_ptr+LOG_TIME_OFS), outbuffer, OUTPUT_BUF_LEN);
		uart_serial_write(&debug_uart, outbuffer, OUTPUT_BUF_LEN);
		uart_serial_putc(&debug_uart, ',');

		//send out UID and I/O:
		ui2a(*((uint32_t*)FRAM_read_ptr+LOG_UID_OFS), 16, 'A', outbuffer);
		outbuffer[OUTPUT_HEX_LEN] = ',';
		outbuffer[OUTPUT_HEX_LEN] = *((uint8_t*)FRAM_read_ptr+LOG_DIR_OFS);
		uart_serial_write(&debug_uart, outbuffer, OUTPUT_BUF_LEN);
		uart_serial_putc(&debug_uart, '\n');

		//increment pointer to next memory location
		FRAM_read_ptr += LOG_ENTRY_LEN;
	}

	//TODO: optional, reset the *LOG_NEXT_POS_OFS to zero.

}


static int ui2a(unsigned long num, unsigned long base, int uc,uint8_t* buffer)
{
    int n=0;
    unsigned int d=1;
    while (num/d >= base)
        d*=base;
    while (d!=0) {
        int dgt = num / d;
        num%= d;
        d/=base;
        if (n || dgt>0 || d==0) {
            *buffer++ = dgt+(dgt<10 ? '0' : (uc ? 'A' : 'a')-10);
            ++n;
            }
        }
    return n;
}

// Converts a given integer x to string str[].  d is the number
// of digits required in output. If d is more than the number
// of digits in x, then 0s are added at the beginning.
int intToStr(unsigned long x, uint8_t* buffer, int d)
{
	buffer += d;
	while (x)
	{
		*buffer-- = (x%10) + '0';
		x = x/10;

		d--;
	}

   // If number of digits required is more, then
   // add 0s at the beginning
   while (d > 0)
   {
	   *buffer-- = '0';
	   d--;
   }

	return x;
}

