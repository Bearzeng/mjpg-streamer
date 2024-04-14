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
#include <vector>
#include <iterator>
#include <math.h>

#include "../../utils.h"
#include "../../mjpg_streamer.h"
#include "gauger_observer.h"
#include "Gauger.h"
#include "cmd.h"
#include "CppSQLite3.h"
#include "calculation.h"
#include "cal_control.h"
#include "cir_queue.h"

//#define UPL_CONFIG_DB "/opt/data/upl_config.db"
//#define UPL_STATE_DB "/opt/data/upl_state.db"
//#define UPL_DATA_DB "/opt/data/upl_data.db"
//#define UPL_FIFO "/opt/upl/fifo"

#define UPL_CONFIG_DB "/tmp/upl_config.db"
#define UPL_STATE_DB "/tmp/upl_state.db"
#define UPL_DATA_DB "/tmp/upl_data.db"
#define UPL_FIFO "/tmp/fifo"

#define OUTPUT_PLUGIN_NAME "Gauger output plugin"

using namespace std;

globals *pglobal;
static pthread_t data_io;
static pthread_t download;
int delay, max_frame_size = 0;
Frame global_frame;
static int input_number = 0;
static GaugerState current_state = STATE_STOPPED;
list<long> gauger_observers;
static list<Template> temp_list;
vector<Calculation*> cal_list;
CalControl *calctl_thread;
cir_queue_t data_queue;
static CalResults cal_results;

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

static int read_frame()
{
    int frame_size = 0;
    unsigned char *tmp_framebuffer = NULL;
    char *resolution;

    pthread_mutex_lock(&pglobal->in[input_number].db);
    pthread_cond_wait(&pglobal->in[input_number].db_update, &pglobal->in[input_number].db);

    /* read buffer */
    frame_size = pglobal->in[input_number].size_raw / 2;

    /* check if buffer for frame is large enough, increase it if necessary */
    if(NULL == global_frame.data || frame_size > max_frame_size) {
        printf("increasing buffer size to %d\n", frame_size);

        max_frame_size = frame_size + (1 << 16);
        if((tmp_framebuffer = (unsigned char *)realloc(global_frame.data, max_frame_size)) == NULL) {
            pthread_mutex_unlock(&pglobal->in[input_number].db);
            LOG("not enough memory\n");
            return ERR_MEMORY;
        }

        global_frame.data = tmp_framebuffer;
    }

    /* copy frame to our local buffer now */
    memcpy(global_frame.data, pglobal->in[input_number].buf_raw, frame_size);
    //get_y_data_from_yuyv(global_frame.data, pglobal->in[input_number].buf_raw, pglobal->in[input_number].size_raw);
    resolution = pglobal->in[input_number].param.argv[4];
    //sscanf(resolution, "%dx%d", &global_frame.width, &global_frame.height);
    global_frame.width = 720;
    global_frame.height = 540;
    gettimeofday(&global_frame.ts, NULL);

    /* allow others to access the global buffer again */
    pthread_mutex_unlock(&pglobal->in[input_number].db);

    return frame_size;
}

void cleanup_cal_list()
{
    for (unsigned int i = 0; i < cal_list.size(); i++) {
        while(!cal_list[i]->isWaiting()) usleep(1000);

        cal_list[i]->stop();
	usleep(1000);
        cal_list[i]->resume();

	while(!cal_list[i]->isExited()) usleep(1000);

	printf("Cleanup cal_list[%d]\n", i);
	delete cal_list[i];
	cal_list[i] = NULL;
    }
    cal_list.clear();
}

void state_change(GaugerState new_state)
{
    GaugerState old_state = current_state;
    current_state = new_state;
    CppSQLite3DB db;

    CppSQLite3Buffer sql;

    sql.format("UPDATE state SET saved_state = %d", new_state);

    try {
        db.open(UPL_STATE_DB);
        db.execDML(sql);
        db.close();
    } catch (CppSQLite3Exception e) {
    	printf("ERR_DB [%s] %s\n", UPL_STATE_DB, e.errorMessage());
	return;
    }

    list<long>::iterator it;
    for (it = gauger_observers.begin(); it != gauger_observers.end(); it++) {
        (*(((GaugerObserver*)(*it))->sc))(old_state, new_state);
    }
}

void *data_io_thread(void *arg)
{
    CppSQLite3DB db;
    CppSQLite3Buffer sql_insert;
    CalData data;

    try {
        db.open(UPL_DATA_DB);
    } catch (CppSQLite3Exception e) {
        printf("Failed to start data IO: ERR_DB [%s] %s\n", UPL_DATA_DB, e.errorMessage());
        return (void*)0;
    }

    while(1){
        try {
            if (pop_cir_queue(&data_queue, &data)) {
                sql_insert.format("INSERT INTO data (template_id, x, y, offsetx, offsety, timestamp) VALUES (%d, %f, %f, %f, %f, '%s')",
                                  data.temp_id, data.x, data.y, data.offsetx, data.offsety, data.ts.c_str());
                printf("Save data id:%d, x:%f, y:%f, time:%s\n", data.temp_id, data.x, data.y, data.ts.c_str());
                //printf("Save data id:%d, x:%f, y:%f, time:%s\n", data.temp_id, data.x, data.y, QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz ddd").toStdString().c_str());
                db.execDML(sql_insert);
                //printf("Data saved, time:%s\n", QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz ddd").toStdString().c_str());
            } else {
                //db_io_running = false;
                usleep(100 * 1000);
            }
        } catch (CppSQLite3Exception e) {
            printf("ERR_DB [%s] %s\n", UPL_DATA_DB, e.errorMessage());
        }
    }
    db.close();
    return (void*)0;
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

    param->argv[0] = (char *)OUTPUT_PLUGIN_NAME;

    /* show all parameters for printf purposes */
    for(i = 0; i < param->argc; i++) {
        printf("argv[%d]=%s\n", i, param->argv[i]);
    }

    reset_getopt();
    while(1) {
        int option_index = 0, c = 0;
        static struct option long_options[] = {
            {"h", no_argument, 0, 0
            },
            {"help", no_argument, 0, 0},
            //{"i", required_argument, 0, 0},
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
            printf("case 0,1\n");
            help();
            return 1;
            break;

        case 2:
        case 3:
            printf("case 2,3\n");
            input_number = atoi(optarg);
            break;
        }
    }

    pglobal = param->global;
    if(!(input_number < pglobal->incnt)) {
        OPRINT("ERROR: the %d input_plugin number is too much only %d plugins loaded\n", input_number, pglobal->incnt);
        return 1;
    }

    OPRINT("input plugin.....: %d: %s\n", input_number, pglobal->in[input_number].plugin);

    init_ctrl_param(&pglobal->out[param->id]);

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

static int update_cal_param(long value)
{
    CalParam *param = (CalParam*)value;
    CppSQLite3DB db;
    CppSQLite3Buffer sql;

    try {
        db.open(UPL_CONFIG_DB);
        sql.format("UPDATE camera_config SET cal_times=%d, cal_interval=%d, grade=%d", param->times, param->interval, param->level);
        db.execDML(sql);
        db.close();
    } catch (CppSQLite3Exception e) {
    	printf("ERR_DB [%s] %s\n", UPL_CONFIG_DB, e.errorMessage());
        return ERR_DB;
    }
    return OK;
}

static int retrieve_cal_param(long value)
{
    CalParam *param = (CalParam*)value;
    CppSQLite3DB db;
    CppSQLite3Buffer sql_query;
    CppSQLite3Query query;

    try {
        db.open(UPL_CONFIG_DB);
        sql_query.format("SELECT cal_times, cal_interval, grade FROM camera_config");
	query = db.execQuery(sql_query);
	if(!query.eof()) {
	    param->times = query.getIntField("cal_times");
	    param->interval = query.getIntField("cal_interval");
	    param->level = query.getIntField("grade");
	}
        query.finalize();
        db.close();
    } catch (CppSQLite3Exception e) {
    	printf("ERR_DB [%s] %s\n", UPL_CONFIG_DB, e.errorMessage());
        return ERR_DB;
    }
    return OK;
}

// Save template raw data to db
int init_target()
{
    list<Template>::iterator it;
    int frame_size, temp_size;
    CppSQLite3DB db;
    CppSQLite3Buffer sql_insert, sql_update; 
    CppSQLite3Binary  blob;

    frame_size = read_frame();

    for (it = temp_list.begin(); it != temp_list.end(); ++it) {
        printf("Save template data for template id: %d\n", (*it).getId());
        (*it).saveData(global_frame.data, global_frame.width);
        //(*it).saveImage();
        //(*it).dump();

        try {
            db.open(UPL_CONFIG_DB);
            temp_size = (*it).getWidth() * (*it).getHeight();
            blob.setBinary((*it).getData(), temp_size);
            sql_insert.format("UPDATE template_config SET data=%Q WHERE id=%d", blob.getEncoded(), (*it).getId());
            db.execDML(sql_insert);
            db.close();
        } catch (CppSQLite3Exception e) {
            printf("ERR_DB [%s] %s\n", UPL_STATE_DB, e.errorMessage());
            return ERR_DB;
        }
    }
    return OK;
}

static int start_calculating(bool is_resumed)
{
    list<Template>::iterator it;
    int ret;

    if (STATE_STARTED == current_state) {
    	return ERR_CALCULATING;
    } else {
    	current_state = STATE_STARTED;
    }

    sqlite3 *config_db = NULL;
    sqlite3_open(UPL_CONFIG_DB, &config_db);
    sqlite3_close(config_db);

    CalParam param;
    if ((ret = retrieve_cal_param(long(&param))) != OK) {
        return ret;
    }

    if (!is_resumed) {
    	if ((ret = init_target()) != OK) {
            return ret;
	}
    }

    printf("Start calculating: times[%d], interval[%d], level[%d]", param.times, param.interval, param.level);
    calctl_thread = new CalControl();
    calctl_thread->init(param.times, param.interval, param.level);
    calctl_thread->start();

    for (it = temp_list.begin(); it != temp_list.end(); ++it) {
        Calculation *cal_thread = new Calculation();
        cal_list.push_back(cal_thread);
        cal_thread->init(&(*it));
        cal_thread->start();
    }

    state_change(STATE_STARTED);

    return OK;
}

static int calculate_once(long value)
{
    CalResults **result_ptr;
    list<Template>::iterator it;
    CalResult cal_result;
    int frame_size;
    char buf[128] = {0};
    bool ret;

    if (STATE_STARTED == current_state) {
    	return ERR_CALCULATING;
    }

    frame_size = read_frame();
    cal_results.ts = global_frame.ts;
    cal_results.data.clear();

    tm* local;
    local = localtime(&global_frame.ts.tv_sec);
    strftime(buf, 128, "%Y-%m-%d %H:%M:%S", local);

    printf("Calculate once on: %s.%3d\n", buf, global_frame.ts.tv_usec/1000);

    for (it = temp_list.begin(); it != temp_list.end(); ++it) {
        cal_result.tempId = (*it).getId();
	if (NULL == (*it).getData()) {
		return ERR_UNINITED;
	}
        ret = CGauger::MatchTemplate(global_frame.data, global_frame.width, global_frame.height, &(*it), 0.8, &cal_result.offsetX, &cal_result.offsetY);

        if (!ret) {
            cal_result.offsetX = cal_result.offsetY = 1.0/0.0;
	}
	printf("ID: %d, offsetx: %f, offsety: %f\n", cal_result.tempId, cal_result.offsetX, cal_result.offsetY);
	cal_results.data.push_back(cal_result);
    }

    result_ptr = (CalResults **)value;
    *result_ptr = &cal_results;

    return OK;
}

/******************************************************************************
Description.: 
Input Value.: -
Return Value: always 0
******************************************************************************/
int output_run(int id)
{
    global_frame.width = 0;
    global_frame.height = 0;
    global_frame.ts.tv_sec = 0;
    global_frame.ts.tv_usec = 0;
    global_frame.data = NULL;

    // Load all templates from database
    CppSQLite3DB db;
    CppSQLite3Buffer sql_query;
    CppSQLite3Query query;
    CppSQLite3Binary  blob;

    init_cir_queue(&data_queue);

    temp_list.clear();

    try {
        db.open(UPL_CONFIG_DB);
        sql_query.format("SELECT id, coordinate_x, coordinate_y, height, width, search_range, sigma, is_reference, is_watched FROM template_config");
	query = db.execQuery(sql_query);
	while (!query.eof()) {
	    Template temp;
	    temp.setId(query.getIntField("id"));
	    temp.setCoordinateX(query.getIntField("coordinate_x"));
	    temp.setCoordinateY(query.getIntField("coordinate_y"));
	    temp.setHeight(query.getIntField("height"));
	    temp.setWidth(query.getIntField("width"));
	    temp.setSearchRange(query.getIntField("search_range"));
            //temp.mmPerPix = query.getFloatField("mm_per_pix");
            temp.setSigma(query.getFloatField("sigma"));
            temp.setIsReference(query.getFloatField("is_reference"));
            temp.setIsWatched(query.getFloatField("is_watched"));
            temp_list.push_back(temp);
	    query.nextRow();
	}
	query.finalize();

        list<Template>::iterator temp_it;
        for (temp_it = temp_list.begin(); temp_it != temp_list.end(); ++temp_it) {
            sql_query.format("SELECT data FROM template_config WHERE id=%d", (*temp_it).getId());
	    query = db.execQuery(sql_query);
	    if (!query.eof() && NULL != query.fieldValue("data")) {
                blob.setEncoded((unsigned char*)query.fieldValue("data"));
                (*temp_it).saveData(blob.getBinary());
            }
	    query.finalize();
        }
        db.close();
    } catch (CppSQLite3Exception e) {
    	printf("ERR_DB [%s] %s\n", UPL_CONFIG_DB, e.errorMessage());
        current_state = STATE_STOPPED;
        return ERR_DB;
    }

    DBG("launching data I/O thread\n");
    pthread_create(&data_io, 0, data_io_thread, NULL);
    pthread_detach(data_io);

    return OK;
}

static void stop_calculating()
{
    if (STATE_STOPPED == current_state) {
        return;
    }

    if (calctl_thread) {
        calctl_thread->stop();
	while(!calctl_thread->isExited());
	delete calctl_thread;
    }

    cleanup_cal_list();
}

static int create_template(long value)
{
    CppSQLite3DB db;
    Template *temp;

    temp = (Template*)value;

    CppSQLite3Buffer sql_insert, sql_query;
    sql_insert.format("INSERT INTO template_config (coordinate_x, coordinate_y) VALUES (%d, %d)", temp->getCoordinateX(), temp->getCoordinateY());
    sql_query.format("SELECT id FROM template_config WHERE coordinate_x = %d AND coordinate_y = %d", temp->getCoordinateX(), temp->getCoordinateY());

    try {
        db.open(UPL_CONFIG_DB);
        db.execDML(sql_insert);

	CppSQLite3Query query = db.execQuery(sql_query);
	if (!query.eof()) {
	    temp->setId(query.getIntField("id"));
    	    printf("Saved template to db.  id: %d, x: %d, y: %d\n", temp->getId(), temp->getCoordinateX(), temp->getCoordinateY());
	}
	query.finalize();
        db.close();
    } catch (CppSQLite3Exception e) {
    	printf("ERR_DB [%s] %s\n", UPL_CONFIG_DB, e.errorMessage());
	return ERR_DB;
    }

    temp_list.push_back(*temp);
    return OK;
}

static int update_template(long value)
{
    CppSQLite3DB db;
    CppSQLite3Buffer sql_update;
    Template* saved_temp = NULL;
    Template* new_temp = (Template*)value;
    bool found = false;

    list<Template>::iterator temp_it;
    for (temp_it = temp_list.begin(); temp_it != temp_list.end();temp_it++) {
        if ((*temp_it).getId() == new_temp->getId()) {
            saved_temp = &(*temp_it);
	    found = true;
            break;
        }
    }

    if (false == found) {
        return ERR_INVALID_TEMP_ID;
    }

    sql_update.format("UPDATE template_config SET name='%s', coordinate_x=%d, coordinate_y=%d, mm_per_pix=%f, is_reference=%d, is_watched=%d WHERE id=%d",
                      new_temp->getName().c_str(), new_temp->getCoordinateX(), new_temp->getCoordinateY(), new_temp->getMmPerPix(),
                      new_temp->getIsReference(), new_temp->getIsWatched(), new_temp->getId());

    try {
        db.open(UPL_CONFIG_DB);
        db.execDML(sql_update);
        db.close();
    } catch (CppSQLite3Exception e) {
    	printf("ERR_DB [%s] %s\n", UPL_CONFIG_DB, e.errorMessage());
	return ERR_DB;
    }

    *saved_temp = *new_temp;
    return OK;
}

static int delete_template(long value)
{
    CppSQLite3DB db;

    CppSQLite3Buffer sql_delete;
    // 0 for delete all
    if (value == 0) {
        sql_delete.format("DELETE FROM template_config");
    } else {
        sql_delete.format("DELETE FROM template_config WHERE id = %d", value);
    }

    try {
        db.open(UPL_CONFIG_DB);
        db.execDML(sql_delete);
        db.close();
    } catch (CppSQLite3Exception e) {
    	printf("ERR_DB [%s] %s\n", UPL_CONFIG_DB, e.errorMessage());
	return ERR_DB;
    }

    if (0 == value) {
        temp_list.clear();
    } else {
        list<Template>::iterator temp_it;
        for (temp_it = temp_list.begin(); temp_it != temp_list.end();) {
            if (value == (*temp_it).getId()) {
                temp_it = temp_list.erase(temp_it);
            } else {
                temp_it++;
            }
        }
    }

    return OK;
}

static int retrieve_template_by_id(long value)
{
    Template *temp = (Template*)value;
    list<Template>::iterator temp_it;
    for (temp_it = temp_list.begin(); temp_it != temp_list.end(); temp_it++) {
        if (temp->getId() == (*temp_it).getId()) {
            *temp = *temp_it;
            return OK;
        }
    }
    return ERR_INVALID_TEMP_ID;
}

static int retrieve_templates(long value)
{
    list<Template> **temp_list_ptr;
    temp_list_ptr = (list<Template> **)value;
    *temp_list_ptr = &temp_list;
    return OK;
}

void* download_thread(void *param)
{
    int pipe_fd = -1;
    int res = 0;
    int nread = 0;
    CppSQLite3DB db;
    CppSQLite3Buffer sql_query;
    CppSQLite3Query query;
    char buf[128];

    printf("Start downloading thread...\n");
    if ((pipe_fd = open(UPL_FIFO, O_WRONLY)) < 0) {
        printf("Failed to open fifo\n");
        return NULL;
    }

    try {
        db.open(UPL_DATA_DB);
        sql_query.format("SELECT id, template_id, x, y, offsetx, offsety, timestamp FROM data");
	query = db.execQuery(sql_query);
	while(!query.eof()) {
            memset(buf, 0, 128);
            nread = sprintf(buf, "%d,%d,%f,%f,%f,%f,%s\n", query.getIntField("id"), query.getIntField("template_id"),
                                                           query.getFloatField("x"),  query.getFloatField("y"),
                                                           query.getFloatField("offsetx"), query.getFloatField("offsety"),
                                                           query.getStringField("timestamp"));

            if (nread > 0) {
                res = write(pipe_fd, buf, nread);
                if(res == -1)
                {
                    printf("Failed to write fifo\n");
                    return NULL;
                }
            }
	    query.nextRow();
	}
        query.finalize();
        db.close();
    } catch (CppSQLite3Exception e) {
    	printf("ERR_DB [%s] %s\n", UPL_CONFIG_DB, e.errorMessage());
        return NULL;
    }

    close(pipe_fd);
    printf("Downloading thread exit\n");
    return NULL;
}

int start_downloading()
{
    if(access(UPL_FIFO, F_OK) == -1)
    {
        if (0 != mkfifo(UPL_FIFO, 0777)) {
            printf("Failed to create fifo\n");
            return ERR_FIFO;
        }
    }

    pthread_create(&download, 0, download_thread, NULL);
    pthread_detach(download);

    return OK;
}

int output_cmd(int plugin, unsigned int control_id, unsigned int group, long value)
{
    printf("command (%d, value: %ld) for group %d triggered for plugin instance #%02d\n", control_id, value, group, plugin);

    switch(control_id){
        case CMD_UNREGISTER: // 0
            printf("CMD: CMD_UNREGISTER\n");
            gauger_observers.remove(value);
            break;

        case CMD_REGISTER: // 1
            printf("CMD: CMD_REGISTER\n");
            gauger_observers.push_back(value);
            break;

        case CMD_START: // 2
            printf("CMD: CMD_START\n");
            return start_calculating(false);

        case CMD_STOP: // 3
            printf("CMD: CMD_STOP\n");
            stop_calculating();
	    return OK;

        case CMD_RESUME: // 4
            printf("CMD: CMD_RESUME\n");
            return start_calculating(true);

        case CMD_CAL: // 5
            printf("CMD: CMD_CAL\n");
            return calculate_once(value);

        case CMD_TARGET: // 6
            printf("CMD: CMD_TARGET\n");
            return init_target();

        case CMD_STATE: { // 7
            printf("CMD: CMD_STATE\n");
            int *state = (int*)value;
            *state = current_state;
            return OK;
        }

        case CMD_TEMP_CREATE: // 8
            printf("CMD: CMD_TEMP_CREATE\n");
            return create_template(value);

        case CMD_TEMP_UPDATE: // 9
            printf("CMD: CMD_TEMP_UPDATE\n");
            return update_template(value);

        case CMD_TEMP_DELETE: // 10
            printf("CMD: CMD_TEMP_DELETE\n");
            return delete_template(value);

        case CMD_TEMP_DELETE_ALL: // 11
            printf("CMD: CMD_TEMP_DELETE_ALL\n");
            return delete_template(0);

        case CMD_TEMP_RETRIEVE: // 12
            printf("CMD: CMD_TEMP_RETRIEVE\n");
            return retrieve_templates(value);

        case CMD_TEMP_RETRIEVE_BY_ID:
            printf("CMD: CMD_TEMP_RETRIEVE_BY_ID\n");
            return retrieve_template_by_id(value);

        case CMD_CAL_PARAM_UPDATE:
            printf("CMD: CMD_CAL_PARAM_UPDATE\n");
            return update_cal_param(value);

        case CMD_CAL_PARAM_RETRIEVE:
            printf("CMD: CMD_CAL_PARAM_RETRIEVE\n");
            return retrieve_cal_param(value);

        case CMD_DOWNLOAD:
            printf("CMD: CMD_DOWNLOAD\n");
            return start_downloading();

        default:
            printf("command not support!\n");
	    return ERR_INVALID_CMD;
    }

    return OK;
}


#ifdef __cplusplus
};
#endif
