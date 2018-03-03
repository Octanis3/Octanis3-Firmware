/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
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

/*
 *  ======== nbox.c ========
 *  This file is responsible for setting up the board specific items for the
 *  nbox board.
 */

#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Timestamp.h>

#include <msp430.h>
#include <inc/hw_memmap.h>
#include <dma.h>
#include <eusci_a_uart.h>
#include <eusci_b_i2c.h>
#include <eusci_b_spi.h>
#include <gpio.h>
#include <nestbox_init.h>
#include <pmm.h>
#include <wdt_a.h>

#include <ti/drivers/SPI.h>

#include "fw/user_button.h"
#include "fw/lightbarrier.h"
#include "fw/rfid_reader.h" //for precompiler assignments

const SPI_Config SPI_config[];

/*
 *  =============================== General ===============================
 */
/*
 *  ======== nbox_initGeneral ========
 */
void nbox_initGeneral(void) {
    /*
     * Disable the GPIO power-on default high-impedance mode to activate
     * previously configured port settings
     */
    PM5CTL0 &= ~LOCKLPM5;
}

/*
 *  =============================== DMA ===============================
 */
/*
 *  ======== nbox_isrDMA ========
 *  This is a application defined DMA ISR. This ISR must map and call the
 *  appropriate Driver_event(handle) API to indicate completed DMA transfers.
 */
Void nbox_isrDMA(UArg arg)
{
    /* Call the SPI DMA function, passing the SPI handle used for WiFi */
    SPI_serviceISR((SPI_Handle) &(SPI_config[0]));
}

/*
 *  =============================== GPIO ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(GPIOMSP430_config, ".const:GPIOMSP430_config")
#endif

#include <ti/drivers/GPIO.h>
#include <ti/drivers/gpio/GPIOMSP430.h>

/*
 * Array of Pin configurations
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in nbox.h
 * NOTE: Pins not used for interrupts should be placed at the end of the
 *       array.  Callback entries can be omitted from callbacks array to
 *       reduce memory usage.
 */
GPIO_PinConfig gpioPinConfigs[] = {
    /* Input pins */
    /* nestbox user button */
#ifdef LAUNCHPAD_PINDEF
    GPIOMSP430_P4_5 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING,
#else
    GPIOMSP430_P4_2 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING,
#endif
#ifdef HF_RFID
	/* NESTBOX_SPI_NFC_IRQ_N */
	GPIOMSP430_P3_6 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING,
#else
	/* NESTBOX_LOADCELL_DATA */
	GPIOMSP430_P3_6 | GPIO_CFG_IN_PD | GPIO_CFG_IN_INT_NONE, //TODO: change back to pull up! load cell data pin must be low to stat that it is "ready".
#endif
	/* NESTBOX_LIGHTBARRIER_IN_EXT */
	GPIOMSP430_P1_4 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
	/* NESTBOX_LIGHTBARRIER_IN_INT */
	GPIOMSP430_P1_3 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
	/* NESTBOX_LF_CLK */
	GPIOMSP430_P2_4 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
	/* NESTBOX_LF_DATA */
	GPIOMSP430_P4_3 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_NONE,
	#ifdef LF_RFID
	/* NESTBOX_LF_FREQ_SELECT */
	GPIOMSP430_P3_5 | GPIO_CFG_IN_PD | GPIO_CFG_IN_INT_NONE,
	#endif

    /* Output pins */
#ifdef LAUNCHPAD_PINDEF
	/* NESTBOX_LED_BLUE */
    GPIOMSP430_P4_6 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
#else
	/* NESTBOX_LED_GREEN */
    GPIOMSP430_P4_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
	/* NESTBOX_LED_BLUE */
	GPIOMSP430_P4_1 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
#endif

	/* NESTBOX_LED_INFRARED ==> DO NOT USE P1.0 as TI-RTOS GPIO;
	 * --> directly modified at register level in lightbarrier task */
//	GPIOMSP430_P1_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
#ifdef HF_RFID
	/* NESTBOX_SPI_NFC_SEL_N */
	GPIOMSP430_P3_4 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
	/* NESTBOX_SPI_NFC_WAKEUP_N */
#ifdef LAUNCHPAD_PINDEF
	GPIOMSP430_P3_5 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
#else
	GPIOMSP430_P3_7 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
#endif
#else
	/* NESTBOX_LOADCELL_CLK */
	GPIOMSP430_P3_4 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
#endif
	/* NESTBOX_LF_MODUL */
	GPIOMSP430_P1_2 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
	/* NESTBOX_LF_MODE */
	GPIOMSP430_P3_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,
	/* NESTBOX_LF_SPEED */
	GPIOMSP430_P1_5 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,

};

/*
 * Array of callback function pointers
 * NOTE: The order of the pin configurations must coincide with what was
 *       defined in nbox.h
 * NOTE: Pins not used for interrupts can be omitted from callbacks array to
 *       reduce memory usage (if placed at end of gpioPinConfigs array).
 */
GPIO_CallbackFxn gpioCallbackFunctions[] = {
	user_button_isr,  /* nestbox user button */
	nfc_wakeup_isr, /* lightbarrier interrupt pin */
	lightbarrier_input_isr, /* lightbarrier detection routine */
	lightbarrier_input_isr, /* lightbarrier detection routine */
	lf_tag_read_isr,
};

/* The device-specific GPIO_config structure */
const GPIOMSP430_Config GPIOMSP430_config = {
    .pinConfigs = (GPIO_PinConfig *)gpioPinConfigs,
    .callbacks = (GPIO_CallbackFxn *)gpioCallbackFunctions,
    .numberOfPinConfigs = sizeof(gpioPinConfigs)/sizeof(GPIO_PinConfig),
    .numberOfCallbacks = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn)
};

/*
 *  ======== nbox_initGPIO ========
 */
void nbox_initGPIO(void)
{
    /* Initialize peripheral and pins */
    GPIO_init();
}


/*
 *  =============================== SPI ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(SPI_config, ".const:SPI_config")
#pragma DATA_SECTION(spiEUSCIBDMAHWAttrs, ".const:spiEUSCIBDMAHWAttrs")
#endif

#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPIEUSCIBDMA.h>

SPIEUSCIBDMA_Object spiEUSCIBDMAObjects[nbox_SPICOUNT];
uint8_t spiEUSCIBDMAscratchBuf[nbox_SPICOUNT];

const SPIEUSCIBDMA_HWAttrs spiEUSCIBDMAHWAttrs[nbox_SPICOUNT] = {
    {
        .baseAddr = EUSCI_B0_BASE,
        .clockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK,
        .bitOrder = EUSCI_B_SPI_MSB_FIRST,
        .scratchBufPtr = &spiEUSCIBDMAscratchBuf[0],
        .defaultTxBufValue = 0,

        /* DMA */
        .dmaBaseAddr = DMA_BASE,
        /* Rx Channel */
        .rxDMAChannelIndex = DMA_CHANNEL_0,
        .rxDMASourceTrigger = DMA_TRIGGERSOURCE_18,
        /* Tx Channel */
        .txDMAChannelIndex = DMA_CHANNEL_1,
        .txDMASourceTrigger = DMA_TRIGGERSOURCE_19
    }
};

const SPI_Config SPI_config[] = {
    {
        .fxnTablePtr = &SPIEUSCIBDMA_fxnTable,
        .object = &spiEUSCIBDMAObjects[0],
        .hwAttrs = &spiEUSCIBDMAHWAttrs[0]
    },
    {NULL, NULL, NULL},
};

/*
 *  ======== nbox_initSPI ========
 */
void nbox_initSPI(void)
{
    /* EUSCIB0 */
    /*
     * NOTE: TI-RTOS examples configure EUSCIB0 as either SPI or I2C.  Thus,
     * a conflict occurs when the I2C & SPI drivers are used simultaneously in
     * an application.  Modify the pin mux settings in this file and resolve the
     * conflict before running your the application.
     */

	// nestbox v1. uses UCB0 for SPI

    /* SOMI/MISO */
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
        GPIO_PIN7, GPIO_SECONDARY_MODULE_FUNCTION);

    /* SIMO/MOSI */
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1,
        GPIO_PIN6, GPIO_SECONDARY_MODULE_FUNCTION);

    /* CLK */
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,
        GPIO_PIN2, GPIO_SECONDARY_MODULE_FUNCTION);

    SPI_init();
}

/*
 *  =============================== UART ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(UART_config, ".const:UART_config")
#pragma DATA_SECTION(uartEUSCIAHWAttrs, ".const:uartEUSCIAHWAttrs")
#endif

#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTEUSCIA.h>

UARTEUSCIA_Object uartEUSCIAObjects[nbox_UARTCOUNT];

/*
 * The baudrate dividers were determined by using the MSP430 baudrate
 * calculator
 * http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
 */
const UARTEUSCIA_BaudrateConfig uartEUSCIABaudrates[] = {
    /* {baudrate, input clock, prescalar, UCBRFx, UCBRSx, oversampling} */
    {
        .outputBaudrate = 115200,
        .inputClockFreq = 8000000,
        .prescalar = 4,
        .hwRegUCBRFx = 5,
        .hwRegUCBRSx = 85,
        .oversampling = 1
    },
    {9600, 8000000, 52, 1, 0, 1},
    {9600,   32768,  3, 0, 3, 0},
};

const UARTEUSCIA_HWAttrs uartEUSCIAHWAttrs[nbox_UARTCOUNT] = {

#ifdef LAUNCHPAD_PINDEF
    {
        .baseAddr = EUSCI_A0_BASE,
        .clockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK,
        .bitOrder = EUSCI_A_UART_LSB_FIRST,
        .numBaudrateEntries = sizeof(uartEUSCIABaudrates)/sizeof(UARTEUSCIA_BaudrateConfig),
        .baudrateLUT = uartEUSCIABaudrates
    },
#else
	{
		.baseAddr = EUSCI_A1_BASE,
		.clockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK,
		.bitOrder = EUSCI_A_UART_LSB_FIRST,
		.numBaudrateEntries = sizeof(uartEUSCIABaudrates)/sizeof(UARTEUSCIA_BaudrateConfig),
		.baudrateLUT = uartEUSCIABaudrates
	},
#endif
};

const UART_Config UART_config[] = {
    {
        .fxnTablePtr = &UARTEUSCIA_fxnTable,
        .object = &uartEUSCIAObjects[0],
        .hwAttrs = &uartEUSCIAHWAttrs[0]
    },
//	{
//		.fxnTablePtr = &UARTEUSCIA_fxnTable,
//		.object = &uartEUSCIAObjects[1],
//		.hwAttrs = &uartEUSCIAHWAttrs[1]
//	},
    {NULL, NULL, NULL}
};

/*
 *  ======== nbox_initUART ========
 */
void nbox_initUART(void)
{
    /* P2.0,1 = USCI_A0 TXD/RXD */
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,
        GPIO_PIN0, GPIO_SECONDARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P2,
        GPIO_PIN1, GPIO_SECONDARY_MODULE_FUNCTION);

    /* P2.5,6 = USCI_A1 TXD/RXD */
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,
        GPIO_PIN5, GPIO_SECONDARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P2,
        GPIO_PIN6, GPIO_SECONDARY_MODULE_FUNCTION);

    /* Initialize the UART driver */
    UART_init();
}

/*
 *  =============================== Watchdog ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(Watchdog_config, ".const:Watchdog_config")
#pragma DATA_SECTION(watchdogMSP430HWAttrs, ".const:watchdogMSP430HWAttrs")
#endif

#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogMSP430.h>

WatchdogMSP430_Object watchdogMSP430Objects[nbox_WATCHDOGCOUNT];

const WatchdogMSP430_HWAttrs watchdogMSP430HWAttrs[nbox_WATCHDOGCOUNT] = {
    {
        .baseAddr = WDT_A_BASE,
        .sfrAddr = SFR_BASE,
        .clockSource = WDT_A_CLOCKSOURCE_SMCLK,
        .clockDivider = WDT_A_CLOCKDIVIDER_8192K
    },
};

const Watchdog_Config Watchdog_config[] = {
    {
        .fxnTablePtr = &WatchdogMSP430_fxnTable,
        .object = &watchdogMSP430Objects[0],
        .hwAttrs = &watchdogMSP430HWAttrs[0]
    },
    {NULL, NULL, NULL}
};

/*
 *  ======== nbox_initWatchdog ========
 */
void nbox_initWatchdog(void)
{
    /* Initialize the Watchdog driver */
    Watchdog_init();
}
