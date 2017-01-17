#include <driverlib.h>

// *****************************************************************************
// MSP430FR5969 Unused Vectors
// *****************************************************************************
// UNUSED_HWI_ISR()
//
// The default linker command file created by CCS links all interrupt vectors to
// their specified address location. This gives you a warning for vectors that
// are not associated with an ISR function. The following function (and pragma's)
// handles all interrupt vectors.
//
// Just make sure you comment out the vector pragmas handled by your own code.
// For example, you will receive a "program will not fit into" error if you do
// not comment out the WDT vector below. This occurs since the linker tries to
// fit both of the vector addresses into the same memory locations ... and they
// won't fit.
// *****************************************************************************
#pragma vector = ADC12_VECTOR                                                   // ADC
#pragma vector = AES256_VECTOR                                                  // AES256
#pragma vector = COMP_E_VECTOR                                                  // Comparator E
#pragma vector = DMA_VECTOR                                                     // DMA
#pragma vector = PORT1_VECTOR                                                   // Port 1
#pragma vector = PORT2_VECTOR                                                   // Port 2
#pragma vector = PORT3_VECTOR                                                   // Port 3
#pragma vector = PORT4_VECTOR                                                   // Port 4
#pragma vector = RESET_VECTOR                                                   // Reset
#pragma vector = RTC_VECTOR                                                     // RTC
#pragma vector = SYSNMI_VECTOR                                                  // System Non-maskable
#pragma vector = TIMER0_A0_VECTOR                                               // Timer0_A5 CC0
#pragma vector = TIMER0_A1_VECTOR                                               // Timer0_A5 CC1-4, TA
#pragma vector = TIMER0_B0_VECTOR                                               // Timer0_B3 CC0
#pragma vector = TIMER0_B1_VECTOR                                               // Timer0_B3 CC1-2, TB
#pragma vector = TIMER1_A0_VECTOR                                               // Timer1_A3 CC0
#pragma vector = TIMER1_A1_VECTOR                                               // Timer1_A3 CC1-2, TA1
#pragma vector = TIMER2_A0_VECTOR                                               // Timer2_A3 CC0
#pragma vector = TIMER2_A1_VECTOR                                               // Timer2_A3 CC1, TA
#pragma vector = TIMER3_A0_VECTOR                                               // Timer3_A2 CC0
#pragma vector = TIMER3_A1_VECTOR                                               // Timer3_A2 CC1, TA
#pragma vector = UNMI_VECTOR                                                    // User Non-maskable
#pragma vector = USCI_A0_VECTOR                                                 // USCI A0 Receive/Transmit
#pragma vector = USCI_A1_VECTOR                                                 // USCI A1 Receive/Transmit
//#pragma vector = USCI_B0_VECTOR                                                 // USCI B0 Receive/Transmit
#pragma vector = WDT_VECTOR                                                     // Watchdog Timer
__interrupt void UNUSED_HWI_ISR (void)
{
    __no_operation();
}

