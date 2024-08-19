#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <errno.h>
#include "des.h"

/****************************************
 * Function Declaration
 ****************************************/
void* st_ls(); /*	 LEFT SCAN 			*/
void* st_lo(); /*  LEFT OPEN  		*/
void* st_lc(); /*  LEFT CLOSE 		*/
void* st_rs(); /*  RIGHT SCAN 		*/
void* st_ro(); /*  RIGHT OPEN 		*/
void* st_rc(); /*  RIGHT CLOSE 		*/
void* st_grl(); /*  GUARD RIGHT LOCK 	*/
void* st_gru(); /*  GUARD RIGHT UNLOCK */
void* st_gll(); /*  GUARD LEFT LOCK 	*/
void* st_glu(); /*  GUARD LEFT UNLOCK	*/
void* st_ws(); /*  WEIGHT SCALE 		*/
void* st_start();/*  Start 				*/
void* st_exit(); /*  EXIT 				*/

void reset();

Display controller_response;
Person person;
int coid, chid, rcvid;
FState f_state = st_start;
int lrstate = DEFAULT;

int main(int argc, char *argv[]) {

	pid_t dpid; /* display pid */

	if (argc != 2) {/* Validate correct amount of arguments */
		printf("Invalid argument amount\n");
		exit(EXIT_FAILURE);
	}

	dpid = atoi(argv[1]); /* Get display pid from cmd line arguments and convert to int */

	/* PHASE I: Create Channel */
	if ((chid = ChannelCreate(0)) == -1) {
		printf("unable to create channel");
		exit(EXIT_FAILURE);
	}

	/* PHASE I Connect to display, controller between client (inputs) and display server */
	if ((coid = ConnectAttach(ND_LOCAL_NODE, dpid, 1, _NTO_SIDE_CHANNEL, 0))
			== -1) {
		printf("could not connect to display");
		exit(EXIT_FAILURE);
	}

	printf("Controller PID: %d\n", getpid()); /* display pid for client */
	reset();/* reset structure to make sure no corrupted data */

	while (1) {
		/* PHASE II */
		if ((rcvid = MsgReceive(chid, &person, sizeof(person), NULL)) == -1) { /* receive message from client */
			printf("Message receive error\n"); /* ON FAIL */
			exit(EXIT_FAILURE);
		}
		controller_response.person = person;

		if (person.state == STATE_EXIT) /* if state is exit then move to exit state from any state, avoids adding exit code to every state */
			f_state = (*st_exit)();
		else
			f_state = (FState) (*f_state)(); /* else follow states through function pointers */

		/* Reply to client  if state is not exit*/
		if (person.state != STATE_EXIT) /* No need to reply on program termination */
			MsgReply(rcvid, EOK, &controller_response,
					sizeof(controller_response));

		if (person.state == STATE_EXIT) /* break infinite loop if Termination with Exit state */
			break;

	}
	ConnectDetach(coid); /* Detach from display */
	ChannelDestroy(chid);/* destroy channel */
	printf("Exiting Controller\n");
	return EXIT_SUCCESS;
}

void* st_start() { /* START STATE  move to idling state, message has been received from inputs*/
	if (lrstate == DEFAULT && person.state == STATE_RIGHT_SCAN) { /* GRl is on idle waiting for person to scan right */
		controller_response.output_type = OUTPUT_ID_SCAN;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("RS - st_grl()\n");
			exit(EXIT_FAILURE);
		}
		lrstate = RIGHT;
		return st_rs;
	}
	if (lrstate == DEFAULT && person.state == STATE_LEFT_SCAN) {/* GRL is on idle waiting for person to scan left */
		controller_response.output_type = OUTPUT_ID_SCAN;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("LS - st_grl()\n");
			exit(EXIT_FAILURE);
		}
		lrstate = LEFT;
		return st_ls;
	}
	return st_start; /* Return the default IDLE state, waiting for a person to scan. */
}

void* st_ls() { /* LEFT SCAN */
	if (person.state == STATE_GUARD_LEFT_UNLOCK) {
		controller_response.output_type = OUTPUT_LEFT_UNLOCKED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("GLU - st_ls()\n");
			exit(EXIT_FAILURE);
		}
		return st_glu; /* return next state, if scan or input were successful */
	}
	return st_ls;
}

void* st_glu() { /*  GUARD LEFT UNLOCK	*/
	if (person.state == STATE_LEFT_OPEN) {
		controller_response.output_type = OUTPUT_LEFT_OPENED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("LO - st_glu()\n");
			exit(EXIT_FAILURE);
		}
		return st_lo; /* return next state, if successful */
	}
	return st_glu;
}

void* st_lo() { /*  LEFT OPEN  		*/
	if (person.state == STATE_WEIGHT_SCALE) { /* IF person entered from the left then they need to weigh in*/
		controller_response.output_type = OUTPUT_WEIGHED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("WS -st_lo()\n");
			exit(EXIT_FAILURE);
		}
		return st_ws;
	} else if (person.state == STATE_LEFT_CLOSED) { /* IF person entered from the right, state after LO is LC*/
		controller_response.output_type = OUTPUT_LEFT_CLOSED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("LC - st_lo()\n");
			exit(EXIT_FAILURE);
		}
		return st_lc;
	}
	return st_lo;
}

void* st_rs() { /* RIGHT SCAN */
	if (person.state == STATE_GUARD_RIGHT_UNLOCK) { /* Person enter by the right door */
		controller_response.output_type = OUTPUT_RIGHT_UNLOCKED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("GRU - st_rs()\n");
			exit(EXIT_FAILURE);
		}
		return st_gru;
	}
	return st_rs;
}

void* st_gru() { /*  GUARD RIGHT UNLOCK */
	if (person.state == STATE_RIGHT_OPEN) { /* let person entire the right door */
		controller_response.output_type = OUTPUT_RIGHT_OPENED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("RO - st_gru()\n");
			exit(EXIT_FAILURE);
		}
		return st_ro;
	}
	return st_gru;
}

void* st_ro() { /*  RIGHT OPEN 		*/
	if (person.state == STATE_WEIGHT_SCALE) { /* IF weighing, then person entered from the right*/
		controller_response.output_type = OUTPUT_WEIGHED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("WS - st_ro()\n");
			exit(EXIT_FAILURE);
		}
		return st_ws;
	} else if (person.state == STATE_RIGHT_CLOSED) { /* if right closed then person entered from the left */
		controller_response.output_type = OUTPUT_RIGHT_CLOSED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("RC- st_ro()\n");
			exit(EXIT_FAILURE);
		}
		return st_rc;
	}
	return st_ro;
}

void* st_ws() { /*  WEIGHT SCALE 		*/
	if (person.state == STATE_LEFT_CLOSED) { /* Person entered from the left */
		controller_response.output_type = OUTPUT_LEFT_CLOSED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("LC - st_ws()\n");
			exit(EXIT_FAILURE);
		}
		return st_lc;
	}
	if (person.state == STATE_RIGHT_CLOSED) { /* person entered from the right */
		controller_response.output_type = OUTPUT_RIGHT_CLOSED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("RC - st_ws()\n");
			exit(EXIT_FAILURE);
		}
		return st_rc;
	}
	return st_ws;
}

void* st_lc() { /*  LEFT CLOSE 		*/
	if (person.state == STATE_GUARD_LEFT_LOCK) { /* person has left the left doors */
		controller_response.output_type = OUTPUT_LEFT_LOCKED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("GLL - st_lc()\n");
			exit(EXIT_FAILURE);
		}
		return st_gll;
	}
	return st_lc;
}

void* st_rc() { /*  RIGHT CLOSE 		*/
	if (person.state == STATE_GUARD_RIGHT_LOCK) { /* person has left the right doors */
		controller_response.output_type = OUTPUT_RIGHT_LOCKED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("GRL - st_rc() \n");
			exit(EXIT_FAILURE);
		}
		return st_grl;
	}
	return st_rc;
}

void* st_grl() { /*  GUARD RIGHT LOCK 	*/
	if (lrstate == RIGHT && person.state == STATE_GUARD_LEFT_UNLOCK) { /* person entered from the right hand side and must exit the left */
		controller_response.output_type = OUTPUT_LEFT_UNLOCKED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("GLU - st_grl()\n");
			exit(EXIT_FAILURE);
		}
		lrstate = DEFAULT;
		return st_glu;
	}
	return st_grl;
}

void* st_gll() { /*  GUARD LEFT LOCK 	*/
	if (lrstate == LEFT && person.state == STATE_GUARD_RIGHT_UNLOCK) { /* person has entered from the left an must now exit from the right */
		controller_response.output_type = OUTPUT_RIGHT_UNLOCKED;
		if (MsgSend(coid, &controller_response, sizeof(controller_response),
				NULL, 0) == -1) {
			printf("GRU - st_gll()\n");
			exit(EXIT_FAILURE);
		}
		lrstate = DEFAULT;
		return st_gru;
	}
	return st_gll;
}

void* st_exit() { /*  EXIT */
	if (MsgSend(coid, &controller_response, sizeof(controller_response), NULL,
			0) == -1) {
		printf("EXIT - st_exit()\n");
		exit(EXIT_FAILURE);
	}
	return st_exit;
}

void reset() {
	person.person_id = 0;
	person.weight = 0;
	person.state = STATE_START;
}
