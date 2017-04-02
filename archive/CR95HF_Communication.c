/*
 * StartCom.c
 * 1.-Power up the CR95HF
 * 2.-Program in the lowest state (Hibernat state)
 * 3.-IRQN is used to wake up the chip (Hibernate state to active state)
 * 4.-MCU (MSP430) Send command and receive data to/from the chip
 *
 * Remarques:
 * 1.-Look if is possible to turn off the chip instead to set it to Hibernate state,
 * in order to have lower power consomption.
 * 2.-Evaluate if SPI communication is the best choice (Pins needed for communication:
 * IRQN, SSI_0, SSI_1, SPI_SS,SPI_MISO,SPI_MOSI,SPI_SCK).
 * May be UART comunicaiton is a better choice if there are not enough disponible pins
 * (Pins needed, UART_RX / IRQ_IN, UART_TX / IRQ_OUT, SSI_0, SSI_1, only for). The choice of SPI protocol
 * was argumented in the developper experience with this protocol.
 * 3.-Optimiser _delay_cycles() using a timer
 *
 *
 *Developed by Jorge Zarate
 */

//***** Header Files **********************************************************
#include <driverlib.h>
#include "myClocks.h"
#include "CR95HF_Communication.h"


//***** Defines ***************************************************************
// See additional #defines in 'CR95HF_Communication.h'
#define SELECT_SERIAL		SPI
//#define SelectSerial		UART
#define POLLING_MASK			0x08
#define SEND					0X00
#define POLL					0X03
#define READ					0X02
#define RESET				0X01

#define SEND_RECEIVE			0X04
#define PROTOCOL_SELECT		0X02
#define ECHO					0X55

#define SSI_0 				GPIO_PORT_P3,GPIO_PIN5
#define	IRQ_IN				GPIO_PORT_P1,GPIO_PIN4
#define 	VSS					GPIO_PORT_P3,GPIO_PIN4
#define SPI_SS				GPIO_PORT_P1, GPIO_PIN3

const uint8_t HibernateState[] = {0x07,0x0E,0x08,0x04,0x00,0x04,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

extern uint8_t TXData;
extern uint8_t RXData;
uint8_t aux=0x00;

//***** Global Variables ******************************************************


//***** Prototypes ************************************************************
//It is assumed that CR95HF is not powered before call this focntion.
//When the chip is power on (VDS HIGH), the chip goes to POWER-UP state automatically
//This function wake up the CR95HF and set it to READY state from POWER-UP state (power-up sequence of CR95HF reference datasheet 3.2).
//The protocol is selected at the same time
//if Cr95hf was turned off, an delay of 10ms (t4 datasheet) has to be added
//at the end of the function the CR95HF is in Power-up state (datasheet)
void CR95HF_enterReadyState();

// Set protocol and its parameters, in this project is used ISO 15963 a 26Kbps with CRC appended.  Reference DATASHEET 5.4
void CR95HF_setProtocol();

//CR95HF polling mode. After Sending a command this function allow to asking to CR95HF
//if it is ready to send ininformation to MCU (datasheet 4.2.1).
//Use this fonction before to a Receiveing command
void CR95HF_polling();

//************************************************************************************************************************
void CR95HF_cmdSendReceiveDataWithTag(uint8_t *Data);
void CR95HF_readResponse(uint8_t *Data);
void CR95HF_cmdGoHibernateState();
void CR95HF_cmdGoActivateState();
void CR95HF_sendEcho();
void CR95HF_powerOff();
void CR95HF_Reset();


//***** main function************************************************************
void CR95HFstartCommunication(void){

	CR95HF_enterReadyState();

	CR95HF_Reset();

	CR95HF_enterReadyState();

	CR95HF_sendEcho();

	CR95HF_powerOff();

//	CR95HF_setProtocol();
//	uint8_t dataToSend[] = {ECHO};
//	CR95HF_cmdSendReceiveDataWithTag(dataToSend); //0x01 inventory request

//	uint8_t dataReceived [20];
//	CR95HF_readResponse(dataReceived);

//	CR95HF_cmdGoHibernateState();

//	CR95HF_cmdGoActivateState();
//	CR95HF_enterReadyState();
}

//***** Functions ************************************************************//


void CR95HF_enterReadyState(){
//The mode serial communication is selected, SPI or UART. This function has to be called before EnterReadyState()
//SPI: SSI_0 = 1; SSI_0=0; UART: SSI_0 = 0; SSI_0=0.Add code to select UART communication if it is needed.

#if	SELECT_SERIAL == SPI
	GPIO_setOutputHighOnPin(SSI_0);		//SSI_0=1;
//    GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN5);		//SSI_1=0; it is done dutomatically by the x-nucle-NFC03A1
#endif

	GPIO_setOutputHighOnPin(IRQ_IN);		//  IRQ_IN HIGH just before power on

	//powerOn CR95HF
	GPIO_setOutputHighOnPin(VSS); //VSS


	_delay_cycles(10U*MS_SECOND);    // 10ms minimum, t4 datasheet (time to power up CR995HF)

//    GPIO_setOutputHighOnPin(GPIO_PORT_P1,GPIO_PIN4);		//  IRQ_IN high
    _delay_cycles(100*US_SECOND);  //100us min, t0 in datasheet
    GPIO_setOutputLowOnPin(IRQ_IN);		//  IRQ_IN low
    _delay_cycles(20*US_SECOND);  //10us minumum, t1 in datasheet
    GPIO_setOutputHighOnPin(IRQ_IN);		//  IRQ_IN high
    _delay_cycles(10U*MS_SECOND);  //ready in <10ms, t3 in datasheet
}

//CR95HF_Reset(){
//    GPIO_setOutputLowOnPin(SPI_SS);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE



//}


// Reset the chip
void CR95HF_Reset(){
    GPIO_setOutputLowOnPin(SPI_SS);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE

    __delay_cycles(US_SECOND);

//    TXData=RESET;

   UCB0IE |= UCRXIE; //EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    __delay_cycles(20);// delay to wait the interruption

    GPIO_setOutputHighOnPin(SPI_SS);	// UCB0STE/SPI_SS=1 (slave select), LOW ACTIVE

    __delay_cycles(MS_SECOND);

}


// Send echo command, to test communication
void CR95HF_sendEcho(){

    GPIO_setOutputLowOnPin(SPI_SS);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE
    __delay_cycles(US_SECOND);

    TXData=SEND;

    UCB0IE |= UCRXIE; //EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

   __delay_cycles(10);

    TXData=ECHO;
    UCB0IE |= UCRXIE;//EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    __delay_cycles(10);

//    Master_TXData=0x00;
//    UCB0IE |= UCTXIE + UCRXIE;//EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);
//    __delay_cycles(10);


    GPIO_setOutputHighOnPin(SPI_SS);	// UCB0STE/SPI_SS=1 (slave select), LOW ACTIVE


    __delay_cycles(MS_SECOND);

//    ***********************************************************polling//
    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE
    __delay_cycles(US_SECOND);

    while(1){
    TXData=POLL;
    UCB0IE |= UCRXIE ;//EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    __delay_cycles(10);

	aux=RXData;
	aux=aux&POLLING_MASK;
	if (aux==0x08)break;			//polling until BIT4 flag is high, that means the chip is ready to send data
	}

	GPIO_setOutputHighOnPin( GPIO_PORT_P1, GPIO_PIN3);

//    EUSCI_B_SPI_clearInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_RECEIVE_INTERRUPT);

    __delay_cycles(MS_SECOND);


//    ***********************************************************RECEIVE ECHO//

    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE

    __delay_cycles(US_SECOND);

    TXData=READ;
//    UCB0IFG |= UCTXIFG;
    UCB0IE |= UCRXIE; //EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    __delay_cycles(10);


//    ***********************************************************RECEIVE ECHO//

    TXData=READ;

    UCB0IE |= UCRXIE; //EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    __delay_cycles(10);


//    **********************************************

    TXData=READ;

    UCB0IE |= UCRXIE; //EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    __delay_cycles(10);


//    **********************************************
    TXData=READ;

    UCB0IE |= UCRXIE; //EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    __delay_cycles(10);

//    **********************************************
    TXData=READ;

    UCB0IE |= UCRXIE; //EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    __delay_cycles(10);

//    **********************************************
    TXData=READ;

    UCB0IE |= UCRXIE; //EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    __delay_cycles(10);

//    **********************************************
    TXData=READ;

    UCB0IE |= UCRXIE; //EUSCI_B_SPI_enableInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    __delay_cycles(10);


//    **********************************************


    GPIO_setOutputHighOnPin( GPIO_PORT_P1, GPIO_PIN3);	// UCB0STE/SPI_SS=1 (slave select), LOW ACTIVE


//    EUSCI_B_SPI_clearInterrupt(EUSCI_B0_BASE,EUSCI_B_SPI_RECEIVE_INTERRUPT);

    __delay_cycles(MS_SECOND);

 /*
	GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE
	EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,POLL);			//control byte -> Polling mode (0x03)
	while (1){
		aux=EUSCI_B_SPI_receiveData(EUSCI_B0_BASE);
		aux=aux&POLLING_MASK;
		if (aux==0x01)break;			//polling until BIT4 flag is high, that means the chip is ready to send data
	}
	GPIO_setOutputHighOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=1 (slave select), LOW ACTIVE;

*/


/*
//    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,SEND);			//control byte -> send/write command, (0x00)
    Master_TXData=SEND;

    UCB0IFG |= UCTXIFG;
//    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,ECHO);			//set ECHO command, (0x55)
    Master_TXData=ECHO;
    UCB0IFG |= UCTXIFG;
*/

}

void CR95HF_powerOff(){

	//powerOFF CR95HF
	GPIO_setOutputLowOnPin(VSS); //VSS

	GPIO_setOutputLowOnPin(IRQ_IN);		//


	_delay_cycles(10U*MS_SECOND);    // 10ms minimum, t4 datasheet (time to power up CR995HF)

}
/*
// Set protocol and its parameters, in this project is used ISO 15963 a 26Kbps with CRC appended.  Reference DATASHEET 5.4
void CR95HF_setProtocol(){
    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,SEND);			//control byte -> send/write command, (0x00)
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,PROTOCOL_SELECT);			//set protocol command, (0x02)
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,0x02);			//length of data sent, (0x02)
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,0x01);    		//set ISO 15693, (0x01)
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,0x01);			//set  26 Kbps transmission and reception rates, CRC appended (evaluate if it is necessary to do a CRC)
    GPIO_setOutputHighOnPin( GPIO_PORT_P1, GPIO_PIN3);	// UCB0STE/SPI_SS=1 (slave select), LOW ACTIVE
}

//Function send command SendReceive data to Chip, in this way the chip send and receive data with the TAG (datasheet 5.5)
void CR95HF_cmdSendReceiveDataWithTag(uint8_t *Data){
	int i=0;
    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,SEND);			//control byte -> send/write command, (0x00)
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,SEND_RECEIVE);			//set sendReceive command, (0x02)
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,sizeof(Data));			//send the data's length

    for (i=0;i<sizeof(Data);i++) {								//send data
    	EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,*(Data+i)) ;
    }

    GPIO_setOutputHighOnPin( GPIO_PORT_P1, GPIO_PIN3);	// UCB0STE/SPI_SS=1 (slave select), LOW ACTIVE
}

//CR95HF polling mode. After Sending a command this function allow to asking to CR95HF
//if it is ready to send ininformation to MCU (datasheet 4.2.1).
//Use this fonction before to a Receiveing command
void CR95HF_polling(){
	uint8_t aux=0x01;
    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,POLL);			//control byte -> Polling mode (0x03)
	while (1){
		aux=EUSCI_B_SPI_receiveData(EUSCI_B0_BASE);
		aux=aux&POLLING_MASK;
		if (aux==0x01)break;			//polling until BIT4 flag is high, that means the chip is ready to send data
	}
	GPIO_setOutputHighOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=1 (slave select), LOW ACTIVE;
}

//CR95HF Receiving mode. The data is stocked in the Array Data (*Data call the array by reference)
void CR95HF_readResponse(uint8_t *Data){
	int i=0;
    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,READ);			//control byte -> Receiving mode (0x02)
    //Add code in order to Make a table with the data received
    for (i=0;i<sizeof(Data);i++) {
    *(Data+i)=EUSCI_B_SPI_receiveData(EUSCI_B0_BASE);
    }
    GPIO_setOutputHighOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=1 (slave select), LOW ACTIVE
}
//************************************************************************************************************************

void CR95HF_cmdGoHibernateState(){
	int i=0;
    GPIO_setOutputLowOnPin( GPIO_PORT_P1, GPIO_PIN3);		// UCB0STE/SPI_SS=0 (slave select), LOW ACTIVE
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,SEND);			//control byte -> send/write command, (0x00)
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,sizeof(HibernateState));			//send command

    for (i=0;i<sizeof(HibernateState);i++) {								//send data
    	EUSCI_B_SPI_transmitData(EUSCI_B0_BASE,*(HibernateState+i)) ;
    }

    GPIO_setOutputHighOnPin( GPIO_PORT_P1, GPIO_PIN3);	// UCB0STE/SPI_SS=1 (slave select), LOW ACTIVE
}
*/
