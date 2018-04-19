/*
 * load_cell.c
 *
 *  Created on: 03 Mar 2018
 *      Author: raffael
 */

#include "load_cell.h"

#ifdef USE_HX
#include "HX711/HX711.h"
#endif

#ifdef USE_ADS
#include "ADS1220/ads1220.h"

#endif

#include "../Board.h"
#include "uart_helper.h"

#include "rfid_reader.h"
#include "logger.h"

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>


#define WEIGHT_THRESHOLD 100 // grams
#ifdef USE_HX
	#define SAMPLE_RATE		HX_SAMPLE_RATE //Hz
#endif
#ifdef USE_ADS
	#define SAMPLE_RATE		ADS_SLOW_SAMPLE_RATE //Hz
#endif

#define MIN_EVENT_TIME 	10 //seconds
#define N_AVERAGES		10
#define EVENT_BUF_SIZE	SAMPLE_RATE*MIN_EVENT_TIME/N_AVERAGES //need to account for 10 averaging window already in place!

#define PLUS_SIGN 		' '
#define MINUS_SIGN		'-'

#define RFID_TIMEOUT		3000 	//ms
#define T_RFID_RETRY		10000 	//ms
#define T_LOADCELL_POLL	1000 	//ms

#define WEIGHT_TOLERANCE 	0.1f		// maximum deviation from average value within one measurement series
#define WEIGHT_MAX_CHANGE	0.015f	// maximum change

void print_load_cell_value(float value, char sign, char log_symbol)
{
	uint8_t strlen;
	uint8_t weight_buf[20];
	strlen = ui2a((unsigned long)(value), 10, 1, HIDE_LEADING_ZEROS, &weight_buf[1]);
	weight_buf[0] = sign;

	Semaphore_pend((Semaphore_Handle)semSerial,BIOS_WAIT_FOREVER);
	uart_serial_print_event(log_symbol, weight_buf, strlen+1);
	Semaphore_post((Semaphore_Handle)semSerial);
}

int load_cell_get_stable(struct Ads1220 *ads)
{
	static float meas_buf[EVENT_BUF_SIZE] = {0.0,};
	static int first_valid = 0;
	static int first_invalid = 0;

	int i = 0;
	int tmp = first_invalid;
	float deviation = 0;

	// fill circular buffer with new measurements
	do{
#ifdef USE_HX
		meas_buf[tmp] = hx711_get_units(N_AVERAGES, &deviation);
#endif
#ifdef USE_ADS
		meas_buf[tmp] = ads1220_get_units(N_AVERAGES, &deviation, ads);
#endif
		if(meas_buf[tmp]<WEIGHT_THRESHOLD)
		{
			first_valid = 0;
			first_invalid = 0;
			return 0;
		}
		if(deviation > WEIGHT_TOLERANCE)
			continue;

		print_load_cell_value(meas_buf[tmp], PLUS_SIGN, 'W');

		tmp = tmp + 1;
		if(tmp >= EVENT_BUF_SIZE)
			tmp = 0;

	}while(tmp != first_valid);

	// calculate average over circular buffer:
	float average = meas_buf[0];
	for(i=1; i<EVENT_BUF_SIZE; i++)
	{
		average = average + meas_buf[i];
	}
	average = average/EVENT_BUF_SIZE;

//	// check backwards if all values are within derivative limit
//	if(first_valid > 0)
//		tmp = first_valid - 1;
//	else
//		tmp = EVENT_BUF_SIZE-1;
//
//	for(i=0; i<EVENT_BUF_SIZE; i++)
//	{
//		average =
//	}

	print_load_cell_value(average, PLUS_SIGN, 'A');
	return 1;
}

void ads1220_set_loadcell_config(struct Ads1220 *ads){
	ads->config.mux = ADS1220_MUX_AIN1_AIN2;
	ads->config.gain = ADS1220_GAIN_128;
	ads->config.pga_bypass = 0;
	ads->config.rate = ADS1220_RATE_1000_HZ;
	// todo: change operating mode to duty-cycle mode
	ads->config.conv = ADS1220_CONTINIOUS_CONVERSION;
	ads->config.vref = ADS1220_VREF_EXTERNAL_AIN;
	ads->config.idac = ADS1220_IDAC_OFF;
	ads->config.i1mux = ADS1220_IMUX_OFF;
	ads->config.i2mux = ADS1220_IMUX_OFF;
	ads->config.low_switch = 1;
	ads->config.filter = ADS1220_FILTER_NONE; //At data rates of 5 SPS and 20 SPS, the filter can be configured to reject 50-Hz or 60-Hz line frequencies or to simultaneously reject 50 Hz and 60 Hz!!!
}

void load_cell_Task()
{
	Task_sleep(1000); //wait until things are settled...

	//weight value
	float value;

	//storage for measurement series
	char event_ongoing = 0;
	char series_completed = 0;

	uint64_t owl_ID = 0;


#ifdef USE_HX

	hx711_begin(nbox_loadcell_data, nbox_loadcell_clk, 128);

	hx711_power_up();

	Task_sleep(1000); //wait until things are settled...
	//todo: check if tare is successful!!
	hx711_tare(20);

	hx711_power_up();
#endif

#ifdef USE_ADS
	spi1_init();
	struct spi_periph ads_spi;
	struct Ads1220 ads;

	ads1220_init(&ads, &ads_spi, nbox_loadcell_spi_cs_n);
	ads1220_set_loadcell_config(&ads);
	ads.config.rate = ADS1220_RATE_20_HZ; //for tare, set to slow=exact mode

	Task_sleep(10);
	ads1220_event(&ads);

	ads1220_configure(&ads);
	Task_sleep(10);
	ads1220_event(&ads);

	ads1220_tare(20, &ads);


	//ads.config.rate = ADS1220_RATE_1000_HZ; //for presence detection, set to fast=inexact mode
	//ads1220_configure(&ads);
	ads1220_event(&ads);

	ads1220_periodic(&ads);
#endif

	while(1)
	{
		// currently no event detected & reader was off
		if(!event_ongoing || series_completed)
		{
#ifdef USE_ADS
		/************ADS1220 POLLING*************/
		ads1220_start_conversion(&ads);
		ads1220_event(&ads);

		Task_sleep(60); //TODO: put semaphore and wait for READY signal!!
		ads1220_periodic(&ads);
		ads1220_event(&ads);
		ads1220_powerdown(&ads);
		print_load_cell_value((float)(ads.data), PLUS_SIGN, 'G');
		/**********END ADS1220 TEST***********/
#endif

#ifdef USE_HX
			// hx711_power_up();

			// check if bird is present:
			float dummy_tol;
			value = hx711_get_units(1,&dummy_tol);

			// hx711_power_down();
#endif

			char sign = PLUS_SIGN;
			if(value<0)
			{
				value = -value;
				sign = MINUS_SIGN;
			}

			//print the inexact weight value: (TODO:remove)
			print_load_cell_value(value, sign, 'D');

			if(value>WEIGHT_THRESHOLD && sign == PLUS_SIGN)
			{
				if(event_ongoing==0)
				{
					rfid_start_detection();
					Semaphore_pend((Semaphore_Handle)semLoadCell,RFID_TIMEOUT);

					if(!log_phase_two())
						rfid_stop_detection();

					if(rfid_get_id(&owl_ID))
					{
						// now start the weight measurement
#ifdef USE_ADS/*
						// change to slow = exact mode
						ads.config.rate = ADS1220_RATE_20_HZ;
						ads1220_configure(&ads);
						ads1220_event(&ads);*/
#endif

						event_ongoing = 1;
						series_completed = 0;
					}
					else
						Task_sleep(T_RFID_RETRY);
				}
			}
			else
			{
				event_ongoing = 0;
			}

			// polling delay...
			Task_sleep(T_LOADCELL_POLL);
		}

		if(event_ongoing)
		{
			// -->ID WAS DETECTED!

			//measure weight again with 10 averages:
//**************************************************************if(load_cell_get_stable(&ads)) //TODO
			if(0)
			{
				//log event!! + mark series completed, but keep event ongoing (in order to not count it twice)!
				series_completed = 1;

				// now start the weight measurement
#ifdef USE_ADS/*
				// change to slow = exact mode
				ads.config.rate = ADS1220_RATE_1000_HZ; //for presence detection, set to fast=inexact mode
				ads1220_configure(&ads);
				ads1220_event(&ads);*/
#endif
			}
			else
			{
				Task_sleep(T_LOADCELL_POLL);
			}
		}
	}

}


