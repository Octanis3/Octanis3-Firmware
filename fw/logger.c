/*
 * logger.c
 *
 *  Created on: 08 Jul 2017
 *      Author: raffael
 */

#include "logger.h"
#include "uart_helper.h"
#include "../Board.h"
#include <xdc/runtime/Timestamp.h>
#include <ti/sysbios/hal/Seconds.h>
#include <msp430.h>
#include "rfid_reader.h"

#include "ADS1220/spi.h"
#include "ff13b/source/ff.h"

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

#define MAX_SD_RETRY        3 // number of times we try to initialize the SD card.

#define LOG_POS_VALID_PW		0x1234 	// write this value to the LOG_NEXT_POS_VALID space in memory
									// at the first time we make a log entry to this FRAM

#define LOG_BACKUP_PERIOD	2		// seconds between two time stamp back-ups

#define LOG_NEXT_POS_VALID	0x12FFC // store the 16bit "password" (type unsigned int == uint16_t)
#define LOG_NEXT_POS_OFS		0x12FFE // store the 16bit offset (type unsigned int == uint16_t)
#define LOG_TIMESTAMP		0x12FF8 // store the 32bit timestamp (type unsigned int == uint16_t)
							// ^--- RESERVED SPACE STARTS HERE!! CHANGE nestbox_memory_map.cmd FILE IF MODIFYING THIS VALUE!
#define LOG_START_POS		0x00013000
#define LOG_END_POS			0x00013FF0 // this is the last byte position to write to; conservative...
#define LOG_END_OFS         (LOG_END_POS - LOG_START_POS)

#define LOG_MIDDLE_POS       ((LOG_END_POS + LOG_START_POS)/2)
#define LOG_MIDDLE_OFS       (LOG_END_POS - LOG_START_POS)/2


/* Note: the allocated storage space is 8 kB large, which is enough for 819 entries */

// general log offsets:
#define LOG_TIME_32b_OFS				0x0 // 32 bit epoch timestamp
#define LOG_CHAR_8b_OFS		        0x4 // 1 byte "R", "X" for long values, "D", "T", "P" for short values
                                                                           //"I", "O" or "U" in/out/unknown

// offsets for short log entries:
#define LOG_ENTRY_SHORT_8b_LEN      0x8
#define LOG_ENTRY_SHORT_16b_LEN     0x4 // length of the second part of the logging (storing the weight measurement)

#define LOG_VALUE_SHORT_16b_OFS     0x3 // for short entries: value
#define LOG_CRC_8b_OFS              0x5 // 1 byte for rounded milliseconds or CRC

// offsets for long log entries:
#define LOG_ENTRY_LONG_8b_LEN       0xC // length of a long log entry (storing the exact weight or UID)
#define LOG_ENTRY_LONG_16b_LEN      0x6

#define LOG_UID_32b_MSB_OFS         0x1 // 64 bit UID
#define LOG_UID_32b_LSB_OFS         0x2 // 64 bit UID
#define LOG_VALUE_LONG_32b_OFS      0x2 // for long weight entries: value (32bit)
#define LOG_STDDEV_16b_OFS          0x3 // tolerance of the weight measurement here
#define LOG_MSEC_8b_OFS             0x5 // 1 byte for rounded milliseconds or CRC


//#define T_PHASE_2			518400 //after 6 days, all events get logged

#define OUTPUT_BUF_LEN		LOG_ENTRY_LONG_8b_LEN*2+4 // adding 4 ',' and twice the digits for decimal values

// variable used to write next log entry
uint16_t* FRAM_offset_ptr;

// variables used to read out the log buffer:
uint16_t* FRAM_read_ptr;
uint16_t FRAM_read_end_ptr_value;

// in which half of the storage region will the next flush out happen?
enum log_partitions {FIRST = 1, SECOND = 2};
enum log_partitions current_log_partition = FIRST;


unsigned int log_initialized = 0;
//const unsigned int phase_two = 0;

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

	//TODO: make correct compensation
    // Here we compensate for 11.1ppm = 5*2.17 --> 5 = 0b101
	RTCCTL23 = RTCCAL2 + RTCCAL0;
	RTCCTL23 &= ~RTCCALS;

    RTCCTL01 &= ~(RTCHOLD);                 // Start RTC

	log_initialized = 1;
}

void log_restart()
{
    //reset time stamp:
    // Seconds_set(0); // not anymore!

    // then flush out all the data recorded so far:
    log_send_data_via_uart((uint16_t*)(*FRAM_offset_ptr+LOG_START_POS));
    current_log_partition = FIRST;
    FRAM_read_ptr = (uint16_t*)LOG_START_POS; // points back to start of logged data.

    // new FRAM initialization
    *FRAM_offset_ptr = 0x0000;

    // store correct password
    unsigned int* FRAM_pw = (unsigned int*)LOG_NEXT_POS_VALID;
    (*FRAM_pw) = LOG_POS_VALID_PW;

    (*(uint32_t*)LOG_TIMESTAMP) = Seconds_get();
}

void log_check_pointer_position()
{
    // Make sure we are not going to exceed the reserved memory region. If we do we
    // will not write reset the pointer to the beginning of the memory.
    if (*FRAM_offset_ptr > LOG_END_OFS-LOG_ENTRY_LONG_8b_LEN)
    {
        FRAM_read_end_ptr_value =  *FRAM_offset_ptr;
        // new initialization
        *FRAM_offset_ptr = 0x0000;
    }
    // else, continue...

}

void quick_print(long value, char log_symbol)
{
    char sign = ' ';
    if(value<0)
    {
        sign = '-';
        value = -value;
    }
    uint8_t strlen;
    uint8_t weight_buf[20];
    if(log_symbol == 'D')
        strlen = ui2a(value<<8, 10, 1, HIDE_LEADING_ZEROS, &weight_buf[1]);
    else
        strlen = ui2a(value, 10, 1, HIDE_LEADING_ZEROS, &weight_buf[1]);
    weight_buf[0] = sign;

    Semaphore_pend((Semaphore_Handle)semSerial,BIOS_WAIT_FOREVER);
    uart_serial_print_event(log_symbol, weight_buf, strlen+1);
    Semaphore_post((Semaphore_Handle)semSerial);
}

int log_write_new_entry(uint8_t logchar, uint16_t value)
{
    log_check_pointer_position();
	unsigned int* FRAM_write_ptr = (unsigned int*)(LOG_START_POS + *FRAM_offset_ptr); // = base address plus *FRAM_offset_ptr

#if(LOG_VERBOSE)
    quick_print(value, logchar);
#endif

	*((uint32_t*)FRAM_write_ptr+LOG_TIME_32b_OFS) = Seconds_get();

	*((unsigned char*)FRAM_write_ptr+LOG_CHAR_8b_OFS) = logchar;

	//TODO: calculate some sort of CRC. For now, just repeat the Logchar value
    *((unsigned char*)FRAM_write_ptr+LOG_CRC_8b_OFS) = logchar;

    *((uint16_t*)FRAM_write_ptr+LOG_VALUE_SHORT_16b_OFS) = value;



	*FRAM_offset_ptr += LOG_ENTRY_SHORT_8b_LEN;                 // Increment write index //TODO: ORIGINALLY WE USED THE 8BIT value??

	return LOG_ENTRY_SHORT_8b_LEN;
}

int log_write_new_rfid_entry(uint64_t uid)
{
    uint32_t t = Timestamp_get32();
    uint32_t msecs = (t & 0x7fff) * 1000 /32768;

    uint16_t msec = msecs;
    uint32_t timestamp = Seconds_get();

    log_check_pointer_position();
    unsigned int* FRAM_write_ptr = (unsigned int*)(LOG_START_POS + *FRAM_offset_ptr); // = base address plus *FRAM_offset_ptr

#if(LOG_VERBOSE)
    quick_print(uid, 'R');
#endif

    *((uint32_t*)FRAM_write_ptr+LOG_TIME_32b_OFS) = timestamp;

    *((uint32_t*)FRAM_write_ptr+LOG_UID_32b_LSB_OFS) = 0xffffffff & uid;
    *((uint32_t*)FRAM_write_ptr+LOG_UID_32b_MSB_OFS) = (uid>>16); //only shift it by 2 bytes, because the 32bit value gets written LSByte first and would get overwritten by logchar!

    // overwrite unused bits from 64bit UID field with log-char 'R'
    *((unsigned char*)FRAM_write_ptr+LOG_CHAR_8b_OFS) = 'R';

//    *((unsigned char*)FRAM_write_ptr+LOG_MSEC_8b_OFS) = 0xff & (msec >> 2); // MILLISECONDS ARE GETTING SHIFTED TO ONLY DISPLAY BITS 10 - 2

    *FRAM_offset_ptr += LOG_ENTRY_LONG_8b_LEN;                 // Increment write index //TODO: WHY 8BIT value??

    return LOG_ENTRY_LONG_8b_LEN;
}

int log_write_new_weight_entry( uint8_t logchar, uint32_t weight, uint16_t stdev)
{
    uint32_t t = Timestamp_get32();
    uint32_t msecs = (t & 0x7fff) * 1000 /32768;

    uint32_t timestamp = Seconds_get();
    uint16_t msec = msecs;

    log_check_pointer_position();
    unsigned int* FRAM_write_ptr = (unsigned int*)(LOG_START_POS + *FRAM_offset_ptr); // = base address plus *FRAM_offset_ptr

#if(LOG_VERBOSE)
    quick_print(weight, logchar);
#endif

    *((uint32_t*)FRAM_write_ptr+LOG_TIME_32b_OFS) = timestamp;

    *((unsigned char*)FRAM_write_ptr+LOG_CHAR_8b_OFS) = logchar;
//    *((unsigned char*)FRAM_write_ptr+LOG_MSEC_8b_OFS) = 0xff & (msec >> 2); // MILLISECONDS ARE GETTING SHIFTED TO ONLY DISPLAY BITS 10 - 2

    *((uint32_t*)FRAM_write_ptr+LOG_VALUE_LONG_32b_OFS) = weight;
    *((uint16_t*)FRAM_write_ptr+LOG_STDDEV_16b_OFS) = stdev;

    *FRAM_offset_ptr += LOG_ENTRY_LONG_8b_LEN;                 // Increment write index //TODO: WHY 8BIT value??

    return LOG_ENTRY_LONG_8b_LEN;
}

const uint8_t start_string[] = "#=========== start FRAM logs =========\n";
const uint8_t title_row[] = "time [s],RFID UID,event type, weight [g], tolerance [mg], temperature [1/10 K]\n";
const uint8_t end_string[] = "#========== end FRAM logs ===========\n";

void log_send_data_via_uart(uint16_t* FRAM_read_end_ptr)
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
    char sd_rx;
    unsigned int sd_retry = 0;

    for(sd_retry = 0; sd_retry <= MAX_SD_RETRY; sd_retry++){

    #if LOG_VERBOSE
        GPIO_write(nbox_spi_cs_n, 0); //turn on SD card

        uart_stop_debug_prints();
        uart_serial_write(&debug_uart, start_string, sizeof(start_string));
        //  uart_serial_write(&debug_uart, title_row, sizeof(title_row));
    #else
        uart_debug_open();
        GPIO_write(nbox_spi_cs_n, 0); //turn on SD card

    #endif

        unsigned int sd_delay = 0;
        for(sd_delay = 0; sd_delay < 10; sd_delay++)
        {
            sd_rx=uart_serial_getc(&debug_uart);
            if(sd_rx == '1')
                break;
            Task_sleep(100);
        }
        for(sd_delay = 0; sd_delay < 20; sd_delay++)
        {
            sd_rx=uart_serial_getc(&debug_uart);
            if(sd_rx == '2')
                break;
            Task_sleep(100);
        }
        for(sd_delay = 0; sd_delay < 20; sd_delay++)
        {
            sd_rx=uart_serial_getc(&debug_uart);
            if(sd_rx == '<')
                break;
            Task_sleep(100);
        }

        if(sd_rx == '<' || sd_retry == MAX_SD_RETRY)
            break;
        else
        {
        #if LOG_VERBOSE
            uart_serial_write(&debug_uart, end_string, sizeof(end_string));
            Task_sleep(100);
            uart_start_debug_prints();
        #else
            uart_debug_close();
        #endif

            GPIO_write(nbox_spi_cs_n, 1); //turn off SD card
            Task_sleep(5000);
        }
    }


	uint8_t outbuffer[OUTPUT_BUF_LEN];

	//print load cell offset as header of each file:
	outbuffer[0] = 'H';
	outbuffer[1] = ',';
	int strlen = ui2a(*((uint32_t*)FRAM_read_ptr+LOG_TIME_32b_OFS), 10, 1, HIDE_LEADING_ZEROS, &(outbuffer[2]));
	outbuffer[strlen+2] = ',';
    uart_serial_write(&debug_uart, outbuffer, strlen+3);

    strlen = ui2a(get_weight_offset(), 10, 1, HIDE_LEADING_ZEROS, outbuffer);
    outbuffer[strlen] = '\n';
    uart_serial_write(&debug_uart, outbuffer, strlen+1);


	while(FRAM_read_ptr < FRAM_read_end_ptr)
	{
		//send out log character and time stamp:
        outbuffer[0] = *((uint8_t*)FRAM_read_ptr+LOG_CHAR_8b_OFS);
        outbuffer[1] = ',';
		int strlen = ui2a(*((uint32_t*)FRAM_read_ptr+LOG_TIME_32b_OFS), 10, 1, HIDE_LEADING_ZEROS, &(outbuffer[2]));
        outbuffer[strlen+2] = ',';
		uart_serial_write(&debug_uart, outbuffer, strlen+3);

        unsigned char logchar = outbuffer[0];

		if(logchar == 'X' || logchar == 'O' || logchar == 'S' || logchar == 'A' || logchar == 'R')
		{
		    //send out milliseconds:
//		    strlen = ui2a((*((uint8_t*)FRAM_read_ptr+LOG_MSEC_8b_OFS)<<2), 10, 1,HIDE_LEADING_ZEROS, outbuffer);
//		    outbuffer[strlen] = ',';
//		    uart_serial_write(&debug_uart, outbuffer, strlen+1);
            if(logchar == 'R')
            {
                //send out UID and I/O:
                strlen = ui2a(((*((uint32_t*)FRAM_read_ptr+LOG_UID_32b_MSB_OFS))>>16) & 0x000000ff, 16, 1,HIDE_LEADING_ZEROS, outbuffer); //the first 32 (actually 8) bits
                uart_serial_write(&debug_uart, outbuffer, strlen);
                strlen = ui2a(*((uint32_t*)FRAM_read_ptr+LOG_UID_32b_LSB_OFS), 16, 1,PRINT_LEADING_ZEROS, outbuffer); //the second 32 bits
                outbuffer[strlen] = '\n';
                uart_serial_write(&debug_uart, outbuffer, strlen+1);
            }
            else
            {
                strlen = ui2a(*((uint32_t*)FRAM_read_ptr+LOG_VALUE_LONG_32b_OFS), 10, 1, HIDE_LEADING_ZEROS, outbuffer);
                outbuffer[strlen] = ',';
                uart_serial_write(&debug_uart, outbuffer, strlen+1);
                strlen = ui2a(*((uint16_t*)FRAM_read_ptr+LOG_STDDEV_16b_OFS), 10, 1, HIDE_LEADING_ZEROS, outbuffer);
                outbuffer[strlen] = '\n';
                uart_serial_write(&debug_uart, outbuffer, strlen+1);
            }

            //increment pointer to next memory location
            FRAM_read_ptr += LOG_ENTRY_LONG_16b_LEN; // TODO_ why not 8bit value???
            continue;
		}


		//print short value:
		if(logchar == 'D')
		    strlen = ui2a(((uint32_t)(*((uint16_t*)FRAM_read_ptr+LOG_VALUE_SHORT_16b_OFS)))<<8, 10, 1, HIDE_LEADING_ZEROS, outbuffer);
		else
		    strlen = ui2a(*((uint16_t*)FRAM_read_ptr+LOG_VALUE_SHORT_16b_OFS), 10, 1, HIDE_LEADING_ZEROS, outbuffer);

        outbuffer[strlen] = '\n';
        uart_serial_write(&debug_uart, outbuffer, strlen+1);

        //increment pointer to next memory location
        FRAM_read_ptr += LOG_ENTRY_SHORT_16b_LEN;

	}


    Task_sleep(3000); //wait for data to be written

#if LOG_VERBOSE
    uart_serial_write(&debug_uart, end_string, sizeof(end_string));
    Task_sleep(100);
    uart_start_debug_prints();
#else
    uart_debug_close();
#endif

    GPIO_write(nbox_spi_cs_n, 1); //turn off SD card
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
}


extern SPI_Handle  nestbox_spi_handle;
int sd_spi_is_initialized = 0;

//Bool sd_spi_send_command(unsigned char cmd, uint32_t arg, uint8_t crc, uint8_t* response, unsigned int readlen)
//{
//    SPI_Transaction     spiTransaction;
//    Bool                transferOK;
//
//    uint8_t txBuf[6];
//    uint8_t rxBuf[6];
//
//    txBuf[0] = cmd;
//    txBuf[1] = arg>>24;
//    txBuf[2] = arg>>16;
//    txBuf[3] = arg>>8;
//    txBuf[4] = arg;
//    txBuf[5] = crc;
//
//    // send command: "init and go to SPI mode":
//    // Init and go to SPI mode: ]r:10 [0x40 0x00 0x00 0x00 0x00 0x95 r:8]
//
//    spiTransaction.count = 6;
//    spiTransaction.txBuf = txBuf;
//    spiTransaction.rxBuf = rxBuf;
//
//    spi_slave_select(nbox_spi_cs_n);
//    transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
//
//    // receive response:
//    spiTransaction.count = 1;
//    txBuf[0] = 0xff;
//    spiTransaction.txBuf = txBuf;
//    spiTransaction.rxBuf = rxBuf;
//
//    unsigned int i=0;
//    for(i = 0; i<readlen; i++)
//    {
//        SPI_transfer(nestbox_spi_handle, &spiTransaction);
//        response[i] = rxBuf[0];
//    }
//    spi_slave_unselect(nbox_spi_cs_n);
//
//    return transferOK;
//}
//
//#define CMD0    0x40
//#define CMD8    0x48
//#define CMD16   0x50
//#define CMD17   0x51
//#define CMD55   0x77
//#define CMD58   0x7A
//#define ACMD41  0x69 // must be preceded by CMD55 !
//
/* inspired by http://codeandlife.com/2012/04/25/simple-fat-and-sd-tutorial-part-3/ */
//int sd_spi_init_logger()
//{
//    SPI_Transaction     spiTransaction;
//    Bool                transferOK;
//    uint8_t     txBuf[8]= {0xff,};
//    uint8_t     rxBuf[8] = {0xff,};
//
//    unsigned int i = 0;
//    // send command: "init and go to SPI mode":
//    // Init and go to SPI mode: ]r:10 [0x40 0x00 0x00 0x00 0x00 0x95 r:8]
//
//    spiTransaction.count = 1;
//    spiTransaction.txBuf = txBuf;
//    spiTransaction.rxBuf = rxBuf;
//
//    spi_slave_unselect(nbox_spi_cs_n);
//    for(i=0; i<8; i++)
//        SPI_transfer(nestbox_spi_handle, &spiTransaction);
//
//    for(i=0; i<10; i++)
//    {
//        sd_spi_send_command(CMD0, 0, 0x95, rxBuf, 4);
//
//        if(rxBuf[1] == 1)
//            break;
//        Task_sleep(100);
//    }
//
//    if(i==10)
//        return -1;
//
//    // CMD8 to read version of SD card
//    // according to https://openlabpro.com/guide/interfacing-microcontrollers-with-sd-card/
//    sd_spi_send_command(CMD8, 0x000001AA, 0x87, rxBuf, 8);
//
//    for(i=0; i<10; i++)
//    {
//        // send ACMD41 repeatedly until initialized
//        sd_spi_send_command(CMD55, 0, 0x01, rxBuf, 4);
//        sd_spi_send_command(ACMD41, 0x40000000, 0x01, rxBuf, 4);
//        if(rxBuf[0] == 0 || rxBuf[1] == 0 || rxBuf[2] == 0 || rxBuf[3] == 0)
//            break;
//        Task_sleep(20);
//    }
//
//    if(i == 10)
//        return -2;
//
//    // Read OCR --> answer: first byte bit6 reads 1 --> high capacity SD card (SDHC)
//    //                                              --> block size is 512 by default!
//    sd_spi_send_command(CMD58, 0, 0xff, rxBuf, 8);
//
//    //Set transfer size: [0x50 0x00 0x00 0x02 0x00 0xFF r:8]
//    sd_spi_send_command(CMD16, 0x00000200, 0xff, rxBuf, 8);
//    Task_sleep(100);
//
//    memcpy(txBuf, (const unsigned char[]){0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, 8);
//
//    Semaphore_pend((Semaphore_Handle)semSerial,BIOS_WAIT_FOREVER);
//
//    unsigned int j=0;
//    for (j=0; j<100; j++) //read 100 sectors
//    {
//        for(i=0; i<10; i++)
//        {
//            //Read sector: [0x51 0x00 0x00 0x00 0x00 0xFF r:520]
//            sd_spi_send_command(CMD17, j+(16<<9), 0xff, rxBuf, 4); // --->> the first time it worked was at j=16 - text at j=33
//            // output:
//            // BSD  4.4 @      ⸮    ⸮      ⸮⸮ f                       ⸮ )⸮NO NAME    FAT32   ⸮1⸮⸮м |⸮⸮⸮⸮  ^⸮⸮⸮ ⸮⸮⸮⸮t⸮⸮⸮⸮0⸮⸮⸮
//            //Non-system disk
//            //Press any key to reboot
//
//
//            //wait for a 0x00 to arrive
//
//            if(rxBuf[0] == 0 ||rxBuf[1] == 0 || rxBuf[2] == 0 || rxBuf[3] == 0) //the 0x00 response usually arrives on rxBuf[1] or rxBuf[2]
//                break;
//            Task_sleep(100);
//
//        }
//        spiTransaction.txBuf = txBuf;
//        spiTransaction.rxBuf = rxBuf;
//        spiTransaction.count = 1; //read 1 Byte at a time until 0 is detected
//        spi_slave_select(nbox_spi_cs_n);
//        if(rxBuf[2] != 0xFE && rxBuf[3] != 0xFE)
//        {
//            for(i=0; i<10; i++)
//            {
//                transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
//                if(rxBuf[0] == 0xFE)
//                    break;
//            }
//        }
//
//        for(i=0; i<65; i++) // read 512 bytes plus 2bytes CRC plus 6 dummy bytes (0xff)
//        {
//            spiTransaction.count = 8; //read 8 Byte at a time
//            spiTransaction.txBuf = txBuf;
//            spiTransaction.rxBuf = rxBuf;
//            transferOK = SPI_transfer(nestbox_spi_handle, &spiTransaction);
//
//            uart_serial_write(&debug_uart, rxBuf,8);
//        }
//        spi_slave_unselect(nbox_spi_cs_n);
//
//        uart_serial_write(&debug_uart, "\n", 1); //newline after each sector
//
//
//        Task_sleep(100);
//    }
//    Semaphore_post((Semaphore_Handle)semSerial);
//
//    if (!transferOK) {
//        return 0;/* Error in SPI transfer or transfer is already in progress */
//    }
//    return 1;
//}

void log_Task()
{
    log_startup();

    //TODO: check first if there is some logged stuff on the FRAM to avoid data loss after a crash.
    FRAM_read_ptr = (unsigned int*)LOG_START_POS; // points to start of logged data.

	Task_sleep(500); //wait until other functions are initialized

#if(LOG_VERBOSE)
    uart_debug_open();
	uart_start_debug_prints(); // disable all UART comm except when SD card is on
#else
    uart_debug_close();
#endif



    //sd_spi_init_logger();
    //fat_disk_initialize (0);

//    FATFS FatFs;        /* FatFs work area needed for each volume */
//    FIL Fil;            /* File object needed for each open file */
//    UINT bw;
//    FRESULT res;
//
//    f_mount(&FatFs, "", 0);     /* Give a work area to the default drive */
    // CAREFUL: filename length limitation!!
//    if (f_open(&Fil, "mun.txt", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {    /* Create a file */
//       uart_serial_print_event('F', &bw, 1);
//
//       res =  f_write(&Fil, "It works!\r\n", 11, &bw);    /* Write data to the file */
//       if(res == FR_OK)
//       {
//           GPIO_write(Board_led_green,0);
//
//       }
//        res = f_close(&Fil);                              /* Close the file */
//
//        if (bw == 11) {     /* Lights green LED if data written well */
//            GPIO_write(Board_led_green,1);
//        }
//        if(res == FR_OK)
//        {
//            GPIO_write(Board_led_green,0);
//
//        }
//    }


	while(1)
	{

	    if(current_log_partition == FIRST && *FRAM_offset_ptr > LOG_MIDDLE_OFS)
	    {
	        //Flush out the first half of the internal log via UART
	        log_send_data_via_uart((uint16_t*)LOG_MIDDLE_POS);
	        current_log_partition = SECOND;
	    }

	    if(current_log_partition == SECOND && *FRAM_offset_ptr < LOG_MIDDLE_OFS)
        {
            //Flush out the first half of the internal log via UART
            log_send_data_via_uart((uint16_t*)(FRAM_read_end_ptr_value+LOG_START_POS));
            current_log_partition = FIRST;
            FRAM_read_ptr = (uint16_t*)LOG_START_POS; // points back to start of logged data.
        }

	    Task_sleep(10000);

//		if(phase_two == 2)
//		{
//			uart_serial_write(&debug_uart, phase_two_string, sizeof(phase_two_string));
//			phase_two = 1;
//		}

//		Semaphore_pend((Semaphore_Handle)semSerial,BIOS_WAIT_FOREVER);
	}
}


