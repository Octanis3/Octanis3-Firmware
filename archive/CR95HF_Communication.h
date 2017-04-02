/*
 * StartCom.h
 * 1.-Power up the CR95HF
 * 2.-Program in the lowest state (Hibernat state)
 * 3.-IRQN is used to wake up the chip (Hibernate state to active state)
 * 4.-MCU (MSP430) Send command and receive data to/from the chip
 *
 *Developed by Jorge Zarate
 */

#include <driverlib.h>

//#include "mySPI.h"
//***** Prototypes ************************************************************
void CR95HFstartCommunication(void);
//***** Defines ***************************************************************


//extern uint8_t Master_RXData;


