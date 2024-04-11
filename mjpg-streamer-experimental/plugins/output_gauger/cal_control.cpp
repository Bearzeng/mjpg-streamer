#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include "../../mjpg_streamer.h"
#include "../../utils.h"
#include "gauger_observer.h"
#include "calculation.h"
#include "cal_control.h"

extern globals *pglobal;
extern vector<Calculation*> cal_list;
extern int max_frame_size;
extern Frame global_frame;
extern void cleanup_cal_list();
extern void state_change(GaugerState new_state);

CalControl::CalControl()
{
	times = 0;
	interval = 0;
	level = 0;
}

CalControl::~CalControl()
{
}

int CalControl::get_y_data_from_yuyv(unsigned char *des, const unsigned char *src, int size)
{
        int i = 0, j = 0;
        while (i < size) {
                des[j] = src[i];
                j++;
                i += 2;
        }
        return j;
}

int CalControl::read_frame()
{
    int frame_size = 0;
    unsigned char *tmp_framebuffer = NULL;
    char *resolution;
    int input_number = 0;

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
    //memcpy(frame, pglobal->in[input_number].buf_raw, frame_size);
    get_y_data_from_yuyv(global_frame.data, pglobal->in[input_number].buf_raw, pglobal->in[input_number].size_raw);
    resolution = pglobal->in[input_number].param.argv[4];
    sscanf(resolution, "%dx%d", &global_frame.width, &global_frame.height);
    gettimeofday(&global_frame.ts, NULL);

    /* allow others to access the global buffer again */
    pthread_mutex_unlock(&pglobal->in[input_number].db);
    for (int i = 0; i < cal_list.size(); i++) {
        cal_list[i]->resume();
    }

    for (int i = 0; i < cal_list.size(); i++) {
        while(!cal_list[i]->isWaiting());
        //printf("cal_list[%d] pasued\n", i);
    }

    return frame_size;
}

void CalControl::run()
{
	unsigned int actual_times;

	while (RUNNING == state) {
        	if (0 == interval) {
        	    level = 0;
        	    if (0 == times) {
        	        read_frame();
        	        continue;
        	    }
        	    else if (actual_times < times) {
        	        read_frame();
        	        actual_times++;
        	        continue;
        	    }
        	    break;
        	} else {
        	    if (0 == times) {
        	        read_frame();
			for (int i = 0; i < interval * 10; i++) {
			        if (RUNNING != state) {
			                break;
			        }
			        usleep(100 * 1000);
			}
        	        continue;
        	    }
        	    else if (actual_times < times) {
        	        read_frame();
			for (int i = 0; i < interval * 10; i++) {
			        if (RUNNING != state) {
			                break;
			        }
			        usleep(100 * 1000);
			}
        	        actual_times++;
        	        continue;
        	    }
        	    break;
        	}
	}
	if (RUNNING == state) {
		cleanup_cal_list();
	}
	cout << "calctl_thread exit" << endl;
	state_change(STATE_STOPPED);
	exited();
	pthread_exit(0);
}

