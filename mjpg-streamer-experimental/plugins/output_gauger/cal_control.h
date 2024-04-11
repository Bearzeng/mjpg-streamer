#ifndef _CAL_CONTROL_H
#define _CAL_CONTROL_H

#include "thread.h"

using namespace std;

class CalControl : public Thread
{
public:
        CalControl();
        ~CalControl();

        void init(unsigned int _times,
                  unsigned int _interval,
                  unsigned char _level)
	{
		times = _times;
		interval = _interval;
		level = _level;
	}

        virtual void run();

private:
	unsigned int times;
	unsigned int interval;
	unsigned char level;

	int get_y_data_from_yuyv(unsigned char *des, const unsigned char *src, int size);
	int read_frame();
};

#endif
