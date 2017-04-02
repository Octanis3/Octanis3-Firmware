/*
 * rfid_reader.c
 *
 *  Created on: 27 Mar 2017
 *      Author: raffael
 */

#include "rfid_reader.h"

void rfid_Task()
{
	/* Initialize SPI structures*/


	/* Initialize ST95HF as reader by default*/


    while (1) {
    		/* Put ST95HF to deep sleep/low power mode */

    		/* Pend on reader semaphore (can be released either by light barrier interrupt,
    		 * in which case a read has to be performed, or by the user button, in which
    		 * case the ST95HF has to be switched into tag mode.
    		 */

    		// Semaphore pend ....

    		/* If barrier triggered, wake up and issue a read command */
		{
			//wake ST95HF

			//send read command

			//receive the ID

			//store ID in flash memory (--> send message to mailbox to be treated by
			// 								int. flash memory task)


		}
    		/* else if user button triggered, wake up/switch ST95HF into tag mode and
    		 * start to listen for incoming NFC read commands.
    		 */
		{
			//(wake ST95HF ?? if needed ??)

			//re-initialize ST95HF in tag-mode

			//wait for incoming NFC reads

				//read received command

				//if command == "send nestbox device ID" --> reply with ID.
					// continue waiting for incoming commands

				//if command == "send stored data" --> successively send the stored data
					// continue waiting for incoming commands

				//if command == "delete stored data" --> delete it
					// continue waiting for incoming commands

				//if command == "end communication" --> re-initialize ST95HF in reader mode
					// exit loop

			// if timeout (no incoming reads for 1 minute) --> re-initialize ST95HF in reader mode
				// exit loop
		}

		// TODO remove this:
    		Task_sleep(100);
    }
}

