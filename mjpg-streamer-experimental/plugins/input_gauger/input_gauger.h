#ifndef INPUT_OPENCV_H_
#define INPUT_OPENCV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../../mjpg_streamer.h"
#include "../../utils.h"


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
    CMD_CAL_ONCE,
    CMD_INIT_TARGET,
    CMD_STATE,
    CMD_SNAPSHUT,
    CMD_UNKNOWN = 255
} Control_id_t;

typedef enum {
    STATE_UNINITIALIZED = 0, // gauger 还没有
    STATE_INITIALIZED, // 已初始化或已停止
    STATE_STARTED,
    STATE_UNKNOWN = 255
} GaugerState;

// Param for a calculation
typedef struct {
    unsigned int times;    // 测量次数
    unsigned int interval; // 测量间隔， 单位：ms
    unsigned char level;   // 取连续多少次作平均
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


int input_init(input_parameter* param, int id);
int input_stop(int id);
int input_run(int id);
int input_cmd(int plugin, unsigned int control_id, unsigned int typecode, int value, char *value_string);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_OPENCV_H_ */
