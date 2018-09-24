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

//#define VERBOSE 		1
// #define LAUNCHPAD_PINDEF 1

#define LOG_VERBOSE 0 // define as 0 or 1!


//#define MLX_READER		1
#ifndef MLX_READER
	#define EM_READER		1
#endif

#define LF_RFID //TODO: remove all references to this

/* LEDs on nestbox_board are active high. */
#define nbox_LED_OFF (0)
#define nbox_LED_ON  (1)

/*!
 *  @def    nbox_GPIOName
 *  @brief  Enum of GPIO names on the nbox dev board
 */
typedef enum nbox_GPIOName {
    //inputs
	nbox_button = 0,

	nbox_lightbarrier_ext,
	nbox_lightbarrier_int,
	nbox_loadcell_data_ready,
	nbox_pir_in1,
	nbox_pir_in2,
#ifdef EM_READER
	nbox_lf_data,
	nbox_lf_clk,
#else
	nbox_lf_clk,
	nbox_lf_data,
#endif
	//outputs
    nbox_led_blue,
#ifndef LAUNCHPAD_PINDEF
    nbox_led_green,
#endif
	nbox_led_ir,
//	nbox_loadcell_clk,
	nbox_lf_modul,
	nbox_loadcell_spi_cs_n,
	nbox_loadcell_exc_a_p,
	nbox_loadcell_exc_a_n,
#ifndef LAUNCHPAD_PINDEF
	nbox_loadcell_exc_b_p,
	nbox_loadcell_exc_b_n,
#endif
	nbox_loadcell_ldo_enable,
	nbox_5v_enable,
//	nbox_5v_pwm_enable,
	                    /* Burst Mode Select. PWM must be driven HIGH during start-up.
						* When VOUT is in regulation, the PWM pin may be driven LOW
						* to command Burst Mode operation. Applying a voltage below
						* 400mV enables Burst Mode operation, providing a significant
						* efficiency improve- ment at light loads*/
	nbox_wifi_enable_n,
	nbox_sdcard_enable_n,
	nbox_vbat_test_enable,
	nbox_pir_enable,
    nbox_GPIOCOUNT
} nbox_GPIOName;

/*!
 *  @def    nbox_SPIName
 *  @brief  Enum of SPI names on the nbox dev board
 */
typedef enum nbox_SPIName {
    nbox_SPI = 0,

    nbox_SPICOUNT
} nbox_SPIName;

/*!
 *  @def    nbox_UARTName
 *  @brief  Enum of UART names on the nbox dev board
 */
typedef enum nbox_UARTName {
//    nbox_UARTA0 = 0,
	nbox_UARTA1,

    nbox_UARTCOUNT
} nbox_UARTName;

/*!
 *  @def    nbox_WatchdogName
 *  @brief  Enum of Watchdog names on the nbox dev board
 */
typedef enum nbox_WatchdogName {
    nbox_WATCHDOG = 0,

    nbox_WATCHDOGCOUNT
} nbox_WatchdogName;


/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings.
 */
extern void nbox_initGeneral(void);

/*!
 *  @brief  Initialize board specific GPIO settings
 *
 *  This function initializes the board specific GPIO settings and
 *  then calls the GPIO_init API to initialize the GPIO module.
 *
 *  The GPIOs controlled by the GPIO module are determined by the GPIO_PinConfig
 *  variable.
 */
extern void nbox_initGPIO(void);

/*!
 *  @brief  Initialize board specific SPI settings
 *
 *  This function initializes the board specific SPI settings and then calls
 *  the SPI_init API to initialize the SPI module.
 *
 *  The SPI peripherals controlled by the SPI module are determined by the
 *  SPI_config variable.
 */
extern void nbox_initSPI(void);

/*!
 *  @brief  Initialize board specific UART settings
 *
 *  This function initializes the board specific UART settings and then calls
 *  the UART_init API to initialize the UART module.
 *
 *  The UART peripherals controlled by the UART module are determined by the
 *  UART_config variable.
 */
extern void nbox_initUART(void);

/*!
 *  @brief  Initialize board specific Watchdog settings
 *
 *  This function initializes the board specific Watchdog settings and then
 *  calls the Watchdog_init API to initialize the Watchdog module.
 *
 *  The Watchdog peripherals controlled by the Watchdog module are determined
 *  by the Watchdog_config variable.
 */
extern void nbox_initWatchdog(void);



#endif /* __nestbox_init_H */
