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
#include <msp430.h>
#include "rfid_reader.h"

#include "ADS1220/spi.h"



#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

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

// offsets of the second block for weight measurement records:
#define LOG_WEIGHT_16b_OFS			0x0 // weight measurement number goes here
#define LOG_STDDEV_16b_OFS			0x1 // tolerance of the weight measurement here
#define LOG_TEMP_16b_OFS				0x2 // temperature at time of the measurement goes here
#define LOG CRC2_16b_OFS				0x3 // CRC of the weight measurement data goes here

#define LOG_ENTRY_8b_LEN				0xE
#define LOG_ENTRY_WEIGHT_8b_LEN		0x8 // length of the second part of the logging (storing the weight measurement)

#define LOG_ENTRY_16b_LEN			0x7
#define LOG_ENTRY_WEIGHT_16b_LEN		0x4 // length of the second part of the logging (storing the weight measurement)


#define T_PHASE_2			518400 //after 6 days, all events get logged

#define OUTPUT_BUF_LEN		LOG_ENTRY_8b_LEN+2+2 // adding two ',' and two digits for decimal time stamp rep. +5 for weight

unsigned int* FRAM_offset_ptr;
unsigned int* FRAM_read_ptr;

unsigned int log_initialized = 0;
const unsigned int phase_two = 0;

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

	// compensate for LFX deviation:
    RTCCTL01 = RTCHOLD;

	//compensate for 11.1ppm = 5*2.17 --> 5 = 0b101
	RTCCTL23 = RTCCAL2 + RTCCAL0;
	RTCCTL23 &= ~RTCCALS;

    RTCCTL01 &= ~(RTCHOLD);                 // Start RTC

	log_initialized = 1;
}

int log_write_new_entry(uint32_t timestamp, uint64_t uid, uint8_t inout, uint16_t weight, uint16_t stdev, uint16_t temp)
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
	else if(inout == 'W')
		inout_c = 'W';
	else if(inout == 'X')
		inout_c = 'X';

	*((unsigned char*)FRAM_write_ptr+LOG_DIR_8b_OFS) = inout_c;
	//TODO: calculate some sort of CRC.

	*FRAM_offset_ptr += LOG_ENTRY_8b_LEN;                 // Increment write index //TODO: why not the 16bit value??

	if(inout_c == 'W' || inout_c == 'X')
	{
		FRAM_write_ptr = (unsigned int*)(LOG_START_POS + *FRAM_offset_ptr); // = base address plus *FRAM_offset_ptr
		//TODO: on the next block, write down the weight value.
		*((uint16_t*)FRAM_write_ptr+LOG_WEIGHT_16b_OFS) = weight;
		*((uint16_t*)FRAM_write_ptr+LOG_STDDEV_16b_OFS) = stdev;
		*((uint16_t*)FRAM_write_ptr+LOG_TEMP_16b_OFS) = temp;

		*FRAM_offset_ptr += LOG_ENTRY_WEIGHT_8b_LEN;                 // Increment write index

		return LOG_ENTRY_8b_LEN + LOG_ENTRY_WEIGHT_8b_LEN;
	}

	return LOG_ENTRY_8b_LEN;
}

void log_send_lb_state()
{
	if(GPIO_read(nbox_lightbarrier_int)==1)
		uart_serial_putc(&debug_uart, '1');
	else
		uart_serial_putc(&debug_uart, '0');

}

const uint8_t start_string[] = "#=========== start FRAM logs =========\n";
const uint8_t title_row[] = "time [s],RFID UID,event type, weight [g], tolerance [mg], temperature [1/10 K]\n";
const uint8_t end_string[] = "#========== end FRAM logs ===========\n";
const uint8_t phase_two_string[] = "#========== STARTING PHASE TWO =========\n";


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
	uart_stop_debug_prints();
	uart_debug_open();

	uart_serial_write(&debug_uart, start_string, sizeof(start_string));
	uart_serial_write(&debug_uart, title_row, sizeof(title_row));

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
		// TODO: treat weight and light barrier event differently
		uart_serial_write(&debug_uart, outbuffer, strlen+2);

		//increment pointer to next memory location
		FRAM_read_ptr += LOG_ENTRY_16b_LEN;

		if(outbuffer[strlen+1] == 'W' || outbuffer[strlen+1] == 'X') //this is a weight measurement record --> continue reading
		{
			uart_serial_putc(&debug_uart, ',');
			//print weight
			strlen = ui2a(*((uint16_t*)FRAM_read_ptr+LOG_WEIGHT_16b_OFS), 10, 1, HIDE_LEADING_ZEROS, outbuffer);
			outbuffer[strlen] = ',';
			uart_serial_write(&debug_uart, outbuffer, strlen+1);
			//print stdev
			strlen = ui2a(*((uint16_t*)FRAM_read_ptr+LOG_STDDEV_16b_OFS), 10, 1, HIDE_LEADING_ZEROS, outbuffer);
			outbuffer[strlen] = ',';
			uart_serial_write(&debug_uart, outbuffer, strlen+1);
			//print temperature
			strlen = ui2a(*((uint16_t*)FRAM_read_ptr+LOG_TEMP_16b_OFS), 10, 1, HIDE_LEADING_ZEROS, outbuffer);
			uart_serial_write(&debug_uart, outbuffer, strlen);

			//increment pointer to next memory location
			FRAM_read_ptr += LOG_ENTRY_WEIGHT_16b_LEN;
		}

		uart_serial_putc(&debug_uart, '\n');

	}

	uart_serial_write(&debug_uart, end_string, sizeof(end_string));
	uart_start_debug_prints();

	//TODO: optional, reset the *LOG_NEXT_POS_OFS to zero.

}

const uint8_t PIR_trigger_in[] = "PIR inside triggered\n";
const uint8_t PIR_trigger_out[] = "PIR outside triggered\n";

void log_send_PIR(unsigned int pin) {
    uint8_t log_message[] = (pin != 0 && pin == nbox_pir_in1) ? PIR_trigger_in : PIR_trigger_out;
    uart_serial_write(&debug_uart, log_message, sizeof(log_message));
}

uint8_t log_phase_two()
{
	return 0;//phase_two>0;
}

//called periodically
Void cron_quick_clock(UArg arg){
	// flash led
//	GPIO_toggle(Board_led_green); // use red led for user inputs

	// back up time stamp
	if(log_initialized)
		(*(uint32_t*)LOG_TIMESTAMP) = Seconds_get();

//	if(Seconds_get() > T_PHASE_2)
//	{
//		if(phase_two == 0)
//		{
//			phase_two = 2;
//			rfid_start_detection();
//		}
//	}
}


extern SPI_Handle  nestbox_spi_handle;
int sd_spi_is_initialized = 0;

Bool sd_spi_send_command(unsigned char cmd, uint32_t arg, uint8_t crc, uint8_t* response, unsigned int readlen)
{
    SPI_Transaction     spiTransaction;
    Bool                transferOK;

    uint8_t txBuf[6];
    uint8_t rxBuf[6];

    txBuf[0] = cmd;
    txBuf[1] = arg>>24;
    txBuf[2] = arg>>16;
    txBuf[3] = arg>>8;
    txBuf[4] = arg;
    txBuf[5] = crc;

    // send command: "init and go to SPI mode":
    // Init and go to SPI mode: ]r:10 [0x40 0x00 0x00 0x00 0x00 0x95 r:8]

    spiTransaction.count = 6;
    spiTransaction.txBuf = txBuf;
    spiTransaction.rxBuf = rxBuf;

    spi_slave_select(nbox_spi_cs_n);
    transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);

    // receive response:
    spiTransaction.count = 1;
    txBuf[0] = 0xff;
    spiTransaction.txBuf = txBuf;
    spiTransaction.rxBuf = rxBuf;

    unsigned int i=0;
    for(i = 0; i<readlen; i++)
    {
        SPI_transfer(nestbox_spi_handle, &spiTransaction);
        response[i] = rxBuf[0];
    }
    spi_slave_unselect(nbox_spi_cs_n);

    return transferOK;
}

#define CMD8    0x48
#define CMD16   0x50
#define CMD17   0x51
#define CMD55   0x77
#define CMD58   0x7A
#define ACMD41  0x69 // must be preceded by CMD55 !

/* inspired by http://codeandlife.com/2012/04/25/simple-fat-and-sd-tutorial-part-3/ */
int sd_spi_init()
{
    SPI_Transaction     spiTransaction;
    Bool                transferOK;
    uint8_t     txBuf[14] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    uint8_t     rxBuf[14] = {0xff,};

    unsigned int i = 0;
    // send command: "init and go to SPI mode":
    // Init and go to SPI mode: ]r:10 [0x40 0x00 0x00 0x00 0x00 0x95 r:8]

    spiTransaction.count = 8;
    spiTransaction.txBuf = txBuf;
    spiTransaction.rxBuf = rxBuf;

    spi_slave_unselect(nbox_spi_cs_n);
    transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
    if (!transferOK) {
        return 0;/* Error in SPI transfer or transfer is already in progress */
    }
    memcpy(txBuf, (const unsigned char[]){0x40, 0x00, 0x00, 0x00, 0x00, 0x95, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, sizeof txBuf);


    for(i=0; i<10; i++)
    {
        spiTransaction.count = 14;
        spiTransaction.txBuf = txBuf;
        spiTransaction.rxBuf = rxBuf;

        spi_slave_select(nbox_spi_cs_n);
        transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
        spi_slave_unselect(nbox_spi_cs_n);
        if (!transferOK) {
            return 0;/* Error in SPI transfer or transfer is already in progress */
        }
        if(rxBuf[7] == 1) // TODO: change to check all bytes!
            break;
        Task_sleep(100);
    }

    if(i==10)
        return -1;

    // CMD8 to read version of SD card
    // according to https://openlabpro.com/guide/interfacing-microcontrollers-with-sd-card/
    sd_spi_send_command(CMD8, 0x000001AA, 0x87, rxBuf, 4);

//    memcpy(txBuf, (const unsigned char[]){CMD8, 0x00, 0x00, 0x01, 0xAA, 0x87, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, sizeof txBuf);
//
//
//    for(i=0; i<10; i++)
//    {
//        spiTransaction.count = 14;
//        spiTransaction.txBuf = txBuf;
//        spiTransaction.rxBuf = rxBuf;
//
//        spi_slave_select(nbox_spi_cs_n);
//        transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
//        spi_slave_unselect(nbox_spi_cs_n);
//        if (!transferOK) {
//            return 0;/* Error in SPI transfer or transfer is already in progress */
//        }
//        Task_sleep(100);
//    }

    for(i=0; i<10; i++)
    {
        sd_spi_send_command(CMD55, 0, 0x01, rxBuf, 4);
        sd_spi_send_command(ACMD41, 0x40000000, 0x01, rxBuf, 4);
        if(rxBuf[0] == 0 || rxBuf[1] == 0 || rxBuf[2] == 0 || rxBuf[3] == 0)
            break;
    }

    if(i == 10)
        return -2;

//    // Initialize card: [0x41 0x00 0x00 0x00 0x00 0xFF r:8]
//    memcpy(txBuf, (const unsigned char[]){0x41, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, sizeof txBuf);
//    spiTransaction.count = 14;
//    spiTransaction.txBuf = txBuf;
//    spiTransaction.rxBuf = rxBuf;
//    spi_slave_select(nbox_spi_cs_n);
//    transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
//    spi_slave_unselect(nbox_spi_cs_n);
//
//    if(rxBuf[6] == 0 || rxBuf[7] == 0 || rxBuf[8] == 0 || rxBuf[9] == 0 || rxBuf[10] == 0 || rxBuf[11] == 0 || rxBuf[12] == 0 || rxBuf[13] == 0)
//    {
//        //good response
//        sd_spi_is_initialized = 1;
//    }
//    else
//    {
//        unsigned char response = 0xff;
//        spiTransaction.count = 1;
//        spiTransaction.txBuf = &(txBuf[8]);
//        spiTransaction.rxBuf = &response;
//
//        for(i=0; i<100; i++)
//        {
//           spi_slave_select(nbox_spi_cs_n);
//           transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
//           spi_slave_unselect(nbox_spi_cs_n);
//           if(response == 0x00)
//           {
//               sd_spi_is_initialized = 1;
//               break;
//           }
//           Task_sleep(10);
//        }
//    }
//    spi_slave_unselect(nbox_spi_cs_n);
//
//    if(i==100)
//        return -2;

    //Set transfer size: [0x50 0x00 0x00 0x02 0x00 0xFF r:8]
    sd_spi_send_command(CMD16, 0x00000200, 0xff, rxBuf, 6);

//    memcpy(txBuf, (const unsigned char[]){0x50, 0x00, 0x00, 0x02, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, sizeof txBuf);
//    spiTransaction.count = 14;
//    spiTransaction.txBuf = txBuf;
//    spiTransaction.rxBuf = rxBuf;
//
//    spi_slave_select(nbox_spi_cs_n);
//    transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
//    spi_slave_unselect(nbox_spi_cs_n);

    int j=0;
    for (j=0; j<100; j++) //read 100 sectors
    {


        //Read sector: [0x51 0x00 0x00 0x00 0x00 0xFF r:520]
        sd_spi_send_command(CMD17, j, 0xff, rxBuf, 1);

          memcpy(txBuf, (const unsigned char[]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, 8);
    //    spiTransaction.count = 14;
    //    spiTransaction.txBuf = txBuf;
    //    spiTransaction.rxBuf = rxBuf;
    //
    //    spi_slave_select(nbox_spi_cs_n);
    //    transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
    //    //TODO: wait for 0x00 response!

        spiTransaction.txBuf = txBuf;
        spiTransaction.rxBuf = rxBuf;
        spiTransaction.count = 1; //read 1 Byte at a time until 0 is detected

        spi_slave_select(nbox_spi_cs_n);

        //wait for a 0x00 to arrive
        for(i=0; i<10; i++)
        {
            transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
            if(rxBuf[0] == 0)
                break;
        }

        //wait for a 0xFE to arrive
        for(i=0; i<10; i++)
        {
            transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
            if(rxBuf[0] == 0xFE)
                break;
        }

        for(i=0; i<64; i++)
        {
            spiTransaction.count = 8; //read 8 Byte at a time
            spiTransaction.txBuf = txBuf;
            spiTransaction.rxBuf = rxBuf;
            spi_slave_select(nbox_spi_cs_n);
            transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
            spi_slave_unselect(nbox_spi_cs_n);

            uart_serial_write(&debug_uart, rxBuf, 8);
        }
        Task_sleep(100);
    }

    if (!transferOK) {
        return 0;/* Error in SPI transfer or transfer is already in progress */
    }
    return 1;
}

void log_Task()
{
	Task_sleep(5000); //wait until UART is initialized
	uart_start_debug_prints();

    Task_sleep(10000); //wait until UART is initialized

	sd_spi_init();

	while(1)
	{
		// print status of light barrier
//		uint8_t stat = '0' + 2*GPIO_read(nbox_lightbarrier_int) + GPIO_read(nbox_lightbarrier_ext);
//		uart_serial_print_event('S', &stat, 1);
//		Semaphore_post((Semaphore_Handle)semSerial);
		Task_sleep(1000);

//		if(phase_two == 2)
//		{
//			uart_serial_write(&debug_uart, phase_two_string, sizeof(phase_two_string));
//			phase_two = 1;
//		}

//		Semaphore_pend((Semaphore_Handle)semSerial,BIOS_WAIT_FOREVER);
	}
}


