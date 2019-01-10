/*
 * load_cell.c
 *
 *  Created on: 03 Mar 2018
 *      Author: raffael
 */

#include "load_cell.h"
#include "ADS1220/ads1220.h"

#include "../Board.h"

#include "rfid_reader.h"
#include "logger.h"

#include <ti/sysbios/hal/Seconds.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#define WEIGHT_THRESHOLD 50000 // raw ADC units --> ADC_VAL = 1098.9 * GRAMS + OFFSET; R^2 = 0.99999

#ifdef USE_HX
	#define SAMPLE_RATE		HX_SAMPLE_RATE //Hz
#endif
#ifdef USE_ADS
	#define SAMPLE_RATE		ADS_SLOW_SAMPLE_RATE //Hz
#endif

#define MIN_EVENT_TIME 	    10 //seconds
#define MIN_ABSENCE_TIME    10 //cycles ~ seconds
#define N_AVERAGES		    10
#define EVENT_BUF_SIZE	SAMPLE_RATE*MIN_EVENT_TIME/N_AVERAGES //need to account for 10 averaging window already in place!

#define PLUS_SIGN 		' '
#define MINUS_SIGN		'-'

#define RFID_TIMEOUT		200 	//ms
#define T_RFID_RETRY		1000 	//ms
#define T_LOADCELL_POLL	1000 	//ms

#define TARE_TOLERANCE      6000    // maximum variation to get a new tare value
#define SAMPLE_TOLERANCE 	1000		// maximum variation of the sampled values within N_AVERAGES samples
#define WEIGHT_TOLERANCE 	50 	// maximum deviation from average value within one measurement series
//#define WEIGHT_MAX_CHANGE	100	// maximum change within one "event"

//#define RAW_THRESHOLD       1000
// TODO: above values should be in %FS

Semaphore_Handle semLoadCellDRDY;

static int32_t last_stored_weight = 0;
static int32_t last_measured_offset = 0;
static int tare_request = 0;
static int threshold_bypass_request = 0;

int32_t get_last_stored_weight()
{
    return last_stored_weight;
}
int32_t get_weight_offset()
{
    return last_measured_offset;
}
int32_t get_weight_threshold()
{
    return last_measured_offset + WEIGHT_THRESHOLD;
    // todo
}
void set_weight_threshold(int32_t new_th)
{
    //todo
}

void load_cell_bypass_threshold(int status)
{
    threshold_bypass_request = status;
    log_write_new_entry('C', status);
}

void load_cell_trigger_tare()
{
    tare_request = 1;
}
typedef enum weightResultStatus_
{
	OWL_LEFT = 0,
	UNSTABLE,
	STABLE,
	OWL_CAME_BACK
} weightResultStatus;


weightResultStatus load_cell_get_stable(struct Ads1220 *ads, uint8_t type) //type = 'X' for owl or 'O' for offset measurement
{
	static int32_t meas_buf[EVENT_BUF_SIZE] = {0,};
	static int first_valid = 0;
//	static int first_invalid = 0;

	int i = 0;
	int tmp = first_valid;
	int values_recorded = 0;
	int32_t deviation = 0;

    static unsigned int threshold_cnt = 0;

	// fill circular buffer with new measurements
	while(values_recorded < EVENT_BUF_SIZE)
	{
#ifdef USE_HX
		meas_buf[tmp] = hx711_get_units(N_AVERAGES, &deviation);
#endif
#ifdef USE_ADS
		meas_buf[tmp] = ads1220_read_average(N_AVERAGES, &deviation, ads);
#endif
		log_write_new_weight_entry(type, meas_buf[tmp], 0x0000ffff & deviation);
		last_stored_weight = meas_buf[tmp];

		if(meas_buf[tmp] < ads->cont_threshold)
		{
		    threshold_cnt = threshold_cnt+1;

		    if(threshold_cnt>100) // measure zero value 100 times!
		    {
                first_valid = 0;
                threshold_cnt = 0;
    //			first_invalid = 0;
                return OWL_LEFT;
		    }
		    else if(threshold_cnt > MIN_ABSENCE_TIME)
		    {
		        type = 'O';
		    }
		    continue;
		}
		else
		{
		    if(type == 'O') // the owl clearly left and potentially another came back --> re-start the series!
		    {
	            threshold_cnt = 0;
		        return OWL_CAME_BACK;
		    }
		    //else:
            threshold_cnt = 0;
		}


		if(deviation > SAMPLE_TOLERANCE)
			continue;

		tmp = tmp + 1;
		if(tmp >= EVENT_BUF_SIZE)
			tmp = 0;

		values_recorded = values_recorded+1;
	}

	// calculate average over circular buffer:
	int32_t average = meas_buf[0];
	int32_t min = average;
	int32_t max = average;

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

	int32_t tol = (max - min);
	if(tol < ads->tolerance)
	{
		ads->stable_weight = average;
		ads->tolerance = tol;
        GPIO_write(Board_led_status,1);
        Task_sleep(200);
        GPIO_write(Board_led_status,0);
	}

	if(tol < WEIGHT_TOLERANCE)
	{
        log_write_new_weight_entry('S', average, tol);

		GPIO_write(Board_led_status,1);
        Task_sleep(2000);
        GPIO_write(Board_led_status,0);
		return STABLE;
	}

	else
	{
        log_write_new_weight_entry('A', average, tol);
		return UNSTABLE;
	}
}

void ads1220_set_init_loadcell_config(struct Ads1220 *ads){
	ads->config.mux = ADS1220_MUX_AIN1_AIN2;
	ads->config.gain = ADS1220_GAIN_128;
	ads->config.pga_bypass = 0;
	ads->config.rate = ADS1220_RATE_20_HZ;
	// todo: change operating mode to duty-cycle mode
	ads->config.conv = ADS1220_CONTINIOUS_CONVERSION;
	ads->config.temp_sensor = ADS1220_TEMPERATURE_DISABLED;
	ads->config.vref = ADS1220_VREF_EXTERNAL_AIN; // this will be toggled with AC excitation
	ads->config.idac = ADS1220_IDAC_OFF;
	ads->config.i1mux = ADS1220_IMUX_OFF;
	ads->config.i2mux = ADS1220_IMUX_OFF;
	ads->config.low_switch = 1; // Switch is closed after START/SYNC command
	ads->config.filter = ADS1220_FILTER_NONE; //At data rates of 5 SPS and 20 SPS, the filter can be configured to reject 50-Hz or 60-Hz line frequencies or to simultaneously reject 50 Hz and 60 Hz!!!
}

struct Ads1220 ads;

void load_cell_Task()
{
	Task_sleep(1000); //wait until things are settled...

	//storage for measurement series
	char event_ongoing = 0;
	char series_completed = 0;
    unsigned int offset_counter = 2000;

	uint64_t owl_ID = 0;
	int rfid_type = 0;

	/* Initialize the Data READY semaphore */
	Semaphore_Params semParams;
	Error_Block eb;
	Error_init(&eb);
	Semaphore_Params_init(&semParams);
	semParams.mode = Semaphore_Mode_BINARY;
	semLoadCellDRDY = Semaphore_create(0, &semParams, &eb);

	// turn on analog supply:
	GPIO_write(nbox_loadcell_ldo_enable, 1);

	spi1_init();
	struct spi_periph ads_spi;

	ads1220_init(&ads, &ads_spi, nbox_loadcell_spi_cs_n);
	ads1220_set_init_loadcell_config(&ads);
	Task_sleep(10);
	ads1220_event(&ads);
	ads1220_configure(&ads); // this one sends the reset and config

    Task_sleep(100);

	int32_t max_cont_deviation = 0;
    int32_t max_periodic_deviation = 0;


    // Try to find the zero offset
    while(1)
    {
        int i = 1;
        for(i=1; i<20; i++)
        {
            ads1220_tare(20, &max_cont_deviation, &max_periodic_deviation, &ads);
            if((max_cont_deviation + max_periodic_deviation) < i*TARE_TOLERANCE)
            {
                last_measured_offset = ads.cont_offset;
                ads1220_set_thresholds(&ads, WEIGHT_THRESHOLD);
                break;
            }
        }
        if(i<20)
            break;
        else // Tare failed
            Task_sleep(30000);
    }

	ads1220_change_mode(&ads, ADS1220_RATE_1000_HZ, ADS1220_SINGLE_SHOT, ADS1220_TEMPERATURE_DISABLED);
	// !! "every write access to any configuration register also starts a new conversion" !!

	Semaphore_pend((Semaphore_Handle)semLoadCellDRDY, 100); // timeout 100 ms in case DRDY pin is not connected
    Task_sleep(100);

	while(1)
	{
		// currently no event detected & reader was off
		if(!event_ongoing || series_completed)
		{
            /************ADS1220 POLLING*************/
            Semaphore_reset((Semaphore_Handle)semLoadCellDRDY, 0);
            ads1220_start_conversion(&ads);
            Semaphore_pend((Semaphore_Handle)semLoadCellDRDY, 100); // timeout 100 ms in case DRDY pin is not connected

            ads1220_periodic(&ads);
            ads1220_event(&ads);
            ads1220_powerdown(&ads);

            if((ads.data)>ads.periodic_threshold || threshold_bypass_request>0)
            {
	            log_write_new_entry('D', ((ads.data)>>8) & 0x0000ffff);
				if(event_ongoing==0)
				{
				    rfid_start_detection();
                    Semaphore_pend((Semaphore_Handle)semLoadCell,RFID_TIMEOUT);
                    rfid_stop_detection();

					rfid_type = rfid_get_id(&owl_ID);

					if(rfid_type>0 || threshold_bypass_request>0)
					{
	                    rfid_reset_detection_counts();

						// change to slow = exact mode
					    GPIO_disableInt(nbox_loadcell_data_ready);

						ads1220_change_mode(&ads, ADS1220_RATE_20_HZ, ADS1220_CONTINIOUS_CONVERSION, ADS1220_TEMPERATURE_DISABLED);
						ads.stable_weight = 0;
						ads.tolerance = SAMPLE_TOLERANCE;

						event_ongoing ='X';
						series_completed = 0;
					}
					else
						Task_sleep(T_RFID_RETRY);
				}
			}
            else if(tare_request) // user requested new tare
            {
                tare_request = 0;
                ads1220_tare(20, &max_cont_deviation, &max_periodic_deviation, &ads);
                if((max_cont_deviation + max_periodic_deviation) < TARE_TOLERANCE)
                {
                    last_measured_offset = ads.cont_offset;
                    ads1220_set_thresholds(&ads, WEIGHT_THRESHOLD);
                }
            }
			else
			{
				// periodically measure tare offset again
				if(offset_counter >= 3600 || event_ongoing == 'S') // ca. every 1 hour AND after a finished event that got a stable result
				{
				    event_ongoing = 'O'; //start a new offset measurement!
				    series_completed = 0;
                    // change to slow = exact mode
                    GPIO_disableInt(nbox_loadcell_data_ready);

                    ads1220_change_mode(&ads, ADS1220_RATE_20_HZ, ADS1220_CONTINIOUS_CONVERSION, ADS1220_TEMPERATURE_DISABLED);

				    offset_counter = 0;
				}
				else
				{
				    rfid_reset_detection_counts();
                    event_ongoing = 0;
                    offset_counter += 1;
				}

			}

			// polling delay...
			Task_sleep(T_LOADCELL_POLL);
		}

		if(event_ongoing>0 && series_completed==0)
		{
			// -->ID WAS DETECTED!

			//measure weight again with 10 averages:

			weightResultStatus res = load_cell_get_stable(&ads, event_ongoing);

			// measure temperature
			ads1220_change_mode(&ads, ADS1220_RATE_20_HZ, ADS1220_CONTINIOUS_CONVERSION, ADS1220_TEMPERATURE_ENABLED);
            GPIO_enableInt(nbox_loadcell_data_ready);

			Semaphore_reset((Semaphore_Handle)semLoadCellDRDY, 0);
	        ads1220_start_conversion(&ads);

			Semaphore_pend((Semaphore_Handle)semLoadCellDRDY, 100); // timeout 100 ms in case DRDY pin is not connected

			ads1220_read(&ads);
			ads1220_event(&ads);

			ads1220_convert_temperature(&ads);

			uint16_t temp = (uint16_t)((ads.temperature+273.15) * 10); //deci kelvins
            log_write_new_entry('T', temp);

            GPIO_disableInt(nbox_loadcell_data_ready);
			ads1220_change_mode(&ads, ADS1220_RATE_20_HZ, ADS1220_CONTINIOUS_CONVERSION, ADS1220_TEMPERATURE_DISABLED);

			if(res == STABLE || res == OWL_LEFT || res == OWL_CAME_BACK)
			{
				//stable event!! + mark series completed, but keep event ongoing (in order to not count it twice)!
				series_completed = 1;

	            if(res == OWL_CAME_BACK || res == OWL_LEFT)
	            {
	                event_ongoing = 0;
	            }
	            else if(res == STABLE)
	            {
	                event_ongoing = 'S';
	            }

				// stop the weight measurement

	            // change to fast = inexact mode
				ads1220_change_mode(&ads, ADS1220_RATE_1000_HZ, ADS1220_SINGLE_SHOT, ADS1220_TEMPERATURE_DISABLED);
	            ads1220_powerdown(&ads); //very important!

				GPIO_enableInt(nbox_loadcell_data_ready);
                Task_sleep(T_LOADCELL_POLL); // VERY IMPORTANT TO HAVE THIS, to get the ADC input discharged!
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
			    uint64_t dummy_owl_ID;
			    // re-check if bird is still here.
			    rfid_start_detection();
                Semaphore_pend((Semaphore_Handle)semLoadCell,RFID_TIMEOUT);
                rfid_stop_detection();

                if(!rfid_get_id(&dummy_owl_ID))
                {
                    log_write_new_entry('U', (uint16_t)owl_ID); //un-detected RFID
                }

				Task_sleep(T_LOADCELL_POLL);
			}
		}
	}

}

void load_cell_power_down()
{
    ads1220_powerdown(&ads); //very important!

}

void load_cell_deep_sleep()
{
    load_cell_power_down();
	spi1_arch_close();
    //todo: redefine load SPI pins to be GPIOs with 0 as output.

}

void load_cell_isr()
{
	//check interrupt source
	Semaphore_post((Semaphore_Handle)semLoadCellDRDY);
}
