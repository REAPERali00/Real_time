#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <unistd.h>
#include "../../des_controller/src/des.h"
/**********************
 * Function declaration
 **********************/
void scanInput(Person *person, char *input); /* Used to manage state transitions */

int main(int argc, char *argv[]) {

	pid_t cpid; /* controller pid storage var */
	int coid; /* return from connect attach  */
	Person person; /* Person structure defined in des.h */
	Display controller_response; /* response from the controller, struct defined in des.h */

	/* Check if correct number of command line arguments */
	if (argc != 2) {
		exit(EXIT_FAILURE);
	}

	cpid = atoi(argv[1]); /* get controllers pid from command line arguments */

	/* Connect to controller */
	if ((coid = ConnectAttach(ND_LOCAL_NODE, cpid, 1, _NTO_SIDE_CHANNEL, 0))
			== -1) { /* ON FAIL */
		exit(EXIT_FAILURE);
	}

	if (MsgSend(coid, &person, sizeof(person), &controller_response,
			sizeof(controller_response)) == -1) {
		exit(EXIT_FAILURE);
	}

	while (RUNNING) { /* Infinite Loop */
		char input[5]; /* no valid command is more then 4 chars,but will give more room... NOTE redeclared after every loop */
		printf(
				"Enter the event type (ls = left scan, rs = right scan, ws = weight scale, lo = left open, ro = right open, lc = left closed, rc = right closed, gru = guard right unlock, grl = guard right lock, gll = guard left lock, glu = guard left unlock, exit = exit programs) \n");

		scanf(" %s", input);

		scanInput(&person, input);

		/* PHASE II send message to controller */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			exit(EXIT_FAILURE);
		}

		/* Check if message is null ( null as in no length) */
//		if (sizeof(controller_response) == 0) {
//			exit(EXIT_FAILURE);
//		}

		if (person.state == STATE_EXIT)
			break;
	}
	sleep(5);
	/* Disconnect from server */
	ConnectDetach(coid);
	printf("Exit\n");
	return EXIT_SUCCESS;
}

int findString(char *input) {
	const char **p = inMessage;
	for (int i = 0; i < NUM_INPUTS; i++, p++) {
		if (strcmp(input, *p) == 0)
			return i;
	}
	return -1;
}

void scanInput(Person *output, char *input) {

	int input_Ind = findString(input);
	switch (input_Ind) {
	case INPUT_LEFT_SCAN:
		printf("enter The person's ID: \n");
		scanf("%d", &output->person_id);
		output->state = input_Ind;
		break;
	case INPUT_RIGHT_SCAN:
		printf("enter The person's ID: \n");
		scanf("%d", &(output->person_id));
		output->state = input_Ind;
		break;
	case INPUT_WEIGHT_SCALE:
		printf("enter The person's Weight: \n");
		scanf("%d", &(output->weight));
		output->state = input_Ind;
		break;
	case -1:
		printf("\n");
		break;
	default:
		output->state = input_Ind;
		break;
	}
}
