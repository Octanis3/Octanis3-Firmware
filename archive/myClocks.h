/*
 * myClocks.h
 *
 */

#ifndef MYCLOCKS_H_
#define MYCLOCKS_H_

//***** Prototypes ************************************************************
void initClocks(void);

//***** Defines ***************************************************************
#define LF_CRYSTAL_FREQUENCY_IN_HZ     32768
#define HF_CRYSTAL_FREQUENCY_IN_HZ     0                                        // FR5969 Launchpad does not ship with HF Crystal populated

#define myMCLK_FREQUENCY_IN_HZ         8000000
#define mySMCLK_FREQUENCY_IN_HZ        8000000
#define myACLK_FREQUENCY_IN_HZ         32768

#define US_SECOND			8
#define MS_SECOND			8000		//This value is for MCLK at 8MHZ, verify if this frequency was defined


#endif /* MYCLOCKS_H_ */

