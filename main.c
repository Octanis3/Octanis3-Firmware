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

/* Board Header file */
#include "Board.h"

// heart beat task
#define HB_TASKSTACKSIZE   512
Task_Struct hb_task_Struct;
Char hb_task_Stack[HB_TASKSTACKSIZE];

// light barrier task
#define LB_TASKSTACKSIZE   512
Task_Struct lb_task_Struct;
Char lb_task_Stack[LB_TASKSTACKSIZE];

// RFID reader task
#define RFID_TASKSTACKSIZE   4096
Task_Struct rfid_task_Struct;
Char rfid_task_Stack[RFID_TASKSTACKSIZE];

// user button task
#define BUTTON_TASKSTACKSIZE   512
Task_Struct button_task_Struct;
Char button_task_Stack[BUTTON_TASKSTACKSIZE];

/*
 *  ======== heartBeatFxn ========
 *  Toggle the Board_led_green. The Task_sleep is determined by arg0 which
 *  is configured for the heartBeat Task instance.
 */
void heartBeat_Task(UArg arg0, UArg arg1)
{
    while (1) {
        Task_sleep((unsigned int)arg0);
//        GPIO_toggle(Board_led_blue);
    }
}

/*
 *  ======== main ========
 */
int main(void)
{
    Task_Params hb_taskParams;
    Task_Params lb_taskParams;
    Task_Params rfid_taskParams;
    Task_Params button_taskParams;


    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initSPI();
    Board_initUART();
    // Board_initWatchdog();

    /* Construct heartBeat Task  thread */
    Task_Params_init(&hb_taskParams);
    hb_taskParams.arg0 = 100;
    hb_taskParams.stackSize = HB_TASKSTACKSIZE;
    hb_taskParams.stack = &hb_task_Stack;
    Task_construct(&hb_task_Struct, (Task_FuncPtr)heartBeat_Task, &hb_taskParams, NULL);


    /* Construct ligthBarrier Task  thread */
	Task_Params_init(&lb_taskParams);
	lb_taskParams.stackSize = LB_TASKSTACKSIZE;
	lb_taskParams.stack = &lb_task_Stack;
	lb_taskParams.priority = 2;
	Task_construct(&lb_task_Struct, (Task_FuncPtr)lightBarrier_Task, &lb_taskParams, NULL);

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
	button_taskParams.priority = 1; // <--- MUST HAVE LOWER PRIORITY, OTHERWISE THE SPI POLLING MAY GET IT STUCK AND HANG OTHER TASKS.
	Task_construct(&button_task_Struct, (Task_FuncPtr)user_button_Task, &button_taskParams, NULL);


    /* Turn on user LED  */
    GPIO_write(Board_led_green, Board_LED_ON);

    /* Start BIOS */
    BIOS_start();

    return (0);
}
