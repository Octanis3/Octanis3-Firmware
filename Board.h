/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialization function definitions */
#include "nestbox_init.h"

/* XDCtools Header files */
#include <xdc/std.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>

#define Board_initGeneral           nbox_initGeneral
#define Board_initGPIO              nbox_initGPIO
#define Board_initSPI               nbox_initSPI
#define Board_initUART              nbox_initUART
#define Board_initWatchdog          nbox_initWatchdog


#define Board_button           		nbox_button
#define PIR_pin                     nbox_pir_in1

#define Board_led_data              nbox_led_data
#define Board_led_status            nbox_led_status

#ifdef ESP12_FLASH_MODE
    #define nbox_wifi_enable      nbox_5v_enable
#endif

#define Board_LED_ON					nbox_LED_ON
#define Board_LED_OFF				nbox_LED_OFF

#define Board_SPI0                  	nbox_SPI
#define nbox_spi_cs_n                 nbox_sdcard_enable_n

//#define Board_USBDEVICE             	0

#define Board_UART_wifi             nbox_UARTA0
#define Board_UART_debug            nbox_UARTA1


#define Board_WATCHDOG0             nbox_WATCHDOG

#define CPY_BUFF_SIZE               256

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
