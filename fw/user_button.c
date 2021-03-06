/*
 * user_button.c
 *
 *  Created on: 01 Apr 2017
 *      Author: raffael
 */
#include <msp430.h>

#include <ti/drivers/GPIO.h>

#include "user_button.h"
#include "logger.h"

// get sensor values from these sources:
#include "rfid_reader.h"
#include "load_cell.h"
#include "battery_monitor.h"
#include "../Board.h"
#include "rtc.h"

#include <ti/sysbios/hal/Hwi.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

//MIN Test program:
#include "min/min.h"
#include "uart_helper.h"
#include <ti/sysbios/hal/Seconds.h>

#define WRITE_REQ 0x80

static int wifi_interrupt_triggered = 0;
static int start_interrupt_triggered = 0;

static int wifi_on = 0;

void user_button_Task()
{
    GPIO_enableInt(Board_button);
    GPIO_enableInt(Board_wifi_sense);

	Task_sleep(5000);

#ifdef ESP12_FLASH_MODE
	uart_wifi_set_floating();
#endif
	struct min_context min_ctx;
    // Initialize the single context. Since we are going to ignore the port value we could
    // use any value. But in a bigger program we would probably use it as an index.
	min_init_context(&min_ctx, 0);

	min_transport_reset(&min_ctx,0);

	uint8_t rx_bytes[32];
	unsigned short n_rx=0;
	unsigned char ctrl_byte = 0;

	int sd_card_detection_status = 0;

	while(1)
	{
	    if(GPIO_read(Board_wifi_sense))
	    {
	        GPIO_write(Board_led_data, Board_LED_ON);
	        wifi_on = 1;
	        wifi_interrupt_triggered=1;
	        P1IES |= BIT0; // set wifi sense to falling edge
	    }
	    else
	    {
	        P1IES &= ~BIT0; // set wifi sense to rising edge
            Semaphore_reset((Semaphore_Handle)semButton, 0);
            GPIO_clearInt(Board_button);
            GPIO_enableInt(Board_button);
            GPIO_clearInt(Board_wifi_sense);
            GPIO_enableInt(Board_wifi_sense);
	        Semaphore_pend((Semaphore_Handle)semButton, BIOS_WAIT_FOREVER);
	    }

	    if(start_interrupt_triggered) //the button on the PCB was pressed -> user asked for a memory flush to SD card.
	    {
	        //write stored data to flash
	        GPIO_write(Board_led_data, Board_LED_ON);
            log_restart();
            if(!wifi_on)
                GPIO_write(Board_led_data, Board_LED_OFF);
            start_interrupt_triggered = 0;
	    }

        if(wifi_interrupt_triggered)
        {
            Task_sleep(100); // wait to power up.

            if(GPIO_read(Board_wifi_sense))
            {
                uart_wifi_open();
                wifi_interrupt_triggered = 0;
                GPIO_clearInt(Board_button);
                GPIO_enableInt(Board_button);
                GPIO_clearInt(Board_wifi_sense);
                GPIO_enableInt(Board_wifi_sense);
            }
            else
            {
                // button was only pressed accidentally!
                wifi_interrupt_triggered = 0;
                wifi_on = 0;
                GPIO_write(Board_led_data, Board_LED_OFF);
            }
        }

        while(wifi_on)
        {
            do
            {
                n_rx = uart_serial_read(&wifi_uart, rx_bytes, 1);

                if(n_rx)
                {
                    min_poll(&min_ctx, rx_bytes, n_rx);
    //                uart_serial_write(&debug_uart, min_ctx.rx_frame_payload_buf, min_ctx.rx_frame_payload_bytes);
                }
                else
                {
            //	        const char test_string[] = "W\n";
            //	        uart_serial_write(&debug_uart, (uint8_t*)test_string, sizeof(test_string));
                }

            }while(min_ctx.rx_frame_state != RECEIVING_EOF && wifi_on);

            if(!wifi_on) //exit loop without looking at the input buffer
                break;

            uart_serial_read(&wifi_uart, rx_bytes, 1); //receive the EOF character
            min_poll(&min_ctx, rx_bytes, 1);

            ctrl_byte = min_ctx.rx_frame_payload_buf[0];

            switch(ctrl_byte & 0x7f)
            {
            case 'H': // heartbeat; just send a confirmation
            {
                unsigned char tx_buf[2];
                tx_buf[0] = 'H';
                tx_buf[1] = 1;

                min_send_frame(&min_ctx, 0x33U, tx_buf, 2);
                break; // DONT FORGET THIS!
            }
            case 'Z':
            {
                if(ctrl_byte & WRITE_REQ)
                {
                    uint32_t timestamp = (min_ctx.rx_frame_payload_buf[1]);
                    timestamp = (timestamp<<8) + (min_ctx.rx_frame_payload_buf[2]);
                    timestamp = (timestamp<<8) + (min_ctx.rx_frame_payload_buf[3]);
                    timestamp = (timestamp<<8) + (min_ctx.rx_frame_payload_buf[4]);
                    rtc_set_clock(timestamp);
                }
                //send back (new) time value
                uint32_t new_timestamp = Seconds_get();
                unsigned char tx_buf[5];
                tx_buf[0] = 'Z';
                tx_buf[1] = new_timestamp >> 24;
                tx_buf[2] = new_timestamp >> 16;
                tx_buf[3] = new_timestamp >> 8;
                tx_buf[4] = new_timestamp;

                min_send_frame(&min_ctx, 0x33U, tx_buf, 5);
                break;
            }
            case 'S':
            {
                if(ctrl_byte & WRITE_REQ)
                {
                    rtc_set_pause_times(min_ctx.rx_frame_payload_buf[1],
                                        min_ctx.rx_frame_payload_buf[2],
                                        min_ctx.rx_frame_payload_buf[3],
                                        min_ctx.rx_frame_payload_buf[4]);
                }
                // send back (new) values
                unsigned char tx_buf[5];
                tx_buf[0] = 'S';
                tx_buf[1] = rtc_get_p_hour();
                tx_buf[2] = rtc_get_p_min();
                tx_buf[3] = rtc_get_r_hour();
                tx_buf[4] = rtc_get_r_min();

                min_send_frame(&min_ctx, 0x33U, tx_buf, 5);
                break;
            }
            case 'B':
            {
                uint16_t vbat = battery_get_vbat();
                unsigned char tx_buf[3];
                tx_buf[0] = 'B';
                tx_buf[1] = vbat >> 8;
                tx_buf[2] = vbat;

                min_send_frame(&min_ctx, 0x33U, tx_buf, 3);

                break;
            }
            case 'W':
            {
                int32_t weight = get_last_stored_weight();
                unsigned char tx_buf[5];
                tx_buf[0] = 'W';
                tx_buf[1] = (unsigned char)(weight >> 24);
                tx_buf[2] = (unsigned char)(weight >> 16);
                tx_buf[3] = (unsigned char)(weight >> 8);
                tx_buf[4] = (unsigned char)(weight);

                min_send_frame(&min_ctx, 0x33U, tx_buf, 5);

                break;
            }
            case 'O':
            {
                int32_t weight = get_weight_offset();
                unsigned char tx_buf[5];
                tx_buf[0] = 'O';
                tx_buf[1] = (unsigned char)(weight >> 24);
                tx_buf[2] = (unsigned char)(weight >> 16);
                tx_buf[3] = (unsigned char)(weight >> 8);
                tx_buf[4] = (unsigned char)(weight);

                min_send_frame(&min_ctx, 0x33U, tx_buf, 5);

                break;
            }
            case 'D':
            {
                if(ctrl_byte & WRITE_REQ)
                {
                    uint32_t new_th = (min_ctx.rx_frame_payload_buf[1]);
                    new_th = (new_th<<8) + (min_ctx.rx_frame_payload_buf[2]);
                    new_th = (new_th<<8) + (min_ctx.rx_frame_payload_buf[3]);
                    new_th = (new_th<<8) + (min_ctx.rx_frame_payload_buf[4]);
                    set_weight_threshold(new_th);
                }
                int32_t weight = get_weight_threshold();
                unsigned char tx_buf[5];
                tx_buf[0] = 'D';
                tx_buf[1] = (unsigned char)(weight >> 24);
                tx_buf[2] = (unsigned char)(weight >> 16);
                tx_buf[3] = (unsigned char)(weight >> 8);
                tx_buf[4] = (unsigned char)(weight);

                min_send_frame(&min_ctx, 0x33U, tx_buf, 5);

                break;
            }
            case 'R':
            {
                uint64_t tag_id = 0;
                rfid_get_last_id(&tag_id);

                unsigned char rfid_tx_buf[5];
                rfid_tx_buf[0] = 'R';
                rfid_tx_buf[1] = (unsigned char)(tag_id >> 24);
                rfid_tx_buf[2] = (unsigned char)(tag_id >> 16);
                rfid_tx_buf[3] = (unsigned char)(tag_id >> 8);
                rfid_tx_buf[4] = (unsigned char)(tag_id);

                min_send_frame(&min_ctx, 0x33U, rfid_tx_buf, 5);

                break;
            }
            case 't': // trigger load cell tare and send confirmation
            {
                unsigned char tx_buf[2];
                tx_buf[0] = 't';
                tx_buf[1] = 1;

                min_send_frame(&min_ctx, 0x33U, tx_buf, 2);
                load_cell_trigger_tare();
                break; // DONT FORGET THIS!
            }
            case 'L': // trigger load cell tare and send confirmation
            {
               unsigned char tx_buf[2];
               tx_buf[0] = 'L';
               tx_buf[1] = 1;

               min_send_frame(&min_ctx, 0x33U, tx_buf, 2);
               load_cell_bypass_threshold(1);
               break; // DONT FORGET THIS!
            }
            case 'l': // trigger load cell tare and send confirmation
            {
                unsigned char tx_buf[2];
                tx_buf[0] = 'l';
                tx_buf[1] = 1;

                min_send_frame(&min_ctx, 0x33U, tx_buf, 2);
                load_cell_bypass_threshold(0);
                break; // DONT FORGET THIS!
            }
            case 'F':
            {
                unsigned char tx_buf[2];
                tx_buf[0] = 'F';
                tx_buf[1] = 1;

                min_send_frame(&min_ctx, 0x33U, tx_buf, 2);
                sd_card_detection_status = log_restart();
                break; // DONT FORGET THIS!
            }
            case 'f':
            {
                unsigned char tx_buf[2];
                tx_buf[0] = 'f';
                tx_buf[1] = sd_card_detection_status;

                min_send_frame(&min_ctx, 0x33U, tx_buf, 2);
                break; // DONT FORGET THIS!
            }

            default:
                break;
            }
        }

        if(wifi_interrupt_triggered)
        {
            uart_wifi_close();

            // turn off all special user modes:
            load_cell_bypass_threshold(0);
        }
	}
}


int user_wifi_enabled()
{
    return wifi_on;
}

void wifi_sense_isr(unsigned int index)
{
    //cautionary measure: set TX gpio to input
    P2OUT &= ~BIT0;
    P2SEL1 &= ~BIT0;

    GPIO_disableInt(Board_wifi_sense);

    if(GPIO_read(Board_wifi_sense)) //sense high voltage -> wifi module was turned on!
    {
        P1IES |= BIT0; // set wifi sense to falling edge
        GPIO_write(Board_led_data, Board_LED_ON);
        wifi_on = 1;
        //resume system, if on pause:
        Semaphore_post((Semaphore_Handle)semSystemPause);
        //handle wifi in task
        Semaphore_post((Semaphore_Handle)semButton);
    }
    else
    {
        GPIO_write(Board_led_data, Board_LED_OFF);
        wifi_on = 0;
        P1IES &= ~BIT0; // set wifi sense to rising edge
    }

    //mark that the interrupt source was wifi button
    wifi_interrupt_triggered = 1;
}


void user_button_isr(unsigned int index)
{
    GPIO_disableInt(Board_button);
    //Resume system, in case it was paused:
    Semaphore_post((Semaphore_Handle)semSystemPause);
    //handle button input:
    start_interrupt_triggered = 1;
    Semaphore_post((Semaphore_Handle)semButton);

}
