/*
 * logger.c
 *
 *  Created on: 18 June 2018
 *      Author: raffael
 */

#include "battery_monitor.h"
#include "../Board.h"
#include <msp430.h>
#include "load_cell.h"


volatile uint16_t ADC_val = 0;
volatile uint8_t ADC_summing = 0;

#define ANALOG_PORTS			1
#define ANALOG_NUM_AVG		16

#define AIN_V_BAT_CH			ADC12INCH_6
#define AIN_V_BAT_ADDR		6
//battery voltage threshold levels (0%: 3V [for testing at ambient temperature]; 100% = 4.1V)

#define BAT_FULL_16			51575 // 1.35*4*2^12*16/(2.5*(82+47)/47)
#define BAT_EMPTY_16			34383 // 3.6*2^12*16/(2.5*(82+47)/47) (0.9V on each cell)

#define BAT_FS		(BAT_FULL_16-BAT_EMPTY_16)

#define BAT_N_MEAS_BELOW_THRESHOLD		15	// if measured N times a voltage below threshold, turn off everything!
#define BAT_TEST_INTERVAL				1000 //milliseconds

//enum adc_status_{
//	IDLE, 	//not in use, and not to be triggered
//	ADC_BUSY, 	//wait for it to finish
//	START, 	//the periodic timer interrupt requires a new measurement, therefore start a new ADC conversion
//	DONE		//ADC conversion is done, read values to status variable
//} adc_status;

void ADC_init()
{
	//INPUT PIN: A6 = P2.3
	P2SEL0 |= BIT3;
	P2SEL1 |= BIT3;
	//adc_status = IDLE;
	//reference voltage (2.5V)
	REFCTL0 = REFVSEL_2;
	//4 adclock sh, msc set, adcon;
	ADC12CTL0 =  ADC12SHT0_2 + ADC12MSC + ADC12ON; //ADC12SHT0_x: These bits define the number of ADC12CLK cycles in the sampling period for registers ADC12MEM0 to ADC12MEM7 and ADC12MEM24 to ADC12MEM31
	//prediv 1:4, adcshp, adc12div 1:8, SMCLK, repeat-single-channel
	ADC12CTL1 = ADC12PDIV_0 + ADC12SHP + ADC12SSEL_0 + ADC12CONSEQ_0;
	//12bit res, low power mode (max 50ksps)
	ADC12CTL2 = ADC12RES_2;
	//ADC12CTL3 = 0x00;

	//Vref as reference
	ADC12MCTL0 = ADC12VRSEL_1 + AIN_V_BAT_CH;
//	ADC12MCTL6 = ADC12VRSEL_1 + AIN_A_EXT2_CH;
//	ADC12MCTL7 = ADC12VRSEL_1 + AIN_A_EXT3_CH;
//	ADC12MCTL8 = ADC12VRSEL_1 + AIN_A_EXT4_CH;
//	ADC12MCTL9 = ADC12VRSEL_1 + AIN_A_EXT5_CH + ADC12EOS;

	// interrupt
	ADC12IER0 = ADC12IE0; //interrupt generated after conversion of last value (i.e. ADC channel 6)
}

void ADC_update()
{
	int i;
	//reset buffer
	ADC_summing = 0;
	ADC_val = 0;

	//start ADC
	ADC12CTL0 |= ADC12ENC + ADC12SC;

	//wait for ADC to finish
	while(ADC_summing < ANALOG_NUM_AVG)
	{
		Task_sleep(100);
	}

	//TODO: calculate result for right units

}

void display_result()
{
	float vbat = ADC_val;
	vbat = vbat * 0.1047; // (1000/2^12*2.5*(82+47)/47/16) --> mV;

	print_load_cell_value(vbat, 'P');
}

void goto_deepsleep()
{
	//TODO
//	timer0_A_stop(); //stop timer to make it possible to turn off SMCLK.

	//todo: write stored data to flash before power down

	//todo: power off all modules
	GPIO_write(nbox_vbat_test_enable, 0);
	GPIO_write(nbox_wifi_enable_n, 1);
	GPIO_write(nbox_sdcard_enable_n, 1);
	GPIO_write(nbox_5v_enable, 0);
	GPIO_write(nbox_loadcell_ldo_enable, 0); // LDO UNUSED; BECAUSE WHEN OFF, THIS DRAWS TOO MUCH CURRENT!!
											// LDO itself consumes ca 10uA when ON, unconnected!
	//disable ADC (saves nothing on top of LPM4)
	ADC12CTL0 &= ~ADC12ON;

//	// disable all timers (saves nothing on top of LPM4) //-->350uA
	TA0CTL = MC__STOP;
	TA1CTL = MC__STOP;
	TA2CTL = MC__STOP;
	TA3CTL = MC__STOP;
	TB0CTL = MC__STOP;

	// disable clocks (saves nothing on top of LPM4)
	CSCTL0_H = CSKEY >> 8;
	CSCTL4 = LFXTOFF + SMCLKOFF + VLOOFF + HFXTOFF;        // turn off SMCLK
	CSCTL0_H = 0;                             // Lock CS registers

	//Todo: stop tick source
	Clock_tickStop();
	//Clock_stop();

	//disable all serial ports (makes it worse!):
//	load_cell_deep_sleep();
//	UART_close(Board_UART_debug);
	//__bic_SR_register(GIE);
	__bis_SR_register(LPM4_bits);        // Enter LPM0 with interrupts
}

void battery_Task()
{
	Task_sleep(2000);

	ADC_init();
	uint8_t counter=0;

	while(1)
	{
		//Test battery status every 5 minutes
		GPIO_write(nbox_vbat_test_enable,1);
		//100 us turn-on time max
		ADC_update();
		GPIO_write(nbox_vbat_test_enable,0);
		display_result();

		if(ADC_val < BAT_EMPTY_16)
			counter++;
		else
			counter=0;

		if(counter > BAT_N_MEAS_BELOW_THRESHOLD)
			goto_deepsleep();


		Task_sleep(BAT_TEST_INTERVAL); //300000
	}
}



// ADC interrupt after all values are read.
void ADC_ISR()
{
	//write the battery voltage value to the buffer
	ADC_val += ADC12MEM0;

	if(++ADC_summing < ANALOG_NUM_AVG)
	{
		//start another conversion series
		ADC12CTL0 |= ADC12ENC + ADC12SC;
	}
	else
	{
		//stop ADC (automatic)
		ADC12CTL0 &= ~ADC12ENC;
		//wake up CPU
//		__bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
	}
}
