/*
 * rfid_reader.c
 *
 *  Created on: 27 Mar 2017
 *      Author: raffael
 */

#include "em4095_lib/EM4095.h"
#include "rfid_reader.h"
#include "MLX90109_library/mlx90109.h"
#include "MLX90109_library/mlx90109_params.h"
#include "logger.h"
#include <msp430.h>


#include <time.h>
#include <ti/sysbios/hal/Seconds.h>
#include <xdc/runtime/Timestamp.h>


#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

/*************** LF: **********************/
static mlx90109_t mlx_dev;
static tagdata lf_tagdata;


void rfid_Task()
{
	/* Initialize LF reader */
	mlx90109_params_t mlx_params = MLX90109_PARAMS;
	mlx90109_init(&mlx_dev, &mlx_params);

    while (1) {
		Semaphore_pend((Semaphore_Handle)semReader, BIOS_WAIT_FOREVER);
		// tag has been successfully read out.

		if(mlx_dev.p.tag_select ==  MLX_TAG_FDX)
		{
			//for FDX-B only: check CRC:
			if(fdx_format(&mlx_dev, &lf_tagdata) == MLX90109_OK)
			{
				lf_tagdata.valid = 1;
				rfid_stop_detection();
			}
		}
		else
		{
			if(em4100_format(&mlx_dev, &lf_tagdata) == MLX90109_OK)
			{
				lf_tagdata.valid = 1;
				rfid_stop_detection();
			}
		}

		if(lf_tagdata.valid)
		{
		    log_write_new_rfid_entry(lf_tagdata.tagId);
//			Semaphore_pend((Semaphore_Handle)semSerial,BIOS_WAIT_FOREVER);
//			uint8_t outbuffer[20]; // (64bits/4bits per character) = 16; conservative buffer size value!
//			uint8_t strlen = ui2a((lf_tagdata.tagId)>>32, 16, 1,HIDE_LEADING_ZEROS, outbuffer); //the first 32 bits
//			strlen = strlen + ui2a((uint32_t)(0xffffffff & (lf_tagdata.tagId)), 16, 1,PRINT_LEADING_ZEROS, &(outbuffer[strlen])); //the second 32 bits
//			uart_serial_print_event('R', outbuffer, strlen);
//			Semaphore_post((Semaphore_Handle)semSerial);
		}
    }
}

int rfid_get_id(uint64_t* id)
{
	if(lf_tagdata.valid)
	{
		*id = lf_tagdata.tagId;
//		if(lf_tagdata.tagId == CALIB162_UID)
//		    return 162;
//        if(lf_tagdata.tagId == CALIB595_UID)
//            return 595;
//        if(lf_tagdata.tagId == CALIB379_UID)
//            return 379;
		//else
		return 1;
	}
	else
		return 0;
}

void rfid_start_detection()
{
	lf_tagdata.valid = 0;
	GPIO_write(nbox_5v_enable,1);
	mlx90109_activate_reader(&mlx_dev);
	em4095_startRfidCapture();
}

void rfid_stop_detection()
{
	em4095_stopRfidCapture();
	mlx90109_disable_reader(&mlx_dev, &lf_tagdata);
	GPIO_write(nbox_5v_enable,0);
}

volatile uint8_t last_bit = 0;
volatile uint16_t last_timer_val = 0;
static uint16_t timediff = 0;

//one bit period = 500us = 62.5 cycles = shortest interval
//mid interval = 750us = 94 cycles
//longest interval = 1000us = 125 cycles

const uint16_t threshold_short = 78;
const uint16_t threshold_long = 109;

void lf_tag_read_isr()
{
#ifdef MLX_READER
//	int cnt = mlx_dev.counter;
//	mlx_dev.int_time[cnt] = TA3R;
	if(mlx90109_read(&mlx_dev)==MLX90109_DATA_OK)
	{
		Semaphore_post((Semaphore_Handle)semReader);
	}

//	if(mlx_dev.counter_header<11)
//		{
//			if (!(GPIO_read(mlx_dev.p.data)))
//			{// 0's
//			mlx_dev.tagId[mlx_dev.counter_header] = 0;
//				mlx_dev.counter_header++;
//
//			}
//			else if(mlx_dev.counter_header == 10)
//			{//the final 1 of the header
//				mlx_dev.counter_header++;
//			}
//			else
//			{//a '1' that is too early
//				mlx_dev.counter_header=0;
//			}
//			mlx_dev.counter = 0;
//			//mlx_dev.last_timestamp = Timestamp_get32();
//		}
//		else //if(mlx_dev.counter_header==11)
//		{
//			mlx_dev.data[mlx_dev.counter] = GPIO_read(mlx_dev.p.data);
//
//			// Detect "1"
//	//		if(GPIO_read(mlx_dev.p.data) > 0)
//	//		{
//	//			mlx_dev.data[mlx_dev.counter]=1;
//	//		}
//	//		else
//	//		{
//	//			mlx_dev.data[mlx_dev.counter]=0;
//	//		}
//			mlx_dev.timediff[mlx_dev.counter] = TA3R-mlx_dev.last_timestamp;
//			mlx_dev.last_timestamp = TA3R;
//			mlx_dev.counter++;
//		}
//
//		// Data complete after 128-11 bit
//		if ( mlx_dev.counter > (114))
//		{
//			mlx_dev.counter = 0;
//			mlx_dev.counter_header = 0;
//			Semaphore_post((Semaphore_Handle)semReader);
//		}
//
//		mlx_dev.int_time[cnt] = (TA3R-mlx_dev.int_time[cnt]);
#else
	// for EM reader, this is the data pin with CCR!!!
	timediff = TB0CCR2 - last_timer_val;
	last_timer_val = TB0CCR2;

	if(timediff > 2000)
	{
		timediff = 0xFFFF - timediff;
	}
	else if(timediff > threshold_long)
	{//01
		//this is for sure a one preceded by a zero
		last_bit = 1;
		if(em4095_read(&mlx_dev, 0)==MLX90109_DATA_OK)
			Semaphore_post((Semaphore_Handle)semReader);

		if(em4095_read(&mlx_dev, 1)==MLX90109_DATA_OK)
			Semaphore_post((Semaphore_Handle)semReader);
	}
	else if(timediff < threshold_short)
	{
		if(last_bit == 1)
		{//1
			if(em4095_read(&mlx_dev, 1)==MLX90109_DATA_OK)
				Semaphore_post((Semaphore_Handle)semReader);
		}
		else
		{//0
			if(em4095_read(&mlx_dev, 0)==MLX90109_DATA_OK)
				Semaphore_post((Semaphore_Handle)semReader);
		}
	}
	else
	{
		if(last_bit == 1)
		{//00
			if(em4095_read(&mlx_dev, 0)==MLX90109_DATA_OK)
				Semaphore_post((Semaphore_Handle)semReader);
			if(em4095_read(&mlx_dev, 0)==MLX90109_DATA_OK)
				Semaphore_post((Semaphore_Handle)semReader);
			last_bit = 0;
		}
		else
		{//(0)1
			if(em4095_read(&mlx_dev, 1)==MLX90109_DATA_OK)
				Semaphore_post((Semaphore_Handle)semReader);
			last_bit = 1;
		}

	}
#endif
}


