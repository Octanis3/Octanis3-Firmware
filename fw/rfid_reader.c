/*
 * rfid_reader.c
 *
 *  Created on: 27 Mar 2017
 *      Author: raffael
 */

#include "rfid_reader.h"
#include "ST95HF.h"
#include "uart_helper.h"
#include "logger.h"

#include <time.h>
#include <ti/sysbios/hal/Seconds.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

#include "95HF_library/lib_ConfigManager.h"

//extern int button_pressed;

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
//extern bool 						KEYPress;
//extern bool							lockKEYUpDown;

//sURI_Info 				url;
//sSMSInfo 				sms;
//sEmailInfo 			email;
//sGeoInfo 				geo;
//
// /* PCD/PICC global memory space */
//
// /* TT1 (PCD only)*/
// uint8_t TT1Tag[NFCT1_MAX_TAGMEMORY];
//
// /* TT2 */
// uint8_t TT2Tag[NFCT2_MAX_TAGMEMORY];
//
// /* TT3 */
// uint8_t TT3Tag[NFCT3_MAX_TAGMEMORY];
// uint8_t *TT3AttribInfo = TT3Tag, *TT3NDEFfile = &TT3Tag[NFCT3_ATTRIB_INFO_SIZE];
//
// /* TT4 */
// uint8_t CardCCfile [NFCT4_MAX_CCMEMORY];
// uint8_t CardNDEFfileT4A [NFCT4_MAX_NDEFMEMORY];
// uint8_t CardNDEFfileT4B [NFCT4_MAX_NDEFMEMORY];
//
// /* TT5 (PCD only)*/
// uint8_t TT5Tag[NFCT5_MAX_TAGMEMORY];
//
// /* USB */
// bool 		USB_Control_Allowed = false;

 /* Variables for the different modes */
 extern DeviceMode_t 				devicemode;
 extern TagType_t 			nfc_tagtype;

//uint8_t readNDEFfromTAG(void)
//{
//	uint8_t status;
//	sRecordInfo RecordStruct;
//
//	/* Init information fields */
//	memset(url.Information,'\0',400);
//	memset(sms.Information,'\0',400);
//	memset(email.Information,'\0',400);
//	memset(geo.Information,'\0',100);
//
//	if (nfc_tagtype == TT1)
//	{
//		errchk(PCDNFCT1_ReadNDEF());
//	}
//	else if (nfc_tagtype == TT2)
//	{
//		errchk(PCDNFCT2_ReadNDEF());
//	}
//	else if (nfc_tagtype == TT3)
//	{
//		errchk(PCDNFCT3_ReadNDEF());
//	}
//	else if (nfc_tagtype == TT4A || nfc_tagtype == TT4B)
//	{
//		errchk(PCDNFCT4_ReadNDEF());
//	}
//	else if (nfc_tagtype == TT5)
//	{
//		errchk(PCDNFCT5_ReadNDEF());
//	}
//	else
//		return ERRORCODE_GENERIC;
//
//	//DONE
//
//	memset(NDEF_Buffer,'\0',20);
//	status = NDEF_IdentifyNDEF( &RecordStruct, NDEF_Buffer);
//	if(status == RESULTOK && RecordStruct.TypeLength != 0)
//	{
//		if (NDEF_ReadURI(&RecordStruct, &url)==RESULTOK)
//		{
//			//    URI content:
//		}
//		else if (NDEF_ReadSMS(&RecordStruct, &sms)==RESULTOK)
//		{
//			//    SMS content:
//		}
//		else if(NDEF_ReadEmail(&RecordStruct, &email)==RESULTOK)
//		{
//			// EMAIL content:
//		}
//		else if(NDEF_ReadGeo(&RecordStruct, &geo)==RESULTOK)
//		{
//			//   GEO content:
//		}
//		// This part has to be improved, it is just in order to write a simple text NDEF for the M24LR discovery
//		else if(RecordStruct.NDEF_Type == TEXT_TYPE)
//		{
//			//  TEXT content:
//		}
//		else if(RecordStruct.NDEF_Type == VCARD_TYPE)
//		{
//			// VCARD detected
//		}
//		else
//		{
//			// Unknown NDEF type
//		}
//	}
//	else if (RecordStruct.TypeLength == 0)
//	{
//		//  No NDEF content
//	}
//	else
//	{
//		//Error parsing NDEF
//	}
//
//	return RESULTOK;
//Error:
//	return status;
//}




void rfid_Task()
{
	uart_debug_open();
	log_startup();


	/* Initialize ST95HF as reader by default*/
	int initialized = 0;

    while (1) {
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
//    			Semaphore_pend((Semaphore_Handle)semReader, BIOS_WAIT_FOREVER); //just wait for the button once and then loop forever.
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
				uint32_t timestamp = Seconds_get();
//				uart_serial_write(&debug_uart, (uint8_t*)timestamp, TIMESTAMP_LENGTH);

				static uint8_t in_out = 0;
				in_out = !in_out;
//				uart_serial_write(&debug_uart, &in_out, 1);

				GPIO_write(Board_led_green,1);
				GPIO_toggle(Board_led_blue);
				Task_sleep(100);
				GPIO_toggle(Board_led_blue);

				// log to flash
				log_write_new_entry(timestamp, *(uint32_t*)LastUIDFound, in_out); //TODO: UID is an array casted into a uint32 --> wrong byte order!


			}
			else
			{
				GPIO_write(Board_led_green,0);
			}


    		}

    		/* Put ST95HF to deep sleep/low power mode */

    		/* Pend on reader semaphore (can be released either by light barrier interrupt,
    		 * in which case a read has to be performed, or by the user button, in which
    		 * case the ST95HF has to be switched into tag mode.
    		 */

    		// Semaphore pend ....

    		/* If barrier triggered, wake up and issue a read command */
		{
			//wake ST95HF

			//send read command

			//receive the ID

			//store ID in flash memory (--> send message to mailbox to be treated by
			// 								int. flash memory task)


		}
    		/* else if user button triggered, wake up/switch ST95HF into tag mode and
    		 * start to listen for incoming NFC read commands.
    		 */
		{
			//(wake ST95HF ?? if needed ??)

			//re-initialize ST95HF in tag-mode

			//wait for incoming NFC reads

				//read received command

				//if command == "send nestbox device ID" --> reply with ID.
					// continue waiting for incoming commands

				//if command == "send stored data" --> successively send the stored data
					// continue waiting for incoming commands

				//if command == "delete stored data" --> delete it
					// continue waiting for incoming commands

				//if command == "end communication" --> re-initialize ST95HF in reader mode
					// exit loop

			// if timeout (no incoming reads for 1 minute) --> re-initialize ST95HF in reader mode
				// exit loop
		}

		// TODO remove this:
//    		Task_sleep(100);
    }
}


void nfc_wakeup_isr()
{
//IMPORTANT NOTE:
	/*When the ST95HF is configure to use the SPI serial interface, pin IRQ_OUT is used to give additional information to user. When the ST95HF is ready to send back a reply, it sends an Interrupt Request by setting a low level on pin IRQ_OUT, which remains low until the host reads the data.
  The application can use the Interrupt mode to skip the polling stage.
  */

	GPIO_toggle(Board_led_blue);
	//check interrupt source

//	Hwi_enable(); //not sure if needed here??

//	GPIO_enableInt(lp_button);//not sure if needed here??
}

