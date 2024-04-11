#ifndef GAUGER_OBSERVER_H
#define GAUGER_OBSERVER_H

// Error number
#define OK			0
#define ERR_DB			-8
#define ERR_CONFIG		-9
#define ERR_CALCULATING		-10
#define ERR_STOP_FAIL		-11
#define ERR_SAVE_IMAG		-12
#define ERR_MEMORY		-15
#define ERR_INVALID_VALUE	-16
#define ERR_THREAD		-17
#define ERR_INVALID_CMD		-18
#define ERR_INVALID_TEMP_ID	-19
#define ERR_UNINITED		-20
#define ERR_TIME		-21
#define ERR_FIFO		-22

#include <vector>

// Command
typedef enum {
    CMD_UNREGISTER = 0,
    CMD_REGISTER,
    CMD_START,
    CMD_STOP,
    CMD_RESUME,
    CMD_CAL,
    CMD_TARGET,
    CMD_STATE,
    CMD_TEMP_CREATE,
    CMD_TEMP_UPDATE,
    CMD_TEMP_DELETE,
    CMD_TEMP_DELETE_ALL,
    CMD_TEMP_RETRIEVE,
    CMD_TEMP_RETRIEVE_BY_ID,
    CMD_CAL_PARAM_UPDATE,
    CMD_CAL_PARAM_RETRIEVE,
    CMD_DOWNLOAD,
    CMD_UNKNOWN = 255
} control_id_t;

// State
typedef enum {
    STATE_STOPPED = 0,
    STATE_STARTED,
    STATE_UNKNOWN = 255
} GaugerState;

// Param for a calculation
typedef struct {
    unsigned int times;
    unsigned int interval;
    unsigned char level;
} CalParam;

typedef struct {
    int width;
    int height;
    struct timeval ts;
    unsigned char *data;
} Frame;

typedef struct {
    unsigned char tempId;
    float offsetX;
    float offsetY;
} CalResult;

typedef struct {
    struct timeval ts;
    std::vector<CalResult> data;
} CalResults;

typedef void(*data_update_cb)(int id, float x, float y, float offsetx, float offsety, struct timeval *ts);
typedef void(*state_change_cb)(unsigned char old_state, unsigned char new_state);
typedef struct {
    data_update_cb du;
    state_change_cb sc;
} GaugerObserver;

#endif // GAUGER_OBSERVER_H
