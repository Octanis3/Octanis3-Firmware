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
/** ============================================================================
 *  @file       nestbox_init.h
 *
 *  @brief      nestbox Board Specific APIs
 *
 *  The nestbox_init header file should be included in an application as
 *  follows:
 *  @code
 *  #include <nestbox_init.h>
 *  @endcode
 *
 *  ============================================================================
 */
#ifndef __NESTBOX_INIT_H
#define __NESTBOX_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/* LEDs on nestbox_board are active high. */
#define nb_LED_OFF (0)
#define nb_LED_ON  (1)

/*!
 *  @def    nb_GPIOName
 *  @brief  Enum of GPIO names on the nb dev board
 */
typedef enum nb_GPIOName {
    nb_button = 0,
	lp_button, // bottom left button on launchpad
    nb_led_green,
    nb_led_blue,
	nb_led_ir,

    nb_GPIOCOUNT
} nb_GPIOName;

/*!
 *  @def    nb_SPIName
 *  @brief  Enum of SPI names on the nb dev board
 */
typedef enum nb_SPIName {
    nb_SPIB0 = 0,

    nb_SPICOUNT
} nb_SPIName;

/*!
 *  @def    nb_UARTName
 *  @brief  Enum of UART names on the nb dev board
 */
typedef enum nb_UARTName {
    nb_UARTA1 = 0,

    nb_UARTCOUNT
} nb_UARTName;

/*!
 *  @def    nb_WatchdogName
 *  @brief  Enum of Watchdog names on the nb dev board
 */
typedef enum nb_WatchdogName {
    nb_WATCHDOG = 0,

    nb_WATCHDOGCOUNT
} nb_WatchdogName;


/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
extern void nb_initGeneral(void);

/*!
 *  @brief  Initialize board specific GPIO settings
 *
 *  This function initializes the board specific GPIO settings and
 *  then calls the GPIO_init API to initialize the GPIO module.
 *
 *  The GPIOs controlled by the GPIO module are determined by the GPIO_PinConfig
 *  variable.
 */
extern void nb_initGPIO(void);

/*!
 *  @brief  Initialize board specific SPI settings
 *
 *  This function initializes the board specific SPI settings and then calls
 *  the SPI_init API to initialize the SPI module.
 *
 *  The SPI peripherals controlled by the SPI module are determined by the
 *  SPI_config variable.
 */
extern void nb_initSPI(void);

/*!
 *  @brief  Initialize board specific UART settings
 *
 *  This function initializes the board specific UART settings and then calls
 *  the UART_init API to initialize the UART module.
 *
 *  The UART peripherals controlled by the UART module are determined by the
 *  UART_config variable.
 */
extern void nb_initUART(void);

/*!
 *  @brief  Initialize board specific Watchdog settings
 *
 *  This function initializes the board specific Watchdog settings and then
 *  calls the Watchdog_init API to initialize the Watchdog module.
 *
 *  The Watchdog peripherals controlled by the Watchdog module are determined
 *  by the Watchdog_config variable.
 */
extern void nb_initWatchdog(void);



#endif /* __nestbox_init_H */
