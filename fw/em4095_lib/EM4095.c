#include "EM4095.h"
#include "../../Board.h"
#include <msp430.h>
#include "../rfid_reader.h"

typedef enum _state {
	STATE_INIT,
	STATE_FIND_PERIOD,
	STATE_CAPTURE,
	STATE_DONE
} State;

const enum { // Constants:
	inputCapturePin		= 8,		// ICP1 alias Arduino pin 8
	// Setup for Timer1 prescaler
	prescale					= 64,		// prescale factor (each tick 4 us @16MHz)
	prescaleBits 			= 0xB011,	// see Table 15-5 in the data sheet.
	// Buffer sizes
	codeLength				= 64,		// No of bits in the RFID code.
	captureSize				= 1 + 2 * codeLength / 4,	// 4 bit periods in each byte. Capture at least 2 x codeLength bit periods (we do not know where the start is, allow for phase offset of one bit)
	bitBufferSize 		= 2 * codeLength / 8,			// Room for 2 x codeLength bits
};

// Variables used during decoding
uint8_t bitBuffer[bitBufferSize];						// Each byte contains data for 4 full bit periods. LSB is first bit.
uint8_t cardFacility;      									// First 8 bits of payload
unsigned long cardUid; 									  // Last 32 bits of payload
uint8_t columnSums[4];

volatile uint16_t bitPeriod;
volatile uint8_t count;
volatile uint16_t shortMin;
volatile uint16_t shortMax;
volatile uint16_t longMin;
volatile uint16_t longMax;
volatile uint16_t discarded;						// Set when a transition length is discarded as invalid
volatile uint8_t captureData[captureSize];		// Each byte contains data for 4 full bit periods.
volatile uint8_t bitCount;


volatile State state;

void em4095_startRfidCapture() {
  state = STATE_INIT;
  TB0EX0 = TBIDEX__8; // extended division factor: 8 --> get 1 MHz
  TB0CTL = TBSSEL__SMCLK + CNTL__16 + ID__8 + MC__CONTINUOUS + TBCLR; // division factor --> get 125 kHz
  TB0CCTL2 = CM_1 + CCIE + SCS + CCIS_0 + CAP;   // CM_1 = rising edge, CCIS_0 = CCIxA, CAPture mode, synchronous capture; interrupt enable

  //TODO: check if overflow detection is necessary.

  //configure P1.5 input pin as CCI2A --> P1.5_ISEL = 01
  P1SEL0 |= BIT5;
  P1SEL1 &= ~BIT5;
}

void em4095_stopRfidCapture() {
	GPIO_disableInt(nbox_lf_data);

	state = STATE_DONE;
	TB0CTL = MC__STOP; // turn off timer
	TB0CCTL2 = CM_0;   // CM_0 = capture mode disabled.
}

int16_t em4095_read(mlx90109_t *dev, uint8_t input_bit)
{
	if((dev->counter_header==9))
	{
		dev->data[dev->counter] = input_bit;

		if(dev->nibble_counter==4 && dev->id_counter<10)
		{
			//crc
			dev->nibble_counter = 0;
			dev->tagId[dev->id_counter] = dev->data[dev->counter-1] + //
											((dev->data[dev->counter-2])<<1) +//
											((dev->data[dev->counter-3])<<2) +//
											((dev->data[dev->counter-4])<<3);
			uint8_t crc = dev->data[dev->counter-1] +//
								dev->data[dev->counter-2] +//
								dev->data[dev->counter-3] +//
								dev->data[dev->counter-4];


			if((crc&0x01) == (dev->data[dev->counter]&0x01))
			{
				// CRC OK!
				dev->nibble_counter = 0;
				dev->id_counter++;
			}
			else
			{
				// start over
				dev->counter_header=0;
				dev->counter = 0;
				dev->nibble_counter = 0;
				return MLX90109_CRC_NOT_OK;
			}

		}
		else
		{
			dev->nibble_counter++;
		}


		dev->counter++;
	}
	else if(dev->counter_header<9)
	{
		// Detect Header (111111111 	 9bit Header (following a stop-0)
		if (input_bit == 0)
		{// 0's
			dev->counter_header=0;
		}
		else
		{//1's
			dev->counter_header++;
		}
		dev->counter = 0;
		dev->nibble_counter = 0;
		dev->id_counter = 0;
	}
	// Data complete after 64 bit incl header
	if (dev->counter > 63-9 && (dev->counter_header==9))
	{
		dev->counter = 0;
		dev->counter_header = 0;
		return MLX90109_DATA_OK;
	}
	else
	{
		return MLX90109_OK;
	}
}



void Timer0_B1_ISR()
{
  switch (__even_in_range(TB0IV, TB0IV_TBIFG)) {
    case TA0IV_TA0CCR1:
      break;
    case TB0IV_TB0CCR2:
    		lf_tag_read_isr();
      break;
    case TA0IV_TA0IFG:
      break;
    default:
      break;
  }
}

/*

//// Called from ISR(TIMER1_OVF_vect)
//void EM4100::onTimerOverflow() {
//	// The timer has overflowed. Start all over
//	state = STATE_INIT;
//} // End onTimerOverflow()

// Called from ISR(TIMER1_CAPT_vect)
void em_onTimerCapture(uint16_t delta_t) {
  // Allways reset the counter
  TCNT1 = 0;

  // ICR1 holds the captured timer value
  if (ICR1 < 10 && state != STATE_DONE) { // ie 40 micro seconds
    // This is always an error, start over.
    // Probably no card is present.
    state = STATE_INIT;
  }

  switch (state) {
    case STATE_INIT:

//   Start all over. Setup variables.

      count = bitCount = discarded = 0;
      bitPeriod = 0xFFFF;
      for (byte i = 0; i < captureSize; i++) {
        captureData[i] = 0;
      }

      // This interrupt was on a rising edge - proceed to next state.
      state = STATE_FIND_PERIOD;
      break;

    case STATE_FIND_PERIOD:

     //  * Find the period used to transmit one bit.
     //  * Here we are only triggered on rising edges.

      if (delta_t < bitPeriod) {
        bitPeriod = delta_t;
      }
      count++;
      if (count == 8) {
        // Assumes 8 rising edges is long enough to see the quickest transitions.
        // Now we need lower and upper limits for comparisons in the next state. Use found period plus/minus 25%.
    	    uint16_t halfPeriod = bitPeriod >> 1;
    	    uint16_t delta = halfPeriod >> 2;
        shortMin  = halfPeriod - delta;
        shortMax  = halfPeriod + delta;
        delta = bitPeriod >> 2;
        longMin   = bitPeriod - delta;
        longMax   = bitPeriod + delta;

        count = 0;
        state = STATE_CAPTURE;
      }
      break;

    case STATE_CAPTURE:

//       * I want to decode both standard manchester encoding and biphase encoding.
//       * To allow that I capture two levels in each bit period.
//       * Then the main program can analyse the data after a sufficient number of periods have passed.
//       * Here we are triggered on both rising and falling edges.

      byte noOfLevelsToCapture;
      if (ICR1 >= shortMin && ICR1 <= shortMax) { // One half bit period
        noOfLevelsToCapture = 1;
      }
      else if (ICR1 >= longMin && ICR1 <= longMax) { // One full bit period
        noOfLevelsToCapture = 2;
      }
      else {
        // Transition time out of bounds. Restart
        state = STATE_INIT;
        discarded = ICR1;
        break;
      }
      // Add the current level to captureData
      byte level; // Split in two lines to avoid complaints from the compiler...
     level = digitalRead(inputCapturePin);
      for (byte i = 0; i < noOfLevelsToCapture; i++) {
        // count is index in captureData.
        // bitCount is index in current byte of captureData.
        // LSB is used first.
        if (level) { // We only need to SET bits as the array was initialized to zeroes.
          captureData[count] |= (1 << bitCount);
        }
        bitCount++;
        if (bitCount == 8) {
          bitCount = 0;
          count++;
          if (count == captureSize) { // Capture array is full
            state = STATE_DONE;
            // Might as well disable the interrupts for Timer 1.
            TIMSK1 = 0;
            break; // Exit for loop
          }
        }
      }

      // Toggle bit to trigger on the other edge next time
      TCCR1B ^= _BV(ICES1);
      break;

    case STATE_DONE:
      // Not much to do.
      break;

    default: // Error, start over
      state = STATE_INIT;
      break;
  }
} // End onTimerCapture()
*/

//
///*
// * The captureData array now contains two bits for each bit period.
// * We do not know the phase and type of encoding.
// * Try combinations of:
// *     - phase offset 0 and 1 bits
// *     - manchester and biphase encodings
// *     - normal and inverted output
// * Parses into bitBuffer and calls parseBitBuffer().
// * If decoding succeeds the results are store in cardFacility and cardUid.
// *
// * @return boolean    True if parsing and decoding succeeds.
// */
//boolean EM4100::parseCaptureData() {
//  byte level; // 0 or 1 - logic level of the sample being inspected
//  byte lastLevel; // The last level inspected
//  uint16_t count; // The number of samples handled
//  byte isTick; // True on every second sample
//  byte foundBit;
//  byte outByteCount;
//  byte outBitCount;
//  for (byte offset = 0; offset <= 1; offset++) {
//    for (byte invert = 0; invert <= 1; invert++) {
//      for (byte encoding = 0; encoding <= 1; encoding++) {
//        #ifdef DEBUG_EM4100
//          Serial.print("offset=");
//          Serial.print(offset);
//          Serial.print(" invert=");
//          Serial.print(invert);
//          Serial.print(" encoding=");
//          Serial.print(encoding);
//          Serial.print(": ");
//        #endif
//        // Initialize output buffer
//        for (count = 0; count < bitBufferSize; count++) {
//          bitBuffer[count] = 0;
//        }
//        count = outByteCount = outBitCount = 0;
//        for (byte byteCount = 0; byteCount < captureSize; byteCount++) {
//          for (bitCount = 0; bitCount < 8; bitCount++) {
//            lastLevel = level; // Not valid for first input, but that is fixed below
//            level = captureData[byteCount] & (1 << bitCount) ? 1 : 0;
//
//            // Special handling for first input
//            if (byteCount == 0 && bitCount == 0) {
//              // We know a transition happened before this sample
//              lastLevel = level ^ 1;
//              // Skip first sample if offset == 1
//              if (offset == 1) {
//                continue; // Next bitCount
//              }
//            }
//
//            // For both encodings we NEED a transition on each clock tick. Otherwise the phase offset is wrong or the sampled data is illegal.
//            isTick = count & 1 ? 0 : 1;
//            count++;
//            if (isTick && level == lastLevel) { // Transition missing!
//              #ifdef DEBUG_EM4100
//                Serial.print("\nMissing transition: byteCount=");
//                Serial.print(byteCount);
//                Serial.print(" bitCount=");
//                Serial.print(bitCount);
//                Serial.print(" count=");
//                Serial.println(count);
//              #endif
//              goto endOffset; // Next offset
//            }
//
//            // Extract a bit for every clock tick
//            if (encoding == 0) { // Manchester encoding
//              // The bit is the value after the clock tick transition
//              if ( ! isTick) {
//                continue;
//              }
//              foundBit = level;
//            }
//            else { // Biphase
//              // The bit is encoded between clock tick transitions: No transition => 1, a transition => 0.
//              if (isTick) {
//                continue;
//              }
//              foundBit = level == lastLevel ? 1 : 0;
//            }
//
//            // Invert?
//            if (invert) {
//              foundBit ^= 1;
//            }
//            #ifdef DEBUG_EM4100
//              Serial.print(foundBit);
//            #endif
//
//            // Add bit to output buffer
//            if (foundBit) {
//              bitBuffer[outByteCount] |= (1 << outBitCount);
//            }
//            outBitCount++;
//            if (outBitCount == 8) {
//              outBitCount = 0;
//              outByteCount++;
//              if (outByteCount == bitBufferSize) { // output buffer full
//                #ifdef DEBUG_EM4100
//                  Serial.println();
//                #endif
//                if (parseBitBuffer()) {
//                  return true;
//                }
//                goto endEncoding; // No need to parse more transitions.
//              }
//            }
//          } // End for(bitCount)
//        } // End for(byteCount)
//        #ifdef DEBUG_EM4100
//          Serial.println();
//        #endif
//        endEncoding:
//        count = 0; // This line only to allow the label above
//      } // End for(encoding)
//    } // End for(invert)
//    endOffset:
//    count = 0; // This line only to allow the label above
//  } // End for(offset)
//  // No cigar
//  return false;
//} // End parseCaptureData()
//
///*
// * The captured transitions have now been translated to a bit stream.
// * Try find data matching the EM4100 encoding.
// *
// * If decoding succeeds the results are store in cardFacility and cardUid.
// *
// * @return boolean    True if decoding succeeds.
// */
//boolean EM4100::parseBitBuffer() {
//  //
//  // Look for EM4100 data
//  //
//  byte theBit;
//
//  // Clear buffers
//  for (byte i = 0; i < 4; i++) {
//    columnSums[i] = 0;
//  }
//  cardFacility = 0;
//  cardUid = 0;
//
//  // Look for the header (9 1-bits in a row)
//  // We have 8*bitBufferSize bits input.
//  // The EM4100 data format is 64 bit long.
//  // There is no need to look for a header AFTER the 64th last bit
//  uint16_t lastStart = 8 * bitBufferSize - 64;
//  byte count = 0;
//  uint16_t i;
//  for (i = 0; i <= lastStart; i++) {
//    theBit = (bitBuffer[i / 8] >> (i % 8)) & 1;
//    if ( ! theBit) {
//      count = 0;
//      continue;
//    }
//    count++;
//    if (count == 9) {
//      break;
//    }
//  }
//  if (count != 9) { // Header not found
//    #ifdef DEBUG_EM4100
//      Serial.println(" - Header not found.");
//    #endif
//    return false;
//  }
//
//  // Bit i in bitBuffer is the 9th and final header bit (index 8).
//  // Verify that bit 64 is 0 (stop bit, index 63))
//  uint16_t offset = i - 8; // How many bits were skipped before the header started (ie index of fist header bit)
//  uint16_t index = offset + 63;
//  theBit = (bitBuffer[index / 8] >> (index % 8)) & 1;
//  if (theBit) {
//    #ifdef DEBUG_EM4100
//      Serial.println(" - Header found, but stopbit not 0.");
//    #endif
//    return false;
//  }
//
//  // Next: 2+8 rows of 4 bits and an even parity bit. MSB first.
//  // The first 2 rows are the customer id (or version number/manufacturer code), the last 8 rows the data.
//  // Collect the data in variables cardFacility (8 bits) and cardUid (32 bits)
//  byte nibble;
//  for (count = 0; count < 2; count ++) {
//    if ( ! parseNibble(offset + 9 + 5 * count, &nibble)) {
//      #ifdef DEBUG_EM4100
//        Serial.print(" - Wrong parity for facility, nibble ");
//        Serial.println(count);
//      #endif
//      return false;
//    }
//    cardFacility |= nibble << (4 * (1 - count)); // Most significant nibble first
//  }
//  for (count = 0; count < 8; count ++) {
//    if ( ! parseNibble(offset + 19 + 5 * count, &nibble)) {
//      #ifdef DEBUG_EM4100
//        Serial.print(" - Wrong parity for uid, nibble ");
//        Serial.println(count);
//      #endif
//      return false;
//    }
//    cardUid |= ((long)nibble) << (4 * (7 - count)); // Most significant nibble first
//  }
//
//  // Finally: 4 column parity bits, even
//  index = offset + 9 + 10 * 5;
//  for (i = 0; i < 4; i++) {
//    theBit = (bitBuffer[index / 8] >> (index % 8)) & 1;
//    if ((columnSums[i] & 1) != theBit) {
//      #ifdef DEBUG_EM4100
//        Serial.print(" - Wrong parity for column ");
//        Serial.println(i);
//      #endif
//      return false;
//    }
//    index++;
//  }
//
//  // Success!
//  return true;
//} // End parseBitBuffer()
//
///*
// * Extracts a nibble from the given offset in bitBuffer and checks for even parity in the 5th bit.
// *
// * @return boolean     True if the parity bit matches the data bits.
// */
//boolean EM4100::parseNibble(uint16_t offset, byte *nibble) {
//  byte theBit;
//  byte bitSum = 0;
//
//  *nibble = 0;
//  for (byte i = 0; i < 4; i++) {
//    theBit = (bitBuffer[offset / 8] >> (offset % 8)) & 1;
//    columnSums[i] += theBit; // Used later for column parity check
//    bitSum += theBit;
//    *nibble |= (theBit << (3 - i)); // First bit is MSB
//    offset++;
//  }
//
//  // Check for even parity
//  theBit = (bitBuffer[offset / 8] >> (offset % 8)) & 1;
//  if ((bitSum & 1) != theBit) {
//    return false;
//  }
//
//  return true;
//} // End parseNibble()
//
///*
// * Returns the found facility code.
// */
//byte EM4100::getCardFacility() {
//	return cardFacility;
//} // End getCardFacility();
//
///*
// * Returns the found UID.
// */
//unsigned long EM4100::getCardUid() {
//	return cardUid;
//} // End getCardUid();
//
