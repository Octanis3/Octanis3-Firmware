// ----------------------------------------------------------------------------
// mySPI.c  (for lab_06c_timerDirectDriveLed project) ('FR5969 Launchpad)
// ----------------------------------------------------------------------------

//***** Header Files **********************************************************
#include <driverlib.h>
#include "myClocks.h"
#include "mySPI.h"


#define SPI_FREQ		2000000

#define SPI_CLOCK_PERIOD 	1000000/SPI_FREQ

//***** Defines ***************************************************************


//***** Global Variables ******************************************************
uint8_t RXData = 0X12;
uint8_t TXData = 0xFF;

int spfr = SPI_CLOCK_PERIOD;
//uint8_t Slave_TXData = 0x01;
//uint8_t Slave_RXData = 0x00;

unsigned int counter_reader=0;
unsigned int counter_transmit=0;
uint8_t DataTransmit[100]={0x20, 0x20, 0x20};
uint8_t DataReceived[100]={0x20, 0x20, 0x20};
//*****************************************************************************
// Initialize Timer
//*****************************************************************************
void initSPI(void)
{
	//**************************************************************************
	    // SPI Initialization MASTER B0
	    //**************************************************************************


	    //Initialize Master B0
	    EUSCI_B_SPI_initMasterParam paramM = {0};
	    paramM.selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK;
	    paramM.clockSourceFrequency = CS_getSMCLK();
	    paramM.desiredSpiClock = SPI_FREQ; 					//SPI CLOCK is 500KHZ
	    paramM.msbFirst = EUSCI_B_SPI_MSB_FIRST;
	    paramM.clockPhase = EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT;		//UCCKPH=0
	    paramM.clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW;  //UCCKPL=0
	    paramM.spiMode = EUSCI_B_SPI_4PIN_UCxSTE_ACTIVE_LOW;
	    EUSCI_B_SPI_initMaster(EUSCI_B0_BASE, &paramM);

	    EUSCI_B_SPI_select4PinFunctionality (EUSCI_B0_BASE,EUSCI_B_SPI_ENABLE_SIGNAL_FOR_4WIRE_SLAVE); //enable chip select to control CR95HF

	    //Enable SPI module
	    EUSCI_B_SPI_enable(EUSCI_B0_BASE);

	    EUSCI_B_SPI_clearInterrupt(EUSCI_B0_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT);// | EUSCI_B_SPI_TRANSMIT_INTERRUPT);

	    // Disable USCI_B0 RX AND TX interrupts
	    EUSCI_B_SPI_disableInterrupt(EUSCI_B0_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT);// | EUSCI_B_SPI_TRANSMIT_INTERRUPT);
	    //
	    EUSCI_B_SPI_disableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);


	    //RESET byte
	    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,0x01);


	    //Wait for slave to initialize
//	    __delay_cycles(100);
	    __delay_cycles(40);

//	    Master_TXData = 0x00;                             // Holds TX data

	    //USCI_B0 TX buffer ready?
	   // while(!EUSCI_B_SPI_getInterruptStatus(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT)){;}

	    //Transmit Data to slave
//	    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE, Master_TXData);



}




//*****************************************************************************
// Interrupt Service Routines, Master SPI USCIB0
//*****************************************************************************

//#pragma vector=USCI_B0_VECTOR
//__interrupt void USCI_B0_ISR(void)

#pragma vector=USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)

{
    switch(__even_in_range(UCB0IV,4))
    {
    //Vector 2 - RXIFG
    case USCI_SPI_UCRXIFG:
    		if (counter_reader>0){

    			while(!EUSCI_B_SPI_getInterruptStatus(EUSCI_B0_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT)){;}

    			//Send next value
			EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,TXData);
    		}
		if(counter_reader==0) {UCB0IFG |= UCRXIFG;}

		while(!EUSCI_B_SPI_getInterruptStatus(EUSCI_B0_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT)){;}
		RXData = EUSCI_B_SPI_receiveData(EUSCI_B0_BASE);

		DataTransmit[counter_transmit]=TXData;
		counter_transmit++;

		DataReceived[counter_reader] = RXData;
		counter_reader++;

		EUSCI_B_SPI_disableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_RECEIVE_INTERRUPT);
		UCB0IFG |= UCRXIFG;

   /*     Master_RXData = EUSCI_B_SPI_receiveData(EUSCI_B0_BASE);
        DataReceived[counter_reader] = Master_RXData;
        counter_reader++;

	    EUSCI_B_SPI_disableInterrupt(EUSCI_B0_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT); //DISABLE INTERRUPT RECEIVE

        //Delay between transmissions for slave to process information
	    __delay_cycles(10*SPI_CLOCK_PERIOD*US_SECOND);*/
        break;

    case USCI_SPI_UCTXIFG:
/*        //USCI_B0 TX buffer ready? it checks if the Tx flag is set,  UCSCIB0 flag register (UCBTXIFG)
//        while(!EUSCI_B_SPI_getInterruptStatus(EUSCI_B0_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT)) {;}

        //UCB0TXBUF = Master_TXData;                   // Transmit characters

        EUSCI_B_SPI_transmitData(EUSCI_B0_BASE, Master_TXData);
        DataTransmit[counter_transmit]=Master_TXData;

        counter_transmit++;

	    EUSCI_B_SPI_disableInterrupt(EUSCI_B0_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT);// set low enable transmit bit

	    __delay_cycles(10*SPI_CLOCK_PERIOD*US_SECOND);*/


	   //Delay between transmissions for slave to process information
	 //  __delay_cycles(40);


        break;

    default: break;
    }
}


/*
switch(__even_in_range(UCA0IV, USCI_SPI_UCTXIFG))
  {
    case USCI_NONE: break;
    case USCI_SPI_UCRXIFG:
      RXData = UCA0RXBUF;
      UCA0IFG &= ~UCRXIFG;
      __bic_SR_register_on_exit(LPM0_bits); // Wake up to setup next TX
      break;
    case USCI_SPI_UCTXIFG:
      UCA0TXBUF = TXData;                   // Transmit characters
      UCA0IE &= ~UCTXIE;
      break;
    default: break;
  }
*/



