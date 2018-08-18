/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== main.c ========
 */

/* Task Header files */
#include "fw/lightbarrier.h"
#include "fw/rfid_reader.h"
#include "fw/user_button.h"
#include "fw/logger.h"
#include "fw/load_cell.h"
#include "fw/battery_monitor.h"
#include "fw/PIR_wakeup.h"

/* Board Header file */
#include "Board.h"

//to set the CPU frequency.
#include <ti/sysbios/knl/Clock.h>

#ifdef LIGHTBARRIER_VERSION
// light barrier task
#define LB_TASKSTACKSIZE   512
Task_Struct lb_task_Struct;
Char lb_task_Stack[LB_TASKSTACKSIZE];
#endif

// RFID reader task
#define RFID_TASKSTACKSIZE   2048
Task_Struct rfid_task_Struct;
Char rfid_task_Stack[RFID_TASKSTACKSIZE];

// user button task
#define BUTTON_TASKSTACKSIZE   256
Task_Struct button_task_Struct;
Char button_task_Stack[BUTTON_TASKSTACKSIZE];

// log task
#define LOG_TASKSTACKSIZE   512
Task_Struct log_task_Struct;
Char log_task_Stack[LOG_TASKSTACKSIZE];

// load cell task
#define LOAD_CELL_TASKSTACKSIZE   1024
Task_Struct load_cell_task_Struct;
Char load_cell_task_Stack[LOAD_CELL_TASKSTACKSIZE];

// battery task
#define BATTERY_TASKSTACKSIZE   512
Task_Struct bat_task_Struct;
Char bat_task_Stack[BATTERY_TASKSTACKSIZE];

// PIR wakeup task
#define PIR_WAKEUP_TASKSTACKSIZE   512
Task_Struct PIR_wakeup_task_Struct;
Char PIR_wakeup_task_Stack[PIR_WAKEUP_TASKSTACKSIZE];

/*
 *  ======== main ========
 */
int main(void)
{
#ifdef LIGHTBARRIER_VERSION
    Task_Params lb_taskParams;
#endif
    Task_Params rfid_taskParams;
    Task_Params button_taskParams;
    Task_Params log_taskParams;
    Task_Params load_cell_taskParams;
    Task_Params bat_taskParams;
    Task_Params PIR_wakeup_taskParams;

    // disable interrupts if an interrupt could lead to
	// another call to Clock_tickReconfig or if interrupt
	// processing relies on having a running timer
	//xdc_runtime_Types_FreqHz clock_frequency;   /* Timestamp frequency */
	//clock_frequency.lo = 16000000;

//	Hwi_disable() or Swi_disable();
//	BIOS_setCpuFreq(&clock_frequency);
//	Clock_tickReconfig(); //need to call Clock_tickStop() and Clock_tickStart if this is called before BIOS_start()!!
	//	Hwi_restore() or Swi_enable()


    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initSPI();
    Board_initUART();
    // Board_initWatchdog();

#ifdef LIGHTBARRIER_VERSION
    /* Construct ligthBarrier Task  thread */
	Task_Params_init(&lb_taskParams);
	lb_taskParams.stackSize = LB_TASKSTACKSIZE;
	lb_taskParams.stack = &lb_task_Stack;
	lb_taskParams.priority = 2;
	Task_construct(&lb_task_Struct, (Task_FuncPtr)lightBarrier_Task, &lb_taskParams, NULL);
#endif

	/* Construct rfidReader Task  thread */
	Task_Params_init(&rfid_taskParams);
	rfid_taskParams.stackSize = RFID_TASKSTACKSIZE;
	rfid_taskParams.stack = &rfid_task_Stack;
	rfid_taskParams.priority = 1; // <--- MUST HAVE LOWER PRIORITY, OTHERWISE THE SPI POLLING MAY GET IT STUCK AND HANG OTHER TASKS.
	Task_construct(&rfid_task_Struct, (Task_FuncPtr)rfid_Task, &rfid_taskParams, NULL);

	/* Construct userButton Task  thread */
	Task_Params_init(&button_taskParams);
	button_taskParams.stackSize = BUTTON_TASKSTACKSIZE;
	button_taskParams.stack = &button_task_Stack;
	button_taskParams.priority = 3;
	Task_construct(&button_task_Struct, (Task_FuncPtr)user_button_Task, &button_taskParams, NULL);

	/* Construct logging Task  thread */
	Task_Params_init(&log_taskParams);
	log_taskParams.stackSize = LOG_TASKSTACKSIZE;
	log_taskParams.stack = &log_task_Stack;
	log_taskParams.priority = 4;
	Task_construct(&log_task_Struct, (Task_FuncPtr)log_Task, &log_taskParams, NULL);

	/* Construct load cell Task  thread */
	Task_Params_init(&load_cell_taskParams);
	load_cell_taskParams.stackSize = LOAD_CELL_TASKSTACKSIZE;
	load_cell_taskParams.stack = &load_cell_task_Stack;
	load_cell_taskParams.priority = 2; //
	Task_construct(&load_cell_task_Struct, (Task_FuncPtr)load_cell_Task, &load_cell_taskParams, NULL);

	/* Construct battery monitoring Task  thread */
	Task_Params_init(&bat_taskParams);
	log_taskParams.stackSize = BATTERY_TASKSTACKSIZE;
	log_taskParams.stack = &bat_task_Stack;
	log_taskParams.priority = 5; //most important task, but with low duty cycle
	Task_construct(&bat_task_Struct, (Task_FuncPtr)battery_Task, &bat_taskParams, NULL);

	/* Construct PIR_wakeup Task  thread */
    Task_Params_init(&PIR_wakeup_taskParams);
    PIR_wakeup_taskParams.stackSize = PIR_WAKEUP_TASKSTACKSIZE;
    PIR_wakeup_taskParams.stack = &PIR_wakeup_task_Stack;
    PIR_wakeup_taskParams.priority = 5; //
    Task_construct(&PIR_wakeup_task_Struct, (Task_FuncPtr)PIR_wakeup_Task, &PIR_wakeup_taskParams, NULL);

    /* Turn on user LED  */
    GPIO_write(Board_led_green, Board_LED_ON);

    /* Start BIOS */
    BIOS_start();

    return (0);
}
