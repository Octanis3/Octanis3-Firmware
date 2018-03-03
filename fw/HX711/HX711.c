#include "HX711.h"

#define LOW 	0
#define HIGH 	1

uint8_t PD_SCK;		// Power Down and Serial Clock Input Pin
uint8_t DOUT;  	// Serial Data Output Pin
uint8_t GAIN = 1;		// amplification factor, default 128!
int32_t OFFSET = -8663406;	// used for tare weight
float SCALE = 2212.5;	// used to return weight in grams, kg, ounces, whatever

void hx711_begin(uint8_t dout, uint8_t pd_sck, uint8_t gain) {
	PD_SCK = pd_sck;
	DOUT = dout;

//	set_gain(gain);
	//After a reset or power-down event, input selection is default to Channel A with a gain of 128.
}

uint8_t hx711_is_ready(){
	return GPIO_read(DOUT) == LOW;
}

void hx711_set_gain(uint8_t gain) {
	switch (gain) {
		case 128:		// channel A, gain factor 128
			GAIN = 1;
			break;
		case 64:		// channel A, gain factor 64
			GAIN = 3;
			break;
		case 32:		// channel B, gain factor 32
			GAIN = 2;
			break;
	}

	GPIO_write(PD_SCK, LOW);
	hx711_read(); // gain is determined by the number of clock pulses (number of bits read out)
}

int32_t hx711_read() {
	// wait for the chip to become ready
	while (!hx711_is_ready()) {
		Task_sleep(10);
	}

	int32_t value = 0;
	uint32_t tmp = 0;

	// pulse the clock pin 24 times to read the data
	uint8_t i;

	for (i = 0; i < 24; i++) {
		GPIO_write(PD_SCK, HIGH);

		GPIO_write(PD_SCK, LOW);

		//bit order = MSBFIRST

		tmp = GPIO_read(DOUT);
		value |= (tmp << (23 - i));
	}

	// set the channel and the gain factor for the next reading using the clock pin
	for (i = 0; i < GAIN; i++) {
		GPIO_write(PD_SCK, HIGH);
		GPIO_write(PD_SCK, LOW);
	}

	// Replicate the most significant bit to pad out a 32-bit signed integer
	if (value & 0x00800000) 
		value |= 0xFF000000;

	return value;
}

int32_t hx711_read_average(uint8_t times) {
	int32_t sum = 0;
	uint8_t i;
	for (i = 0; i < times; i++) {
		sum += hx711_read();
		Task_sleep(100);
	}
	return sum / times;
}

double hx711_get_value(uint8_t times) {
	return hx711_read_average(times) - OFFSET;
}

float hx711_get_units(uint8_t times) {
	return hx711_get_value(times) / SCALE;
}

void hx711_tare(uint8_t times) {
	double sum = hx711_read_average(times);
	hx711_set_offset(sum);
}

void hx711_set_scale(float scale) {
	SCALE = scale;
}

float hx711_get_scale() {
	return SCALE;
}

void hx711_set_offset(int32_t offset) {
	OFFSET = offset;
}

long hx711_get_offset() {
	return OFFSET;
}

void hx711_power_down() {
	GPIO_write(PD_SCK, LOW);
	GPIO_write(PD_SCK, HIGH);
}

void hx711_power_up() {
	GPIO_write(PD_SCK, LOW);
}


