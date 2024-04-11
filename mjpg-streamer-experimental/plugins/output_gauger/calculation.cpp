#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iterator>
#include <math.h>
#include <time.h>
#include "../../mjpg_streamer.h"
#include "../../utils.h"
#include "gauger_observer.h"
#include "calculation.h"
#include "cir_queue.h"

extern globals *pglobal;
extern list<long> gauger_observers;
extern cir_queue_t data_queue;
extern Frame global_frame;
extern pthread_mutex_t frame_mutex;
extern pthread_cond_t frame_cond;

Calculation::Calculation()
{
	temp = NULL;
}

Calculation::~Calculation()
{
}

int Calculation::calculate_once()
{
	float offsetx = 0.0, offsety = 0.0;
	bool ret = false;
	list<long>::iterator it;
	char buf[128] = {0};
	CalData data;
	stringstream ss;

	tm* local;
	local = localtime(&global_frame.ts.tv_sec);
	strftime(buf, 128, "%Y-%m-%d %H:%M:%S", local);
	ss << buf << "." << setfill('0') << setw(3) << global_frame.ts.tv_usec/1000;
	data.ts = ss.str();
	data.temp_id = temp->getId();
	
	if (NULL == global_frame.data || NULL == temp) {
		return OK;
	}
	ret = CGauger::MatchTemplate(global_frame.data, global_frame.width, global_frame.height, temp, 0.8, &data.x, &data.y);
	
	if (!ret) {
		data.x = data.y = 1.0/0.0;
	} else {
		data.offsetx = (data.x - temp->getCoordinateX()) * temp->getMmPerPix();
		data.offsety = (data.y - temp->getCoordinateY()) * temp->getMmPerPix();
		push_cir_queue(&data_queue, data);
	}

	cout << "ID:" << data.temp_id <<  ", x:" << data.x << ", y:" << data.y << ", offsetx:" << data.offsetx << ",\toffsety:" << data.offsety << ",\tts:" << data.ts << endl;
	//print_queue(&data_queue);

	// Inform observers only when the template is watched
	if (!temp->getIsWatched()) {
		return OK;
	}

	for (it = gauger_observers.begin(); it != gauger_observers.end(); it++) {
		(*(((GaugerObserver*)(*it))->du))(temp->getId(), data.x, data.y, offsetx, offsety, &global_frame.ts);
	}
	return OK;
}

void Calculation::run()
{
	while (RUNNING == state) {
		if (RUNNING == state) {
			calculate_once();
		} else {
			break;
		}
		state = WAITING;
		wait();
		if (STOP == state) {
			break;
		} else {
			state = RUNNING;
		}
	}
	exited();
	pthread_exit(0);
}

