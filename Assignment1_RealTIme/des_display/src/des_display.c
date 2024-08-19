#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <errno.h>
#include "../../des_controller/src/des.h"

void display(Display*);
int main() {

	int chid, rcvid;
	Display display_message;

	if ((chid = ChannelCreate(0)) == -1) {
		perror("Failed to create channel");
		exit(EXIT_FAILURE);
	}
	printf("The display is running as PID: %d\n", getpid());

	while (1) {
		rcvid = MsgReceive(chid, &display_message, sizeof(display_message),
		NULL);
		if (rcvid == -1) {
			perror("Failed to receive message");
			exit(EXIT_FAILURE);
		}
		if (display_message.person.state != STATE_EXIT){
			display(&display_message);
			MsgReply(rcvid, EOK, NULL, 0);
		}
		if (display_message.person.state == STATE_EXIT) /* break infinite loop if Termination with Exit state */
			break;
	}

	ChannelDestroy(chid);
	printf("Exit Display\n");
	return 0;
}

void display(Display *output) {
	switch (output->output_type) {
	case OUTPUT_ID_SCAN:
		printf(outMessage[OUTPUT_ID_SCAN], output->person.person_id);
		printf("\n");
		break;
	case OUTPUT_WEIGHED:
		printf(outMessage[OUTPUT_WEIGHED], output->person.weight);
		printf("\n");
		break;
	default:
		printf(outMessage[output->output_type]);
		printf("\n");
		break;
	}
}
