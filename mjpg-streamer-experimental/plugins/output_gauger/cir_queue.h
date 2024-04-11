#ifndef __CIR_QUEUE_H__
#define __CIR_QUEUE_H__

#define QUE_SIZE 1024

#include <string>
using namespace std;
//#include "calculation.h"

class CalData {
public:
    CalData() {
        temp_id = -1;
        x = y = offsetx = offsety = 0.0;
        ts = "";
    }
    ~CalData(){}

    CalData& operator=(const CalData &c) {
        temp_id = c.temp_id;
        x = c.x;
        y = c.y;
        offsetx = c.offsetx;
        offsety = c.offsety;
        ts = c.ts;
	return *this;
    }

    int temp_id;
    float x;
    float y;
    float offsetx;
    float offsety;
    string ts;
};

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cir_queue_t
{
  CalData data[QUE_SIZE];
  int front;
  int rear;
  int count;
}cir_queue_t;

void init_cir_queue(cir_queue_t* q);
int is_empty_cir_queue(cir_queue_t* q);
int is_full_cir_queue(cir_queue_t* q);
bool push_cir_queue(cir_queue_t* q, CalData data);
bool pop_cir_queue(cir_queue_t* q, CalData *data);
bool top_cir_queue(cir_queue_t* q, CalData *data);
void destroy_cir_queue(cir_queue_t* q);
void print_queue(cir_queue_t* q);

#ifdef __cplusplus
}
#endif

#endif
