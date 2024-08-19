#include "metronome.h"

name_attach_t *attach;
metronome_settings_t Metronome;
int server_coid;
char data[255];

int main(int argc, char *argv[]) {
	dispatch_t *dpp;
	resmgr_io_funcs_t io_funcs;
	resmgr_connect_funcs_t conn_funcs;
	ioattr_t ioattr[DEVICES];
	pthread_attr_t thread_attrib;
	dispatch_context_t *ctp;

	int id, i;
	int server_coid;

	if (argc != 4) {
		printf("ERROR: You must provide all required arguments.\n");
		printUsage();
		exit(EXIT_FAILURE);
	}

	iofunc_funcs_t metro_ocb_funcs = {
	_IOFUNC_NFUNCS, metro_ocb_calloc, metro_ocb_free, };

	iofunc_mount_t metro_mount = { 0, 0, 0, 0, &metro_ocb_funcs };

	Metronome.m_props.beatsPerMin = atoi(argv[1]);
	Metronome.m_props.timeSigTop = atoi(argv[2]);
	Metronome.m_props.timeSigBot = atoi(argv[3]);
	if (search(&Metronome) == ERROR) {
		printf("Metronome Settings entry is not valid! \n");
		return (EXIT_FAILURE);
	}

	if ((dpp = dispatch_create()) == NULL) {
		fprintf(stderr, "%s:  Unable to allocate dispatch context.\n", argv[0]);
		return (EXIT_FAILURE);
	}

	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &conn_funcs, _RESMGR_IO_NFUNCS,
			&io_funcs);
	conn_funcs.open = io_open;
	io_funcs.read = io_read;
	io_funcs.write = io_write;

	for (i = 0; i < DEVICES; i++) {
		iofunc_attr_init(&ioattr[i].attr, S_IFCHR | 0666, NULL, NULL);
		ioattr[i].device = i;
		ioattr[i].attr.mount = &metro_mount;
		if ((id = resmgr_attach(dpp, NULL, devnames[i], _FTYPE_ANY, 0,
				&conn_funcs, &io_funcs, &ioattr[i])) == ERROR) {
			fprintf(stderr, "%s:  Unable to attach name.\n", argv[0]);
			return (EXIT_FAILURE);
		}
	}

	ctp = dispatch_context_alloc(dpp);

	pthread_attr_init(&thread_attrib);
	pthread_create(NULL, &thread_attrib, &metronome_thread, &Metronome);

	while (RUNNING) {
		if ((ctp = dispatch_block(ctp))) {
			dispatch_handler(ctp);
		} else
			printf("ERROR \n");
	}

	pthread_attr_destroy(&thread_attrib);
	name_detach(attach, 0);
	name_close(server_coid);
	return EXIT_SUCCESS;
}

void printUsage() {
	printf(
			"Command: ./metronome <beats/minute> <time-signature-top> <time-signature-bottom>\n");
}

int io_read(resmgr_context_t *ctp, io_read_t *msg, metro_ocb_t *mocb) {
	int nb;

	if (data == NULL)
		return 0;

	if (mocb->ocb.attr->device == METRONOME_HELP) {
		sprintf(data,
				"Metronome Resource Manager (Resmgr)\n\nUsage: metronome <bpm> <ts-top> <ts-bottom>\n\nAPI:\n pause[1-9]\t\t\t-pause the metronome for 1-9 seconds\n quit:\t\t\t\t- quit the metronome\n set <bpm> <ts-top> <ts-bottom>\t- set the metronome to <bpm> ts-top/ts-bottom\n start\t\t\t\t- start the metronome from stopped state\n stop\t\t\t\t- stop the metronome; use 'start' to resume\n");
	} else {
		sprintf(data,
				"[metronome: %d beats/min, time signature: %d/%d, sec-per-interval: %.2f, nanoSecs: %.0lf]\n",
				Metronome.m_props.beatsPerMin, Metronome.m_props.timeSigTop,
				Metronome.m_props.timeSigBot, Metronome.t_props.interval,
				Metronome.t_props.nano_sec);
	}
	nb = strlen(data);

	if (mocb->ocb.offset == nb)
		return 0;

	nb = min(nb, msg->i.nbytes);
	_IO_SET_READ_NBYTES(ctp, nb);
	SETIOV(ctp->iov, data, nb);
	mocb->ocb.offset += nb;

	if (nb > 0)
		mocb->ocb.flags |= IOFUNC_ATTR_ATIME;

	return (_RESMGR_NPARTS(1));
}

int io_write(resmgr_context_t *ctp, io_write_t *msg, metro_ocb_t *mocb) {
	int nb = 0;

	if (mocb->ocb.attr->device == METRONOME_HELP) {
		printf("\nError: Cannot Write to device /dev/local/metronome-help\n");
		nb = msg->i.nbytes;
		_IO_SET_WRITE_NBYTES(ctp, nb);
		return (_RESMGR_NPARTS(0));
	}

	if (msg->i.nbytes == ctp->info.msglen - (ctp->offset + sizeof(*msg))) {
		char *buf;
		char *pause_msg;
		char *set_msg;
		int i, small_integer = 0;
		buf = (char*) (msg + 1);

		int userIndex = userInput(buf);
		switch (userIndex) {
		case PAUSE:
			for (i = 0; i < 2; i++) {
				pause_msg = strsep(&buf, " ");
			}
			small_integer = atoi(pause_msg);
			if (small_integer >= 1 && small_integer <= 9) {
				MsgSendPulse(server_coid, SchedGet(0, 0, NULL),
				METRONOME_PULSE_CODE, small_integer);
			} else {
				printf("Integer is not between 1 and 9.\n");
			}
			break;
		case QUIT:
			MsgSendPulse(server_coid, SchedGet(0, 0, NULL), QUIT_PULSE_CODE,
					small_integer);
			break;
		case START:
			MsgSendPulse(server_coid, SchedGet(0, 0, NULL), START_PULSE_CODE,
					small_integer);
			break;
		case STOP:
			MsgSendPulse(server_coid, SchedGet(0, 0, NULL), STOP_PULSE_CODE,
					small_integer);
			break;
		case SET:
			for (i = 0; i < 4; i++) {
				set_msg = strsep(&buf, " ");
				if (i == 1) {
					Metronome.m_props.beatsPerMin = atoi(set_msg);
				} else if (i == 2) {
					Metronome.m_props.timeSigTop = atoi(set_msg);
				} else if (i == 3) {
					Metronome.m_props.timeSigBot = atoi(set_msg);
				}
			}
			MsgSendPulse(server_coid, SchedGet(0, 0, NULL), SET_PULSE_CODE,
					small_integer);
			break;
		default:
			fprintf(stderr, "\nError - \'%s\' is not a valid command. \n", buf);
			strcpy(data, buf);
			break;
		}

		nb = msg->i.nbytes;
	}

	_IO_SET_WRITE_NBYTES(ctp, nb);

	if (msg->i.nbytes > 0)
		mocb->ocb.flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;

	return (_RESMGR_NPARTS(0));
}

int io_open(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle,
		void *extra) {
	if ((server_coid = name_open(METRO_ATTACH, 0)) == ERROR) {
		perror("ERROR - name_open failed - io_open() \n ");
		return EXIT_FAILURE;
	}
	return (iofunc_open_default(ctp, msg, &handle->attr, extra));
}

void* metronome_thread(void *ta) {
	timer_t timer;
	my_message_t msg;
	int rcvid;
	int index = 0;
	int timerStat;
	char *tablePointer;
	struct sigevent event;
	struct itimerspec itime;

	// Initialize name_attach
	if ((attach = name_attach(NULL, METRO_ATTACH, 0)) == NULL) {
		printf("Error - name_attach - ./metronome.c\n");
		exit(EXIT_FAILURE);
	}

	// Configure event for timer
	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0, attach->chid,
			_NTO_SIDE_CHANNEL, 0);
	event.sigev_priority = SIGEV_PULSE_PRIO_INHERIT;
	event.sigev_code = MIN_PULSE_CODE;

	// Create and configure timer
	timer_create(CLOCK_REALTIME, &event, &timer);
	index = search(&Metronome);
	setTimer(&Metronome);
	startTimer(&itime, timer, &Metronome);
	tablePointer = metronomeData[index].pattern;

	while (RUNNING) {
		// Receive messages
		if ((rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL)) == ERROR) {
			printf("Error - MessageReceive() - ./metronome\n");
			exit(EXIT_FAILURE);
		} else if (rcvid == PULSE) {
			switch (msg.pulse.code) {
			case METRONOME_PULSE_CODE:
				if (timerStat == STARTED) {
					itime.it_value.tv_sec = msg.pulse.value.sival_int;
					timer_settime(timer, 0, &itime, NULL);
				}
				break;
			case SET_PULSE_CODE:
				index = search(&Metronome);
				tablePointer = metronomeData[index].pattern;
				setTimer(&Metronome);
				startTimer(&itime, timer, &Metronome);
				printf("\n");
				break;
			case START_PULSE_CODE:
				if (timerStat == STOPPED) {
					startTimer(&itime, timer, &Metronome);
					timerStat = STARTED;
				}
				break;
			case STOP_PULSE_CODE:
				if (timerStat == STARTED || timerStat == PAUSED) {
					stopTimer(&itime, timer);
					timerStat = STOPPED;
				}
				break;
			case MIN_PULSE_CODE:
				if (*tablePointer == '|') {
					printf("%.2s", tablePointer);
					tablePointer = (tablePointer + 2);
				} else if (*tablePointer == '\0') {
					printf("\n");
					tablePointer = metronomeData[index].pattern;
				} else {
					printf("%c", *tablePointer++);
				}
				break;
			case QUIT_PULSE_CODE:
				timer_delete(timer);
				name_detach(attach, 0);
				name_close(server_coid);
				exit(EXIT_SUCCESS);
			}
		}
		fflush(stdout);
	}

	return NULL;
}

void setTimer(metronome_settings_t *Metronome) {
	Metronome->t_props.beatsPerSec = (double) 60
			/ Metronome->m_props.beatsPerMin;
	Metronome->t_props.measure = Metronome->t_props.beatsPerSec * 2;
	Metronome->t_props.interval = Metronome->t_props.measure
			/ Metronome->m_props.timeSigBot;
	Metronome->t_props.nano_sec = (Metronome->t_props.interval
			- (int) Metronome->t_props.interval) * 1e+9;
}

int userInput(char *buf) {
	for (int i = 0; i < NUM_INPUTS; i++) {
		if (strstr(buf, userInputs[i]) != NULL) {
			return i;
		}
	}
	return ERROR;
}

int search(metronome_settings_t *Metronome) {
	for (int index = 0; index < 8; index++) {
		if (metronomeData[index].timeSigBot == Metronome->m_props.timeSigBot
				&& metronomeData[index].timeSigTop
						== Metronome->m_props.timeSigTop) {
			return index;
		}
	}
	return ERROR;
}

void stopTimer(struct itimerspec *itime, timer_t timer) {
	itime->it_value.tv_sec = 0;
	timer_settime(timer, 0, itime, NULL);
}

void startTimer(struct itimerspec *itime, timer_t timer,
		metronome_settings_t *Metronome) {
	itime->it_value.tv_sec = 1;
	itime->it_value.tv_nsec = 0;
	itime->it_interval.tv_sec = Metronome->t_props.interval;
	itime->it_interval.tv_nsec = Metronome->t_props.nano_sec;
	timer_settime(timer, 0, itime, NULL);
}

metro_ocb_t* metro_ocb_calloc(resmgr_context_t *ctp, ioattr_t *mattr) {
	metro_ocb_t *mocb;
	mocb = calloc(1, sizeof(metro_ocb_t));
	mocb->ocb.offset = 0;
	return (mocb);
}

void metro_ocb_free(metro_ocb_t *mocb) {
	free(mocb);
}
