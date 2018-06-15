#ifndef EM4100_h
#define EM4100_h

#include "../MLX90109_library/mlx90109.h"

void em4095_startRfidCapture();
void em4095_stopRfidCapture();

//void onTimerOverflow();
//void onTimerCapture();
//void dumpCaptureInfo();
//void dumpCaptureData();
//uint8_t parseCaptureData();
//uint8_t getCardFacility();
//unsigned long getCardUid();

int16_t em4095_read(mlx90109_t *dev, uint8_t input_bit);

void Timer0_B1_ISR();

#endif
