/*
 * rfid_reader.c
 *
 *  Created on: 27 Mar 2017
 *      Author: raffael
 */

#include "rfid_reader.h"
#include "ST95HF.h"
#include "MLX90109_library/mlx90109.h"
#include "MLX90109_library/mlx90109_params.h"
#include "uart_helper.h"
#include "logger.h"
#include <msp430.h>


#include <time.h>
#include <ti/sysbios/hal/Seconds.h>
#include <xdc/runtime/Timestamp.h>


#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

#ifndef LF_RFID
/*************** HF: **********************/
#include "95HF_library/lib_ConfigManager.h"

/**** STUFF FROM EXAMPLE CODE ************/
#include "NDEF/lib_NDEF_URI.h"
#include "NDEF/lib_NDEF_SMS.h"
#include "NDEF/lib_NDEF_Text.h"
#include "NDEF/lib_NDEF_Email.h"
#include "NDEF/lib_NDEF_Geo.h"
#include "NDEF/lib_NDEF.h"

extern uint8_t 					TagUID[10];
extern ISO14443A_CARD 	ISO14443A_Card;
extern ISO14443B_CARD 	ISO14443B_Card;
extern FELICA_CARD 			FELICA_Card;
extern uint8_t 					NDEF_Buffer [];

/* Variables for the different modes */
extern DeviceMode_t 				devicemode;
extern TagType_t 			nfc_tagtype;
char loggedUID[20] = {' '};

#else
/*************** LF: **********************/
static mlx90109_t mlx_dev;
static tagdata lf_tagdata;

#endif

void rfid_Task()
{
	uart_debug_open();
	log_startup();

	/* Initialize LF reader */
	mlx90109_params_t mlx_params = MLX90109_PARAMS;
	mlx90109_init(&mlx_dev, &mlx_params);

    while (1) {

#ifdef HF_RFID
    	Task_sleep(100); //Semaphore_pend((Semaphore_Handle)semReader, BIOS_WAIT_FOREVER);

    		if(initialized == 0)
    		{
    			st95_init_spi();
//    			st95_startup();
    			ConfigManager_HWInit();

    			GPIO_write(Board_led_green,1);
			Task_sleep(1500);
			GPIO_write(Board_led_green,0);
			GPIO_write(Board_led_blue,0);
//    			button_pressed = 0;
    			initialized = 1;
//    			st95_echo();
    		}
    		else if(initialized == 1){
    			Semaphore_pend((Semaphore_Handle)semReader, BIOS_WAIT_FOREVER); //just wait for the button once and then loop forever.
//    			Semaphore_post((Semaphore_Handle)semReader);

    			uint8_t status;
			int8_t TagType = TRACK_NOTHING;
			bool FirstTagFounded = true;
			char LastUIDFound[20] = {' '};
			bool NewTagDetected = false;

    			/* Scan to find if there is a tag */
			TagType = ConfigManager_TagHunting(TRACK_ALL);

			/* Tag has been discovered, clear screen */
			if( TagType != TRACK_NOTHING && FirstTagFounded == true)
			{
				FirstTagFounded = false;
			}

			switch(TagType)
			{
				case TRACK_NFCTYPE1:
					if(memcmp (LastUIDFound, TagUID, 6))
					{
						memcpy(LastUIDFound,TagUID,6);
						//Reading TT1...
						NewTagDetected = true;
					}
					break;

				case TRACK_NFCTYPE2:
					if(memcmp (LastUIDFound, ISO14443A_Card.UID, ISO14443A_Card.UIDsize))
					{
						memcpy(LastUIDFound,ISO14443A_Card.UID,ISO14443A_Card.UIDsize);
						//Reading TT2...      ");
						NewTagDetected = true;
					}
					break;

				case TRACK_NFCTYPE3:
					if(memcmp (LastUIDFound, FELICA_Card.UID, 8))
					{
						memcpy(LastUIDFound,FELICA_Card.UID,8);
						//"Reading TT3...      ");
						NewTagDetected = true;
					}
					break;

				case TRACK_NFCTYPE4A:
					if(memcmp (LastUIDFound, ISO14443A_Card.UID, ISO14443A_Card.UIDsize))
					{
						memcpy(LastUIDFound,ISO14443A_Card.UID,ISO14443A_Card.UIDsize);
						//Reading TT4A...     ");
						NewTagDetected = true;
					}
					break;

				case TRACK_NFCTYPE4B:
					if(memcmp (LastUIDFound, ISO14443B_Card.PUPI, 4))
					{
						memcpy(LastUIDFound,ISO14443B_Card.PUPI, 4);
						//"Reading TT4B...     ");
						NewTagDetected = true;
					}
					break;

				case TRACK_NFCTYPE5:
					if(memcmp (LastUIDFound, TagUID, 8))
					{
						memcpy(LastUIDFound,TagUID, 8);
						//Reading TT5...      ");
						NewTagDetected = true;
					}
					break;

				default:
//					GPIO_write(Board_led_IR,1);
					break;

    			}


//			/* Check the tag type found */
//			if (NewTagDetected == true)
//			{
//				NewTagDetected = false;
//				/* Try to read the NDEF message inside the tag */
//				status = readNDEFfromTAG();
//				if (status == PCDNFC_ERROR_MEMORY_INTERNAL)
//				{
//					//Cannot fill internal memory with NDEF message
//					GPIO_write(Board_led_IR,1);
//				}
//				else if (status == PCDNFC_ERROR_NOT_FORMATED)
//				{
//					// Empty tag      ");
//					GPIO_write(Board_led_IR,1);
//				}
//				else if (status != PCDNFC_OK)
//				{
//					//"Transmission Error
//
//					// In case of transmission error clear the LastUID
//					memset(LastUIDFound,'\0',20);
//				}
//			}


			PCD_FieldOff();

			/* Check the tag type found */
			if (NewTagDetected == true)
			{
//				uart_serial_write(&debug_uart, (uint8_t*)LastUIDFound, UID_LENGTH);
//
//				uint32_t timestamp = Seconds_get();
//				uart_serial_write(&debug_uart, (uint8_t*)timestamp, TIMESTAMP_LENGTH);

//				static uint8_t in_out = 0;
//				in_out = !in_out;
//				uart_serial_write(&debug_uart, &in_out, 1);

				memcpy(loggedUID, LastUIDFound, 20);


				GPIO_write(Board_led_green,1);
				GPIO_toggle(Board_led_blue);
				Task_sleep(100);
				GPIO_toggle(Board_led_blue);

				// log to flash
//				log_write_new_entry(timestamp, *(uint32_t*)LastUIDFound, in_out); //TODO: UID is an array casted into a uint32 --> wrong byte order!


			}
			else
			{
				memset(loggedUID, 0xff, sizeof loggedUID);
				GPIO_write(Board_led_green,0);
			}


    		}


	#endif

	#ifdef LF_RFID

		Semaphore_pend((Semaphore_Handle)semReader, BIOS_WAIT_FOREVER);
		// tag has been successfully read out.

		if(mlx_dev.p.tag_select ==  MLX_TAG_FDX)
		{
			//for FDX-B only: check CRC:
			if(mlx90109_format(&mlx_dev, &lf_tagdata) == MLX90109_OK)
			{
				lf_tagdata.valid = 1;
				rfid_stop_detection();
			}
		}
		else
		{

			lf_tagdata.tagId = *((uint64_t*)&mlx_dev.tagId[1]);
			lf_tagdata.valid = 1;
			rfid_stop_detection();
		}
	#endif
    }
}

int rfid_get_id(uint64_t* id)
{
	if(lf_tagdata.valid)
	{
		*id = lf_tagdata.tagId;
		return 1;
	}
	else
		return 0;
}

void rfid_start_detection()
{
#ifdef LF_RFID
	lf_tagdata.valid = 0;
	mlx90109_activate_reader(&mlx_dev);
#endif
}

void rfid_stop_detection()
{
	mlx90109_disable_reader(&mlx_dev, &lf_tagdata);
}

void nfc_wakeup_isr()
{
//IMPORTANT NOTE:
	/*When the ST95HF is configure to use the SPI serial interface, pin IRQ_OUT is used to give additional information to user. When the ST95HF is ready to send back a reply, it sends an Interrupt Request by setting a low level on pin IRQ_OUT, which remains low until the host reads the data.
  The application can use the Interrupt mode to skip the polling stage.
  */

//	GPIO_toggle(Board_led_blue);
	//check interrupt source

//	Hwi_enable(); //not sure if needed here??

//	GPIO_enableInt(lp_button);//not sure if needed here??
}

void lf_tag_read_isr()
{
//	int cnt = mlx_dev.counter;
//	mlx_dev.int_time[cnt] = TA3R;
	if(em4100_read(&mlx_dev)==MLX90109_DATA_OK)
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

}


