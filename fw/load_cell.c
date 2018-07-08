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

#include <ti/sysbios/hal/Seconds.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

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

#define RFID_TIMEOUT		200 	//ms
#define T_RFID_RETRY		1000 	//ms
#define T_LOADCELL_POLL	1000 	//ms

#define SAMPLE_TOLERANCE 	2.0f		// maximum variation of the sampled values within N_AVERAGES samples
#define WEIGHT_TOLERANCE 	1.0f		// maximum deviation from average value within one measurement series
#define WEIGHT_MAX_CHANGE	0.015f	// maximum change within one "event"

#define RAW_THRESHOLD       220000
// TODO: above values should be in %FS

Semaphore_Handle semLoadCellDRDY;

float WEIGHT_SLOPE = 1.0;
float WEIGHT_Y0 = 0;

typedef enum weightResultStatus_
{
	OWL_LEFT = 0,
	UNSTABLE,
	STABLE
} weightResultStatus;

void print_load_cell_value(float value, char log_symbol)
{
	char sign = PLUS_SIGN;
	if(value<0)
	{
		sign = MINUS_SIGN;
		value = -value;
	}
	uint8_t strlen;
	uint8_t weight_buf[20];
	strlen = ui2a((unsigned long)(value), 10, 1, HIDE_LEADING_ZEROS, &weight_buf[1]);
	weight_buf[0] = sign;

	Semaphore_pend((Semaphore_Handle)semSerial,BIOS_WAIT_FOREVER);
	uart_serial_print_event(log_symbol, weight_buf, strlen+1);
	Semaphore_post((Semaphore_Handle)semSerial);
}

weightResultStatus load_cell_get_stable(struct Ads1220 *ads)
{
	static float meas_buf[EVENT_BUF_SIZE] = {0.0,};
	static int first_valid = 0;
//	static int first_invalid = 0;

	int i = 0;
	int tmp = first_valid;
	int values_recorded = 0;
	float deviation = 0;

	// fill circular buffer with new measurements
	while(values_recorded < EVENT_BUF_SIZE)
	{
#ifdef USE_HX
		meas_buf[tmp] = hx711_get_units(N_AVERAGES, &deviation);
#endif
#ifdef USE_ADS
		meas_buf[tmp] = ads1220_get_units(N_AVERAGES, &deviation, ads);
#endif
		print_load_cell_value(meas_buf[tmp]*1000, 'X');

		if(meas_buf[tmp]<(float)WEIGHT_THRESHOLD)
		{
			first_valid = 0;
//			first_invalid = 0;
			return OWL_LEFT;
		}
		if(deviation > SAMPLE_TOLERANCE)
			continue;

		tmp = tmp + 1;
		if(tmp >= EVENT_BUF_SIZE)
			tmp = 0;

		values_recorded = values_recorded+1;
	}

	// calculate average over circular buffer:
	float average = meas_buf[0];
	float min = average;
	float max = average;

	for(i=1; i<EVENT_BUF_SIZE; i++)
	{
		average = average + meas_buf[i];
		if(meas_buf[i]>max)
			max = meas_buf[i];
		if(meas_buf[i]<min)
			min = meas_buf[i];
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

	float tol = (max - min);
	if(tol < ads->tolerance)
	{
		ads->stable_weight = average;
		ads->tolerance = tol;
        GPIO_write(Board_led_blue,1);
        Task_sleep(200);
        GPIO_write(Board_led_blue,0);
	}

	if(tol < WEIGHT_TOLERANCE)
	{
		print_load_cell_value(average, 'S');
		GPIO_write(Board_led_blue,1);
        Task_sleep(2000);
        GPIO_write(Board_led_blue,0);
		return STABLE;
	}

	else
	{
		print_load_cell_value(average, 'A');
		return UNSTABLE;
	}
}

void ads1220_set_loadcell_config(struct Ads1220 *ads){
	ads->config.mux = ADS1220_MUX_AIN1_AIN2;
	ads->config.gain = ADS1220_GAIN_128;
	ads->config.pga_bypass = 0;
	ads->config.rate = ADS1220_RATE_20_HZ;
	// todo: change operating mode to duty-cycle mode
	ads->config.conv = ADS1220_SINGLE_SHOT;
	ads->config.temp_sensor = ADS1220_TEMPERATURE_DISABLED;
	ads->config.vref = ADS1220_VREF_EXTERNAL_AIN; // this will be toggled with AC excitation
	ads->config.idac = ADS1220_IDAC_OFF;
	ads->config.i1mux = ADS1220_IMUX_OFF;
	ads->config.i2mux = ADS1220_IMUX_OFF;
	ads->config.low_switch = 1; // Switch is closed after START/SYNC command
	ads->config.filter = ADS1220_FILTER_NONE; //At data rates of 5 SPS and 20 SPS, the filter can be configured to reject 50-Hz or 60-Hz line frequencies or to simultaneously reject 50 Hz and 60 Hz!!!
}

void load_cell_Task()
{
	Task_sleep(1000); //wait until things are settled...

	//weight value
//	float value = 0;

	int32_t raw_threshold = 0;

	//storage for measurement series
	char event_ongoing = 0;
	char series_completed = 0;

	uint64_t owl_ID = 0;
	int rfid_type = 0;
	float calib163 = 0;
	float calib379 = 0;
	float calib595 = 0;



#ifdef USE_HX

	hx711_begin(nbox_loadcell_data, nbox_loadcell_clk, 128);

	hx711_power_up();

	Task_sleep(1000); //wait until things are settled...
	//todo: check if tare is successful!!
	hx711_tare(20);

	hx711_power_up();
#endif

#ifdef USE_ADS
	/* Initialize the Data READY semaphore */
	Semaphore_Params semParams;
	Error_Block eb;
	Error_init(&eb);
	Semaphore_Params_init(&semParams);
	semParams.mode = Semaphore_Mode_BINARY;
	semLoadCellDRDY = Semaphore_create(0, &semParams, &eb);

	GPIO_enableInt(nbox_loadcell_data_ready);

	// turn on analog supply:
	//GPIO_write(nbox_loadcell_ldo_enable, 1);
//	GPIO_write(nbox_loadcell_exc_a_p, 0); //pmos, turn on
//	GPIO_write(nbox_loadcell_exc_b_n, 1); //nmos, turn on
	//TODO: check if delay is needed!

	spi1_init();
	struct spi_periph ads_spi;
	struct Ads1220 ads;


	ads1220_init(&ads, &ads_spi, nbox_loadcell_spi_cs_n);
	ads1220_set_loadcell_config(&ads);
	//ads.config.rate = ADS1220_RATE_20_HZ; //for tare, set to slow=exact mode

	Task_sleep(10);
	ads1220_event(&ads);

	ads1220_configure(&ads); // this one sends the reset and config



	ads1220_tare(20, &ads);
	ads1220_set_raw_threshold(&raw_threshold, (float)WEIGHT_THRESHOLD);


	ads1220_change_mode(&ads, ADS1220_RATE_1000_HZ, ADS1220_SINGLE_SHOT, ADS1220_TEMPERATURE_DISABLED);

#endif

	while(1)
	{
		// currently no event detected & reader was off
		if(!event_ongoing || series_completed)
		{
#ifdef USE_ADS
		/************ADS1220 POLLING*************/
		ads1220_start_conversion(&ads);
		Semaphore_reset((Semaphore_Handle)semLoadCellDRDY, 0);
		Semaphore_pend((Semaphore_Handle)semLoadCellDRDY, 100); // timeout 100 ms in case DRDY pin is not connected

		ads1220_periodic(&ads);
		ads1220_event(&ads);
		ads1220_powerdown(&ads);
		// TODO: remove conversion
		// value = ads1220_convert_units(&ads);
		/**********END ADS1220 TEST***********/
#endif

#ifdef USE_HX
			// hx711_power_up();

			// check if bird is present:
			float dummy_tol;
			value = hx711_get_units(1,&dummy_tol);

			// hx711_power_down();
#endif

			//print the inexact weight value: (TODO:remove)
//			print_load_cell_value(value*1000, 'W');

            if((ads.data)>RAW_THRESHOLD)
//			if((ads.data)>raw_threshold)
			{
				if(event_ongoing==0)
				{
					if(!log_phase_two())
					{
					    print_load_cell_value((float)(ads.data), 'D');

						rfid_start_detection();
						Semaphore_pend((Semaphore_Handle)semLoadCell,RFID_TIMEOUT);
						rfid_stop_detection();
					}
					rfid_type = rfid_get_id(&owl_ID);

					if(rfid_type>0 || log_phase_two())
					{
						// now start the weight measurement

					    //indicate calibration mode if calib UID detected:
					    if(rfid_type > 1)
					        GPIO_write(Board_led_blue,1);

#ifdef USE_ADS
						// change to slow = exact mode
					    GPIO_disableInt(nbox_loadcell_data_ready);

						ads1220_change_mode(&ads, ADS1220_RATE_20_HZ, ADS1220_CONTINIOUS_CONVERSION, ADS1220_TEMPERATURE_DISABLED);
						ads.stable_weight = 0;
						ads.tolerance = SAMPLE_TOLERANCE;
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

		if(event_ongoing==1 && series_completed==0)
		{
			// -->ID WAS DETECTED!

			//measure weight again with 10 averages:

			weightResultStatus res = load_cell_get_stable(&ads);

			// measure temperature
			ads1220_change_mode(&ads, ADS1220_RATE_20_HZ, ADS1220_CONTINIOUS_CONVERSION, ADS1220_TEMPERATURE_ENABLED);
            GPIO_enableInt(nbox_loadcell_data_ready);

			Semaphore_reset((Semaphore_Handle)semLoadCellDRDY, 0);
			Semaphore_pend((Semaphore_Handle)semLoadCellDRDY, 100); // timeout 100 ms in case DRDY pin is not connected

			ads1220_read(&ads);
			ads1220_event(&ads);

			ads1220_convert_temperature(&ads);

			uint16_t temp = (uint16_t)((ads.temperature+273.15) * 10); //deci kelvins

			print_load_cell_value(ads.temperature*100, 'T');
            GPIO_disableInt(nbox_loadcell_data_ready);

			ads1220_change_mode(&ads, ADS1220_RATE_20_HZ, ADS1220_CONTINIOUS_CONVERSION, ADS1220_TEMPERATURE_DISABLED);

			if((res == STABLE && (!log_phase_two())) || res == OWL_LEFT)
			{
				//log event!! + mark series completed, but keep event ongoing (in order to not count it twice)!
				series_completed = 1;
				uint16_t weight = (uint16_t)(ads.stable_weight * WEIGHT_SLOPE + WEIGHT_Y0);
				uint16_t tol = (uint16_t)(ads.tolerance * WEIGHT_SLOPE*1000);


				char log_char = 'X';
				if(res == STABLE)
					log_char = 'W';

				if(rfid_type > 1)
                {
				    owl_ID = 0xCA71B;
				    GPIO_write(Board_led_blue,0);
				    switch(rfid_type)
				    {
				        case 162:
				            calib163 = ads.stable_weight;
				            break;
				        case 379:
				            calib379 = ads.stable_weight;
				            break;
				        case 595:
				            calib595 = ads.stable_weight;
				            break;
				        default:
				            break;
				     }

				    if(calib163>0 && calib595>0 && calib379>0)
				    {
				        //calibration done
				        float slope=(595-163)/(calib595-calib163);
				        float b = 163 - slope*calib163;

				        // check if 3rd point lays on the line
				        float midpoint = slope*calib379 + b;
				        if((midpoint - 379)>2 || (379 - midpoint)>2)
				        {
				            //bad calibration:
				            int i=0;
				            for(i=0;i<10;i++)
				            {
                                GPIO_write(Board_led_red,1);
                                Task_sleep(200);
                                GPIO_write(Board_led_red,0);
                                Task_sleep(200);
                            }
				        }
				        else
				        {
				            //good calibration:
				            int i=0;
                            for(i=0;i<5;i++)
                            {
                                Task_sleep(200);
                                GPIO_write(Board_led_blue,1);
                                Task_sleep(200);
                                GPIO_write(Board_led_blue,0);
                            }

                            WEIGHT_SLOPE = slope;
                            WEIGHT_Y0 = b;
				        }
				    }

                }

				log_write_new_entry(Seconds_get(), owl_ID,log_char, weight, tol, temp);

				// stop the weight measurement
#ifdef USE_ADS
				// change to fast = inexact mode
				ads1220_change_mode(&ads, ADS1220_RATE_1000_HZ, ADS1220_SINGLE_SHOT, ADS1220_TEMPERATURE_DISABLED);
                GPIO_enableInt(nbox_loadcell_data_ready);

#endif
			}
//			else if(res == OWL_LEFT)
//			{
//				// owl has left the perch...
//				series_completed = 1;
//				log_write_new_entry(Seconds_get(), owl_ID,'W', 0);
//#ifdef USE_ADS
//				// change to fast = inexact mode
//				ads1220_change_mode(&ads, ADS1220_RATE_1000_HZ, ADS1220_SINGLE_SHOT);
//#endif
//				Task_sleep(T_LOADCELL_POLL);
//			}
			else if(res == UNSTABLE) // owl is still on the perch, but not stable
			{
				Task_sleep(T_LOADCELL_POLL);
			}
		}
	}

}

void load_cell_deep_sleep()
{
	spi1_arch_close();
}

void load_cell_isr()
{
	//check interrupt source
	Semaphore_post((Semaphore_Handle)semLoadCellDRDY);
}
