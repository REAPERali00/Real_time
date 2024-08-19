#ifndef DOOR_ENTRY_SYSTEM_H_
#define DOOR_ENTRY_SYSTEM_H_

#define NUM_STATES 13

#define RUNNING 1 /** RUNNING DEFINE FOR INFINITE LOOPS */

#define LEFT 0    /* Going down the LEFT path */
#define RIGHT 1   /* Going down the RIGHT path */
#define DEFAULT 2 /* Neither LEFT nor RIGHT selected */

typedef void* (*FState)();

typedef enum {
	STATE_LEFT_SCAN, STATE_RIGHT_SCAN, STATE_WEIGHT_SCALE, //2
	STATE_LEFT_OPEN,
	STATE_RIGHT_OPEN,
	STATE_LEFT_CLOSED, //5
	STATE_RIGHT_CLOSED,
	STATE_GUARD_LEFT_UNLOCK,
	STATE_GUARD_LEFT_LOCK,
	STATE_GUARD_RIGHT_UNLOCK, //9
	STATE_GUARD_RIGHT_LOCK,
	STATE_EXIT,
	STATE_START
} State;

#define NUM_INPUTS 12

typedef enum {
	INPUT_LEFT_SCAN,
	INPUT_RIGHT_SCAN,
	INPUT_WEIGHT_SCALE,
	INPUT_LEFT_OPEN,
	INPUT_RIGHT_OPEN,
	INPUT_LEFT_CLOSED,
	INPUT_RIGHT_CLOSED,
	INPUT_GUARD_LEFT_UNLOCK,
	INPUT_GUARD_LEFT_LOCK,
	INPUT_GUARD_RIGHT_UNLOCK,
	INPUT_GUARD_RIGHT_LOCK,
	INPUT_EXIT
} Input;

const char *inMessage[NUM_INPUTS] = { "ls", "rs", "ws", "lo", "ro", "lc", "rc",
		"glu", "gll", "gru", "grl", "exit" };

#define NUM_OUTPUTS 11

typedef enum {
	OUTPUT_ID_SCAN,
	OUTPUT_WEIGHED,
	OUTPUT_LEFT_OPENED,
	OUTPUT_RIGHT_OPENED,
	OUTPUT_LEFT_CLOSED,
	OUTPUT_RIGHT_CLOSED,
	OUTPUT_LEFT_LOCKED,
	OUTPUT_RIGHT_LOCKED,
	OUTPUT_LEFT_UNLOCKED,
	OUTPUT_RIGHT_UNLOCKED,
	OUTPUT_EXIT
} Output;

const char *outMessage[NUM_OUTPUTS] = {
		"Person scanned ID. ID: %d",
		"Person has been weighed. Weight: %d", "Person opened Left Door",
		"Person opened Right Door", "Left door closed (automatically)",
		"Right door closed (automatically)", "Left door Locked by Guard",
		"Right door Locked by Guard", "Left door Unlocked by Guard",
		"Right door Unlocked by Guard", "Exit"
};

typedef struct {
	int person_id;
	int weight;
	int direction;
	State state;
} Person;

typedef struct {
	int output_type;
	Person person;
} Display;

#define CTRL_ERR_RCV 0
#define CTRL_ERR_SND 1

#endif /* DOOR_ENTRY_SYSTEM_H_ */
