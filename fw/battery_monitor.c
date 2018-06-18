/*
 * logger.c
 *
 *  Created on: 18 June 2018
 *      Author: raffael
 */

#include "battery_monitor.h"
#include "../Board.h"
#include <msp430.h>

volatile uint16_t ADC_val = 0;
volatile uint8_t ADC_summing = 0;

#define ANALOG_PORTS			1
#define ANALOG_NUM_AVG		16

#define AIN_V_BAT_CH			6
#define AIN_V_BAT_ADDR		6
//battery voltage threshold levels (0%: 3V [for testing at ambient temperature]; 100% = 4.1V)
#define BAT_FULL		4800
#define BAT_EMPTY	3600 // (0.9V on each cell)

#define BAT_FS		(BAT_FULL-BAT_EMPTY)


//enum adc_status_{
//	IDLE, 	//not in use, and not to be triggered
//	ADC_BUSY, 	//wait for it to finish
//	START, 	//the periodic timer interrupt requires a new measurement, therefore start a new ADC conversion
//	DONE		//ADC conversion is done, read values to status variable
//} adc_status;

void ADC_init()
{
	//INPUT PIN: A6 = P2.3

	//adc_status = IDLE;
	//reference voltage (2.5V)
	REFCTL0 = REFVSEL_2;
	//4 adclock sh, msc set, adcon;
	ADC12CTL0 = ADC12SHT1_0 + ADC12SHT1_0 + ADC12MSC + ADC12ON;
	//prediv 1:4, adcshp, adc12div 1:8, SMCLK, sequence
	ADC12CTL1 = ADC12PDIV_1 + ADC12SHP + ADC12DIV_7 + ADC12SSEL_3 + ADC12CONSEQ_1;
	//12bit res, low power mode (max 50ksps)
	ADC12CTL2 = ADC12RES_2 + ADC12PWRMD;
	//
	ADC12CTL3 = 0x00;

	//Vref as reference
	ADC12MCTL6 = ADC12VRSEL_1 + AIN_V_BAT_CH + ADC12EOS;
//	ADC12MCTL6 = ADC12VRSEL_1 + AIN_A_EXT2_CH;
//	ADC12MCTL7 = ADC12VRSEL_1 + AIN_A_EXT3_CH;
//	ADC12MCTL8 = ADC12VRSEL_1 + AIN_A_EXT4_CH;
//	ADC12MCTL9 = ADC12VRSEL_1 + AIN_A_EXT5_CH + ADC12EOS;

	// interrupt
	ADC12IER0 = ADC12IE6; //interrupt generated after conversion of last value (i.e. ADC channel 6)
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
		Task_sleep(10);
	}

	//TODO: calculate result for right units

}



void goto_deepsleep(char lowbat)
{
	//TODO
	timer0_A_stop(); //stop timer to make it possible to turn off SMCLK.

	//todo: write stored data to flash before power down

	//todo: power off all modules

	__bis_SR_register(LPM4_bits + GIE);        // Enter LPM0 w/ interrupts
}

void battery_Task()
{
	ADC_init();

	while(1)
	{
		//Test battery status every 5 minutes
		ADC_update();


		Task_sleep(10000); //300000
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
