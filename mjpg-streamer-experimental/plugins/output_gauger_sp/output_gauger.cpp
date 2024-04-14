/*******************************************************************************
#                                                                              #
#      MJPG-streamer allows to stream JPG frames from an input-plugin          #
#      to several output plugins                                               #
#                                                                              #
#      Copyright (C) 2007 Tom Stöveken                                         #
#                                                                              #
# This program is free software; you can redistribute it and/or modify         #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation; version 2 of the License.                      #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with this program; if not, write to the Free Software                  #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA    #
#                                                                              #
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <getopt.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <list>
#include <iterator>
#include <sqlite3.h>
#include <math.h>

#include "../../utils.h"
#include "../../mjpg_streamer.h"
#include "output_gauger.h"
#include "Gauger.h"
#include "cmd.h"
#include "log.h"

//#define CONFIG_DB "/opt/data/upl_config.db"
//#define STATE_DB "/opt/data/upl_state.db"
//#define LOG_FILE "/opt/upl/upl.log"
#define CONFIG_DB "/tmp/upl_config.db"
#define STATE_DB "/tmp/upl_state.db"
#define LOG_FILE "/tmp/upl.log"
#define LOG_LEVEL 4

#define OUTPUT_PLUGIN_NAME "Gauger output plugin"

using namespace std;

static pthread_t cal;
static pthread_t img_save;
globals *pglobal;
static int delay, max_frame_size = 0;
static unsigned char *frame = NULL;
static int input_number = 0;
static list<long> gauger_observers;
static struct timeval ts;
static gaugerState current_state = STATE_STOPPED;
static float offsetx;
static float offsety;

typedef struct {
    unsigned int times;
    unsigned int interval;
    unsigned char level;
} CalParam;
static CalParam cal_param = {0, 0, 0};

typedef struct {
    int frame_size;
    unsigned char *frame;
} ImgParam;
static ImgParam img_param = {0, 0};

CGauger *g_gauger;
extern TARGETPARAM targetParam;
unsigned char SHAKE_LEVEL[3] = { 1, 2, 5 };

#ifdef __cplusplus
extern "C" {
#endif
static int start_calculating(bool is_resumed);
#ifdef __cplusplus
}
#endif

/******************************************************************************
Description.: print a help message
Input Value.: -
Return Value: -
******************************************************************************/
void help(void)
{
    fprintf(stderr, " ---------------------------------------------------------------\n" \
            " Help for output plugin..: "OUTPUT_PLUGIN_NAME"\n" \
            " ---------------------------------------------------------------\n" \
            " The following parameters can be passed to this plugin:\n\n" \
            " [-i | --input ].......: read frames from the specified input plugin\n\n" \
            " ---------------------------------------------------------------\n");
}

/******************************************************************************
Description.: clean up allocated ressources
Input Value.: unused argument
Return Value: -
******************************************************************************/
void cal_cleanup(void *arg)
{
    static unsigned char first_run = 1;

    if(!first_run) {
        SLOGI("already cleaned up ressources");
        return;
    }

    first_run = 0;
    OPRINT("cleaning up ressources allocated by cal thread\n");

    if(frame != NULL) {
        free(frame);
	frame = NULL;
    }
}

static int get_y_data_from_yuyv(unsigned char *des, const unsigned char *src, int size)
{
        int i = 0, j = 0;
        while (i < size) {
                des[j] = src[i];
                j++;
                i += 2;
        }
        return j;
}

int read_frame()
{
    int frame_size = 0;
    unsigned char *tmp_framebuffer = NULL;

    pthread_mutex_lock(&pglobal->in[input_number].db);
    pthread_cond_wait(&pglobal->in[input_number].db_update, &pglobal->in[input_number].db);

    /* read buffer */
    frame_size = pglobal->in[input_number].size_raw;

    /* check if buffer for frame is large enough, increase it if necessary */
    if(NULL == frame || frame_size > max_frame_size) {
        SLOGD("Increasing buffer size to %d", frame_size);

        max_frame_size = frame_size + (1 << 16);
        if((tmp_framebuffer = (unsigned char *)realloc(frame, max_frame_size)) == NULL) {
            pthread_mutex_unlock(&pglobal->in[input_number].db);
            SLOGE("Not enough memory");
            return ERR_MEMORY;
        }

        frame = tmp_framebuffer;
    }

    /* copy frame to our local buffer now */
    memcpy(frame, pglobal->in[input_number].buf_raw, frame_size);
    //get_y_data_from_yuyv(frame, pglobal->in[input_number].buf_raw, pglobal->in[input_number].size_raw);

    /* allow others to access the global buffer again */
    pthread_mutex_unlock(&pglobal->in[input_number].db);

    return frame_size;
}

int calculate_once(unsigned char shake_level)
{
    float x, y, sumx = 0.0, sumy = 0.0, avgx = 0.0, avgy = 0.0;
    POINT_I ptOut;
    list<long>::iterator it;
    unsigned char count = SHAKE_LEVEL[shake_level];
    char buf[64] = {0};
    bool ret;

    for (int index = 0; index < count; index++) {
        read_frame();
	ret = g_gauger->MeasureTarget(frame, ptOut, &x, &y);
        //SLOGI("ptOut=(%d, %d), \tx=%f, y=%f", ptOut.x, ptOut.y, x, y);
	if (!ret) {
	    break;
	}

	if (isnan(x) || isnan(y)) {
	    return ERR_INVALID_VALUE;
	}

	sumx += x;
	sumy += y;
    }

    gettimeofday(&ts, NULL);
    tm* local = localtime(&ts.tv_sec);
    strftime(buf, 64, "%Y-%m-%d %H:%M:%S", local);

    if (!ret) {
    	x = y = 1.0/0.0;
        SLOGI("x=%f,\ty=%f,\tts=%s.%d", x, y, buf, ts.tv_usec/1000);
        for (it = gauger_observers.begin(); it != gauger_observers.end(); it++) {
            (*(((GaugerObserverStub*)(*it))->du))(x, y, &ts);
        }
    } else {
        avgx = sumx / count;
        avgy = 0 - sumy / count;
        SLOGI("x=%f,\ty=%f,\tts=%s.%d", avgx, avgy, buf, ts.tv_usec/1000);
        for (it = gauger_observers.begin(); it != gauger_observers.end(); it++) {
            (*(((GaugerObserverStub*)(*it))->du))(avgx + offsetx, avgy + offsety, &ts);
        }
    }

    return OK;
}

int identify_target()
{
    POINT_I pt[5];
    int ret;
    read_frame();
    
    ret = g_gauger->InitTarget(frame, pt);
    if (0 == ret) {
        SLOGI("Identify target OK");
    } else {
        SLOGW("Failed to identify target");
    }
    return ret;
}

static void state_change(gaugerState new_state)
{
    gaugerState old_state = current_state;
    current_state = new_state;

    sqlite3 *db = NULL;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open(STATE_DB, &db);
    if (rc) {
    	SLOGI("Failed to open %s: %s", STATE_DB, sqlite3_errmsg(db));
	return;
    }

    const char *query = "UPDATE state SET saved_state = ?";
    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, new_state);
    	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
    }
    sqlite3_close(db);

    list<long>::iterator it;
    for (it = gauger_observers.begin(); it != gauger_observers.end(); it++) {
        (*(((GaugerObserverStub*)(*it))->sc))(old_state, new_state);
    }
}

void *calculate_thread(void *arg)
{
    CalParam *param = (CalParam*)arg;
    unsigned int times = param->times;
    unsigned int interval = param->interval;
    unsigned char level = param->level;
    max_frame_size = 0;

    if (0 == interval) {
        level = 0;
    }

    SLOGI("Start calculating thread, times: %d, interval: %d", times, interval);
    pthread_cleanup_push(cal_cleanup, NULL);
    state_change(STATE_STARTED);
    if (calculate_once(level) < 0) {
        current_state = STATE_STOPPED;
        start_calculating(false);
        pthread_exit((void *)0);
    }
    if (times > 0) {
         times--;
         if (0 == times) {
             state_change(STATE_STOPPED);
             pthread_exit((void *)0);
         }
    }
    // Full speed calculation
    if (0 == interval) {
        // Keep calculting until stopped by hand
        if (0 == times) {
            while(1) calculate_once(level);
	} else {
	    for (; times > 0; times--) {
    	        calculate_once(level);
            }
            state_change(STATE_STOPPED);
	}
    }
    // Calculating with specified interval
    else {
        sleep(interval);
        if (0 == times) {
            while(1) {
                calculate_once(level);
                sleep(interval);
            }
        } else {
	    for (; times > 0; times--) {
                calculate_once(level);
                sleep(interval);
            }
            state_change(STATE_STOPPED);
        }
    }

    pthread_cleanup_pop(1);
    return NULL;
}

void *img_save_thread(void *arg)
{
    sqlite3 *state_db = NULL;
    sqlite3_stmt *stmt;
    ImgParam *param;
    int rc;

    param = (ImgParam*)arg;
    rc = sqlite3_open(STATE_DB, &state_db);
    if (rc) {
        SLOGI("Failed to open %s: %s", STATE_DB, sqlite3_errmsg(state_db));
    }

    const char *update_query = "UPDATE state SET saved_state=1, init_image=?, image_size=?";
    if (SQLITE_OK == sqlite3_prepare_v2(state_db, update_query, strlen(update_query), &stmt, 0)) {
        sqlite3_bind_blob(stmt, 1, param->frame, param->frame_size, NULL);
        sqlite3_bind_int(stmt, 2, param->frame_size);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(state_db);
    return NULL;
}

#ifdef __cplusplus
extern "C" {
#endif

/*** plugin interface functions ***/
/******************************************************************************
Description.: this function is called first, in order to initialize
              this plugin and pass a parameter string
Input Value.: parameters
Return Value: 0 if everything is OK, non-zero otherwise
******************************************************************************/
int output_init(output_parameter *param)
{
    int i;
    delay = 0;

    if (0 != init_log("GAUGER", LOG_FILE, 0, LOG_LEVEL, 0)) {
        SLOGI("Failed to init log");
        return -1;
    }

    SLOGI("Gauger plugin start...");

    param->argv[0] = (char *)OUTPUT_PLUGIN_NAME;

    /* show all parameters for SLOGI purposes */
    for(i = 0; i < param->argc; i++) {
        SLOGI("argv[%d]=%s", i, param->argv[i]);
    }

    reset_getopt();
    while(1) {
        int option_index = 0, c = 0;
        static struct option long_options[] = {
            {"h", no_argument, 0, 0
            },
            {"help", no_argument, 0, 0},
            {"i", required_argument, 0, 0},
            {"input", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        c = getopt_long_only(param->argc, param->argv, "", long_options, &option_index);

        /* no more options to parse */
        if(c == -1) break;

        /* unrecognized option */
        if(c == '?') {
            help();
            return 1;
        }

        switch(option_index) {
            /* h, help */
        case 0:
        case 1:
            SLOGI("case 0,1");
            help();
            return 1;
            break;

        case 2:
        case 3:
            SLOGI("case 2,3");
            input_number = atoi(optarg);
            break;
        }
    }

    pglobal = param->global;
    if(!(input_number < pglobal->incnt)) {
        SLOGE("ERROR: the %d input_plugin number is too much only %d plugins loaded\n", input_number, pglobal->incnt);
        return 1;
    }

    OPRINT("input plugin.....: %d: %s\n", input_number, pglobal->in[input_number].plugin);

    init_ctrl_param(&pglobal->out[param->id]);
    g_gauger  = get_gauger_instance();

    return OK;
}

/******************************************************************************
Description.: 
Input Value.: -
Return Value: always 0
******************************************************************************/
int output_stop(int id)
{
    return OK;
}

static int start_calculating(bool is_resumed)
{
    POINT_I pt[5];
    int init_ret, frame_size;
    sqlite3 *state_db = NULL, *config_db = NULL;
    sqlite3_stmt *stmt;
    char **result;
    char *errMsg = NULL;
    int rc, cal_times = 0, cal_interval = 0, shake_level = 0, target_type = 25.0, row_count = 0, column_count = 0;
    unsigned char *saved_frame = NULL;
    unsigned char *tmp_framebuffer = NULL;
    
    if (current_state == STATE_STARTED) {
      return ERR_CALCULATING;
    }
    current_state = STATE_STARTED;

    rc = sqlite3_open(CONFIG_DB, &config_db);
    if (rc) {
    	SLOGE("Failed to open %s: %s\n", CONFIG_DB, sqlite3_errmsg(config_db));
        current_state = STATE_STOPPED;
        return ERR_DB;
    }

    const char *config_query = "select offsetx, offsety, cal_times, cal_interval, grade, target_type from camera_config";
    sqlite3_get_table(config_db, config_query, &result, &row_count, &column_count, &errMsg);
    if (row_count > 0) {
        offsetx = atof(result[column_count]);
	offsety = atof(result[column_count + 1]);
        cal_times = atoi(result[column_count + 2]);
        cal_interval = atoi(result[column_count + 3]);
        shake_level = atoi(result[column_count + 4]);
        target_type = atoi(result[column_count + 5]);
    }
    sqlite3_free_table(result);
    sqlite3_close(config_db);

    cal_param.times = cal_times;
    cal_param.interval = cal_interval;
    cal_param.level = shake_level;
    targetParam.DistLR = target_type;
    targetParam.DistTB = target_type;
    g_gauger->SetTargetParam(targetParam);

    // Resuming last calculating, grab image from db
    if (is_resumed) {
        rc = sqlite3_open(STATE_DB, &state_db);
        if (rc) {
            SLOGE("Failed to open %s: %s\n", STATE_DB, sqlite3_errmsg(state_db));
            current_state = STATE_STOPPED;
            return ERR_DB;
        }

        const char *query = "select * from state";
        if (SQLITE_OK == sqlite3_prepare_v2(state_db, query, strlen(query), &stmt, 0)) {
            sqlite3_step(stmt);
            saved_frame = (unsigned char*)sqlite3_column_blob(stmt, 2);
            frame_size = sqlite3_column_int(stmt, 3);

            if(NULL == frame || frame_size > max_frame_size) {
                SLOGI("Increasing buffer size to %d", frame_size);
        
                max_frame_size = frame_size + (1 << 16);
                if((tmp_framebuffer = (unsigned char *)realloc(frame, max_frame_size)) == NULL) {
                    LOG("not enough memory\n");
                    current_state = STATE_STOPPED;
                    return ERR_MEMORY;
                }
                frame = tmp_framebuffer;
            }

            memcpy(frame, saved_frame, frame_size);
        }
        if ((init_ret = g_gauger->InitTarget(frame, pt)) < 0) {
            current_state = STATE_STOPPED;
            SLOGW("Failed to start: target not identified");
            return init_ret;
        }
        sqlite3_finalize(stmt);
        sqlite3_close(state_db);
    }
    // New calculating, re-init camera and save image
    else {
        frame_size = read_frame();
	img_param.frame_size = frame_size;
	img_param.frame = frame;

        if ((init_ret = g_gauger->InitTarget(frame, pt)) < 0) {
            current_state = STATE_STOPPED;
            SLOGW("Failed to start: target not identified");
            return init_ret;
        }

        if (0 != pthread_create(&img_save, 0, img_save_thread, (void*)&img_param)) {
            current_state = STATE_STOPPED;
            SLOGW("Failed to start: thread error");
            return ERR_SAVE_IMAG;
        }
        pthread_detach(img_save);
    }

    SLOGI("Init point info:");
    SLOGI("pt[0]=(%d, %d)", pt[0].x, pt[0].y);
    SLOGI("pt[1]=(%d, %d)", pt[1].x, pt[1].y);
    SLOGI("pt[2]=(%d, %d)", pt[2].x, pt[2].y);
    SLOGI("pt[3]=(%d, %d)", pt[3].x, pt[3].y);
    SLOGI("pt[4]=(%d, %d)", pt[4].x, pt[4].y);

    if (0 != pthread_create(&cal, 0, calculate_thread, (void*)&cal_param)) {
        current_state = STATE_STOPPED;
        return ERR_THREAD;
    }
    pthread_detach(cal);

    return OK;
}


/******************************************************************************
Description.: 
Input Value.: -
Return Value: always 0
******************************************************************************/
int output_run(int id)
{
#if 0
    sqlite3 *db = NULL;
    unsigned char *saved_frame = NULL;
    unsigned char *tmp_framebuffer = NULL;
    sqlite3_stmt *stmt;
    int init_ret, saved_state, frame_size;
    POINT_I pt[5];

    int rc = sqlite3_open(STATE_DB, &db);
    if (rc) {
    	SLOGI("Failed to open %s: %s\n", STATE_DB, sqlite3_errmsg(db));
        return ERR_DB;
    }

    const char *query = "select * from state";
    if (SQLITE_OK == sqlite3_prepare_v2(db, query, strlen(query), &stmt, 0)) {
    	sqlite3_step(stmt);
	saved_state = sqlite3_column_int(stmt, 1);
	saved_frame = (unsigned char*)sqlite3_column_blob(stmt, 2);
	frame_size = sqlite3_column_int(stmt, 3);

        if(NULL == frame || frame_size > max_frame_size) {
            SLOGI("increasing buffer size to %d\n", frame_size);
    
            max_frame_size = frame_size + (1 << 16);
            if((tmp_framebuffer = (unsigned char *)realloc(frame, max_frame_size)) == NULL) {
                LOG("not enough memory\n");
                return ERR_MEMORY;
            }
	    frame = tmp_framebuffer;
	}

        memcpy(frame, saved_frame, frame_size);
    
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (STATE_STARTED == saved_state) {
        if ((init_ret = g_gauger->InitTarget(frame, pt)) < 0) {
            return init_ret;
        }
        SLOGI("pt[0]=(%d, %d)\n", pt[0].x, pt[0].y);
        SLOGI("pt[1]=(%d, %d)\n", pt[1].x, pt[1].y);
        SLOGI("pt[2]=(%d, %d)\n", pt[2].x, pt[2].y);
        SLOGI("pt[3]=(%d, %d)\n", pt[3].x, pt[3].y);
        SLOGI("pt[4]=(%d, %d)\n", pt[4].x, pt[4].y);

        start_calculating(true);
    }
#endif
    return OK;
}

static void stop_calculating()
{
    pthread_cancel(cal);
    state_change(STATE_STOPPED);
}

int output_cmd(int plugin, unsigned int control_id, unsigned int group, long value)
{
    SLOGI("command (%d, value: %ld) for group %d triggered for plugin instance #%02d", control_id, value, group, plugin);

    switch(control_id){
        case CMD_UNREGISTER: // Unregister
            gauger_observers.remove(value);
            break;
        case CMD_REGISTER: // Register
            gauger_observers.push_back(value);
            break;
        case CMD_START:
            SLOGI("Received START command");
            return start_calculating(false);
            break;
        case CMD_STOP:
            SLOGI("Received STOP command");
            stop_calculating();
            break;
        case CMD_RESUME:
            return start_calculating(true);
            break;
        case CMD_CAL:
            calculate_once(0);
            break;
        case CMD_TARGET:
            SLOGI("Received IDENTIFY_TARGET command");
            return identify_target();
            break;
        case CMD_STATE:
            break;
        default:
            SLOGI("command not support!");
    }

    return OK;
}


#ifdef __cplusplus
};
#endif
