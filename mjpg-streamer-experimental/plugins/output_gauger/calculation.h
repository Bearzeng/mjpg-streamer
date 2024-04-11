#ifndef _CALCULATION_H
#define _CALCULATION_H

#include <list>
#include "thread.h"
#include "Gauger.h"

using namespace std;

class Calculation : public Thread
{
public:
        Calculation();
        ~Calculation();

        void init(Template *_temp)
	{
		temp = _temp;
	}

        virtual void run();

private:
	Template *temp;
	//static referred;

	int calculate_once();
};

#endif
