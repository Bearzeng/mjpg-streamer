
#ifndef __CMD_H__
#define __CMD_H__

#include "../output.h"
#include "Gauger.h"

void init_ctrl_param(output *out);

CGauger* get_gauger_instance(void);

/*class CalObj {
public:
        static CalObj *getInstance();
        void setX(double _x) { x = _x; }
        void setY(double _y) { y = _y; }
        void update();
private:
        CalObj();
        virtual ~CalObj();

        double x;
        double y;
        static CalObj *instance;
};*/

#endif
