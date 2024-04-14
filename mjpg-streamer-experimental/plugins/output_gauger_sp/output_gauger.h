#ifndef __OUTPUT_GAUGER_H
#define __OUTPUT_GAUGER_H

#define OK			0
#define ERR_DB			-8
#define ERR_CONFIG		-9
#define ERR_CALCULATING		-10
#define ERR_STOP_FAIL		-11
#define ERR_SAVE_IMAG		-12
#define ERR_MEMORY		-15
#define ERR_INVALID_VALUE	-16
#define ERR_THREAD		-17

typedef enum {
	CMD_UNREGISTER = 0,
	CMD_REGISTER,
	CMD_START,
	CMD_STOP,
	CMD_RESUME,
	CMD_CAL,
	CMD_TARGET,
	CMD_STATE,
	CMD_UNKNOWN = 255
} control_id_t;

typedef enum {
    STATE_STOPPED = 0,
    STATE_STARTED,
    STATE_UNKNOWN = 255
} gaugerState;

typedef void(*data_update_cb)(double x, double y, struct timeval *ts);
typedef void(*state_change_cb)(unsigned char old_state, unsigned char new_state);
typedef struct {
    data_update_cb du;
    state_change_cb sc;
} GaugerObserverStub;

#endif
