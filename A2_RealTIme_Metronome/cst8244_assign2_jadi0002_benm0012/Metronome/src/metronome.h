#ifndef METRONOME_H_
#define METRONOME_H_

struct ioattr_t;
#define IOFUNC_ATTR_T struct ioattr_t
struct metro_ocb;
#define IOFUNC_OCB_T struct metro_ocb

#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <math.h>
#include <sys/types.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>

#define MIN_PULSE_CODE                  _PULSE_CODE_MINAVAIL         //0
#define METRONOME_PULSE_CODE            (_PULSE_CODE_MINAVAIL +1)    //1
#define START_PULSE_CODE                (_PULSE_CODE_MINAVAIL +2)    //2
#define STOP_PULSE_CODE                 (_PULSE_CODE_MINAVAIL +3)    //3
#define SET_PULSE_CODE                  (_PULSE_CODE_MINAVAIL +4)    //4
#define QUIT_PULSE_CODE                 (_PULSE_CODE_MINAVAIL +5)    //5

#define METRO_ATTACH  "metronome"

#define STARTED 0
#define STOPPED 1
#define PAUSED 2

#define PULSE 0
#define ERROR -1
#define RUNNING 1

struct DataTableRow {
	int timeSigTop;
	int timeSigBot;
	int intervals;
	char pattern[16];
};

struct DataTableRow metronomeData[] = { { 2, 4, 4, "|1&2&" }, { 3, 4, 6,
		"|1&2&3&" }, { 4, 4, 8, "|1&2&3&4&" }, { 5, 4, 10, "|1&2&3&4-5-" }, { 3,
		8, 6, "|1-2-3-" }, { 6, 8, 6, "|1&a2&a" }, { 9, 8, 9, "|1&a2&a3&a" }, {
		12, 8, 12, "|1&a2&a3&a4&a" } };

typedef union {
	struct _pulse pulse;
	char msg[255];
} my_message_t;

typedef struct {
	double beatsPerSec;
	double measure;
	double interval;
	double nano_sec;
} timer_props_t;

typedef struct {
	int beatsPerMin;
	int timeSigTop;
	int timeSigBot;
} metro_props_t;

typedef struct {
	metro_props_t m_props;
	timer_props_t t_props;
} metronome_settings_t;

#define DEVICES 2
#define METRONOME 0
#define METRONOME_HELP 1

char *devnames[DEVICES] =
		{ "/dev/local/metronome", "/dev/local/metronome-help" };

typedef struct ioattr_t {
	iofunc_attr_t attr;
	int device;
} ioattr_t;

typedef struct metro_ocb {
	iofunc_ocb_t ocb;
	char buffer[50];
} metro_ocb_t;

#define NUM_INPUTS 5
typedef enum {
	PAUSE, QUIT, START, STOP, SET
} Inputs;

char *userInputs[NUM_INPUTS] = { "pause", "quit", "start", "stop", "set" };

int userInput(char *buf);
int io_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb);
int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb);
int io_open(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle,
		void *extra);
void* metronome_thread(void *metronome);
void setTimer(metronome_settings_t *Metronome);
int search(metronome_settings_t *Metronome);
void stopTimer(struct itimerspec *itime, timer_t timer_id);
void startTimer(struct itimerspec *itime, timer_t timer_id,
		metronome_settings_t *Metronome);
void printUsage();
metro_ocb_t* metro_ocb_calloc(resmgr_context_t *ctp, IOFUNC_ATTR_T *mtattr);
void metro_ocb_free(IOFUNC_OCB_T *mocb);

#endif /* SRC_METRONOME_H_ */
