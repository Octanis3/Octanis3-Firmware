/*
 * load_cell.h
 *
 *  Created on: 03 Mar 2018
 *      Author: raffael
 */

#ifndef FW_LOAD_CELL_H_
#define FW_LOAD_CELL_H_

//#define USE_HX
#define USE_ADS

#include <xdc/cfg/global.h> //needed for semaphore
#include <ti/sysbios/knl/Semaphore.h>

// Semaphore_Handle semLoadCellDRDY;
int32_t get_last_stored_weight();
int32_t get_last_measured_tare();
int32_t get_weight_threshold();
void set_weight_threshold(int32_t new_th);
void load_cell_trigger_tare();
void load_cell_bypass_threshold(int status);

void load_cell_Task();

void load_cell_deep_sleep();

void load_cell_isr();

#endif /* FW_LOAD_CELL_H_ */
