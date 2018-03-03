#ifndef HX711_h
#define HX711_h

#include <stdint.h>
#include "../../Board.h"


// Allows to set the pins and gain later than in the constructor
void hx711_begin(uint8_t dout, uint8_t pd_sck, uint8_t gain);

// check if HX711 is ready
// from the datasheet: When output data is not ready for retrieval, digital output pin DOUT is high. Serial clock
// input PD_SCK should be low. When DOUT goes to low, it indicates data is ready for retrieval.
uint8_t hx711_is_ready();

// set the gain factor; takes effect only after a call to read()
// channel A can be set for a 128 or 64 gain; channel B has a fixed 32 gain
// depending on the parameter, the channel is also set to either A or B
void hx711_set_gain(uint8_t gain);

// waits for the chip to be ready and returns a reading
int32_t hx711_read();

// returns an average reading; times = how many times to read
int32_t hx711_read_average(uint8_t times);

// returns (read_average() - OFFSET), that is the current value without the tare weight; times = how many readings to do
double hx711_get_value(uint8_t times);

// returns get_value() divided by SCALE, that is the raw value divided by a value obtained via calibration
// times = how many readings to do
float hx711_get_units(uint8_t times);

// set the OFFSET value for tare weight; times = how many times to read the tare value
void hx711_tare(uint8_t times);

// set the SCALE value; this value is used to convert the raw data to "human readable" data (measure units)
void hx711_set_scale(float scale);

// get the current SCALE
float hx711_get_scale();

// set OFFSET, the value that's subtracted from the actual reading (tare weight)
void hx711_set_offset(int32_t offset);

// get the current OFFSET
int32_t hx711_get_offset();

// puts the chip into power down mode
void hx711_power_down();

// wakes up the chip after power down mode
void hx711_power_up();

#endif /* HX711_h */
