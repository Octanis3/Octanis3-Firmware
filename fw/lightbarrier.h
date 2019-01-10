/*
 * lightbarrier.h
 *
 *  Created on: 27 Mar 2017
 *      Author: raffael
 */

#ifndef FW_LIGHTBARRIER_H_
#define FW_LIGHTBARRIER_H_

#if USE_LB

void lightBarrier_Task();

void lightbarrier_input_isr(unsigned int index);

#endif

#endif /* FW_LIGHTBARRIER_H_ */
