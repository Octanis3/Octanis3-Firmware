// ----------------------------------------------------------------------------
// myGpio.c  , define GPIO
// ----------------------------------------------------------------------------

//***** Header Files **********************************************************
#include <driverlib.h>                                                          // DriverLib include file
#include "myGpio.h"


//***** Defines ***************************************************************


//***** Global Variables ******************************************************


//*****************************************************************************
// Initialize GPIO
//*****************************************************************************
void initGPIO(void) {

    //**************************************************************************
	// Configure LaunchPad LEDs
    //**************************************************************************
   /* // Set pin P4.6 to output direction, then turn LED off
    GPIO_setAsOutputPin( GPIO_PORT_P4, GPIO_PIN6 );                             // Red LED (LED1)
    GPIO_setOutputLowOnPin( GPIO_PORT_P4, GPIO_PIN6 );*/

    // Set pin P1.0 to output direction and turn LED off, this pin is ised to
    GPIO_setAsOutputPin( GPIO_PORT_P1, GPIO_PIN0 );                             // Green LED (LED2)
    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN0 );


    //**************************************************************************
	// Configure OUTPUT PINS to control the CR95HF chip
    //**************************************************************************


	//Set pin P3.4 to output direction, this is connected to VSS of CR95H5
	GPIO_setAsOutputPin( GPIO_PORT_P3, GPIO_PIN4 );                             // VSS CR95HF
	GPIO_setOutputLowOnPin( GPIO_PORT_P3, GPIO_PIN4);

	//Set pin P3.6 to output direction, this is connected to VSS of CR95H5
//	GPIO_setAsOutputPin( GPIO_PORT_P3, GPIO_PIN6 );                             // VSS2 CR95HF
//	GPIO_setOutputLowOnPin( GPIO_PORT_P3, GPIO_PIN6);


    // Set pin P1.4 to output direction, pin connected to IRQ_IN pin in the CR95HF
    GPIO_setAsOutputPin( GPIO_PORT_P1, GPIO_PIN4 );                             // IRQ_IN in CR95HF
    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN4);

    // Set pin P3.5 to output direction, pin connected to SSI_0 pin in the CR95HF
    GPIO_setAsOutputPin( GPIO_PORT_P3, GPIO_PIN5 );                             // SSI_0 in CR95HF
    GPIO_setOutputLowOnPin( GPIO_PORT_P3, GPIO_PIN5);

//    	Set pin P3.6 to output direction, pin connected to SSI_1 pin in the CR95HF. This pin is set to Ground by the nfc03a1 architencture
//    GPIO_setAsOutputPin( GPIO_PORT_P3, GPIO_PIN6 );                             // SSI_1 in CR95HF
//    GPIO_setOutputLowOnPin( GPIO_PORT_P3, GPIO_PIN6);

    //**************************************************************************
    // SPI configuration PINS MASTER USCIB0
    //**************************************************************************

    // Configure SPI pins
    // Configure Pins for UCB0CLK

    // Set pin P1.3 to output direction, to Secondary Module UCB0STE (used as Chip Select in SPI 4 wire mode),
    //pin connected to SPI_SS (Slave Selected) in the CR95HF
    GPIO_setAsOutputPin( GPIO_PORT_P1, GPIO_PIN3 );                             // SPI_SS in CR95HF
    GPIO_setOutputHighOnPin( GPIO_PORT_P1, GPIO_PIN3);


     /* Select Port 2
     * Set Pin 2 to input Secondary Module Function, (UCB0CLK).
     */
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P2,
        GPIO_PIN2,
        GPIO_SECONDARY_MODULE_FUNCTION
        );

    // Configure Pins for UCB0TXD/UCB0SIMO, UCB0RXD/UCB0SOMI
    //Set P1.6, P1.7 as Secondary Module Function Input.
    /*
     * Select Port 1
     * Set Pin 6, 7 to input Secondary Module Function, (UCB0TXD/UCB0SIMO, UCB0RXD/UCB0SOMI).
     */
    GPIO_setAsPeripheralModuleFunctionInputPin(
        GPIO_PORT_P1,
        GPIO_PIN6 + GPIO_PIN7,
        GPIO_SECONDARY_MODULE_FUNCTION
        );


    PMM_unlockLPM5();


    //**************************************************************************
    // Configure external crystal pins
    //**************************************************************************
    // Set LFXT (low freq crystal pins) to crystal input (rather than GPIO)
    // Since HFXT is not used, we don't need to set these pins. But for the 
    // record, they are:
    //              GPIO_PIN6                            // HFXTIN on PJ.6
    //              GPIO_PIN7                            // HFXOUT on PJ.7
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_PJ,
            GPIO_PIN4 +                                  // LFXIN  on PJ.4
            GPIO_PIN5 ,                                  // LFXOUT on PJ.5
            GPIO_PRIMARY_MODULE_FUNCTION
    );

    //**************************************************************************
    // Output MSP clock signals to external pins
    // - This allows verifying the clocks with a logic analyzer
    //**************************************************************************
    // Output the ACLK and SMCLK signals to their respective pins - which allows you to
    // watch them with a logic analyzer (ACLK on P2.0, SMCLK on P3.4)
//    GPIO_setAsPeripheralModuleFunctionOutputPin(
//                    GPIO_PORT_P2,                        // Unfortunately, P2.0 is not pinned out to FR5969 Boosterpack pin
//                    GPIO_PIN0,                           // ACLK on P2.0
//                    GPIO_TERNARY_MODULE_FUNCTION
//    );
//    GPIO_setAsPeripheralModuleFunctionOutputPin(
//                    GPIO_PORT_P3,
//                    GPIO_PIN4,                           // SMCLK on P3.4
//                    GPIO_TERNARY_MODULE_FUNCTION         // Could use Secondary or Tertiary mode
//    );
}


//*****************************************************************************
// Interrupt Service Routines
//*****************************************************************************
//#pragma vector=PORT1_VECTOR
//__interrupt void pushbutton_ISR (void)
//{
//    switch( __even_in_range( P1IV, P1IV_P1IFG7 )) {
//        case P1IV_NONE:   break;                               // None
//        case P1IV_P1IFG0:                                      // Pin 0
//             __no_operation();
//             break;
//       case P1IV_P1IFG1:                                       // Pin 1 (button 2)
//            GPIO_toggleOutputOnPin( GPIO_PORT_P1, GPIO_PIN0 );
//            break;
//       case P1IV_P1IFG2:                                       // Pin 2
//            __no_operation();
//            break;
//       case P1IV_P1IFG3:                                       // Pin 3
//            __no_operation();
//            break;
//       case P1IV_P1IFG4:                                       // Pin 4
//            __no_operation();
//            break;
//       case P1IV_P1IFG5:                                       // Pin 5
//            __no_operation();
//            break;
//       case P1IV_P1IFG6:                                       // Pin 6
//            __no_operation();
//            break;
//       case P1IV_P1IFG7:                                       // Pin 7
//            __no_operation();
//            break;
//       default:   _never_executed();
//    }
//}

//#pragma vector=PORT4_VECTOR
//__interrupt void pushbutton1_ISR (void)
//{
//    switch( __even_in_range( P4IV, P4IV_P4IFG7 )) {
//        case P4IV_NONE:   break;                               // None
//        case P4IV_P4IFG0:                                      // Pin 0
//             __no_operation();
//             break;
//       case P4IV_P4IFG1:                                       // Pin 1
//           __no_operation();
//            break;
//       case P4IV_P4IFG2:                                       // Pin 2
//            __no_operation();
//            break;
//       case P4IV_P4IFG3:                                       // Pin 3
//            __no_operation();
//            break;
//       case P4IV_P4IFG4:                                       // Pin 4
//            __no_operation();
//            break;
//       case P4IV_P4IFG5:                                       // Pin 5 (button 1)
//           GPIO_toggleOutputOnPin( GPIO_PORT_P4, GPIO_PIN6 );
//            break;
//       case P4IV_P4IFG6:                                       // Pin 6
//            __no_operation();
//            break;
//       case P4IV_P4IFG7:                                       // Pin 7
//            __no_operation();
//            break;
//       default:   _never_executed();
//    }
//}
