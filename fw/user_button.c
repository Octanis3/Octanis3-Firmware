/*
 * user_button.c
 *
 *  Created on: 01 Apr 2017
 *      Author: raffael
 */

#include <ti/drivers/GPIO.h>

#include "user_button.h"
#include "logger.h"
#include "rfid_reader.h"
#include "../Board.h"

#include <ti/sysbios/hal/Hwi.h>

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

//MIN Test program:
#include "min/min.h"
#include "uart_helper.h"
#include <ti/sysbios/hal/Seconds.h>

#define WRITE_REQ 0x80

void user_button_Task()
{
	GPIO_enableInt(Board_button);
	Task_sleep(5000);
//
	// MIN test program
        // A MIN context (we only have one because we're going to use a single port).
        // MIN 2.0 supports multiple contexts, each on a separate port, but in this example
        // we will use just SerialUSB.
	uart_wifi_open();
	struct min_context min_ctx;
    // Initialize the single context. Since we are going to ignore the port value we could
    // use any value. But in a bigger program we would probably use it as an index.
	min_init_context(&min_ctx, 0);

	min_transport_reset(&min_ctx,0);

	uint8_t rx_bytes[32];
	unsigned short n_rx=0;
	unsigned char ctrl_byte = 0;

	while(1)
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
	    }while(min_ctx.rx_frame_state != RECEIVING_EOF);

	    ctrl_byte = min_ctx.rx_frame_payload_buf[0];


	    switch(ctrl_byte & 0x7f)
	    {
	    case 'Z':
	        if(ctrl_byte & WRITE_REQ)
	        {
	            uint32_t timestamp = min_ctx.rx_frame_payload_buf[1];
	            timestamp = timestamp<<8 + min_ctx.rx_frame_payload_buf[2];
                timestamp = timestamp<<8 + min_ctx.rx_frame_payload_buf[3];
                timestamp = timestamp<<8 + min_ctx.rx_frame_payload_buf[4];
	            Seconds_set(timestamp);

	            const char test_string[] = "set new time\n";

	            uart_serial_write(&debug_uart, (uint8_t*)test_string, sizeof(test_string));

	        }
	        uint32_t timestamp = Seconds_get();
	        unsigned char tx_buf[32];
	        tx_buf[0] = 'Z';
	        tx_buf[1] = timestamp >> 24;
            tx_buf[2] = timestamp >> 16;
            tx_buf[3] = timestamp >> 8;
            tx_buf[4] = timestamp;

	        min_send_frame(&min_ctx, 0x33U, tx_buf, 5);
	        break;
	    default:
	        break;
	    }


//		Semaphore_pend((Semaphore_Handle)semButton, BIOS_WAIT_FOREVER);

		/* Turn on data LED  */
		GPIO_write(Board_led_blue, Board_LED_ON);


//		log_restart();

		Task_sleep(1000); //avoid too many subsequent memory readouts
        GPIO_write(Board_led_blue, Board_LED_OFF);

//		GPIO_clearInt(Board_button);
//		GPIO_enableInt(Board_button);
	}
}


void user_button_isr(unsigned int index)
{
//	button_pressed = 1;
	GPIO_disableInt(Board_button);

	//check interrupt source
	Semaphore_post((Semaphore_Handle)semButton);
}
