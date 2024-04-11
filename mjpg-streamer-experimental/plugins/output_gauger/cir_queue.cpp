#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "cir_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

sem_t queue_sem;

void init_cir_queue(cir_queue_t *q)
{    
	int res;
	
	/* Create semaphore */
	res = sem_init(&queue_sem, 0, QUE_SIZE);
	if (res != 0)
	{
	  perror("Semaphore init failed.\n");
	  exit(EXIT_FAILURE);
	}
	memset(q->data, 0, QUE_SIZE*sizeof(CalData));
	
	q->front = q->rear = 0;
	q->count = 0;
}

int is_empty_cir_queue(cir_queue_t *q)
{
	int empty_flag;
	
	sem_wait(&queue_sem);    
	
	empty_flag = q->front == q->rear;
	
	sem_post(&queue_sem);
	
	return empty_flag;
}

int is_full_cir_queue(cir_queue_t *q)
{
	int full_flag;
	
	sem_wait(&queue_sem);    
	
	full_flag = (q->rear == ((QUE_SIZE - 1 + q->front) % QUE_SIZE));
	
	sem_post(&queue_sem);
	
	return full_flag;
}

bool push_cir_queue(cir_queue_t *q, CalData data)
{
	if (is_full_cir_queue(q))
	{
	  printf("queue overflow.\n");
	  return false;
	}
	
	sem_wait(&queue_sem);    
	
	q->count++;
	q->data[q->rear] = data;
	q->rear = (q->rear+1) % QUE_SIZE;
	
	sem_post(&queue_sem);
	return true;
}

bool pop_cir_queue(cir_queue_t *q, CalData *data)
{
	if (is_empty_cir_queue(q))
	{
	  //printf("queue empty.\n");
	  return false;
	}
	
	sem_wait(&queue_sem);    
	
	*data = q->data[q->front];
	q->count--;
	q->front = (q->front+1) % QUE_SIZE;
	
	sem_post(&queue_sem);
	
	return true;
}

bool top_cir_queue(cir_queue_t *q, CalData *data)
{
	if (is_empty_cir_queue(q))
	{
	  printf("queue is empty.\n");
	  return false;
	}
	
	sem_wait(&queue_sem);    
	
	*data = q->data[q->front];
	
	sem_post(&queue_sem);
	
	return true;
}

void destroy_cir_queue(cir_queue_t *q)
{
	sem_destroy(&queue_sem);
	
	return;    
}

void print_queue(cir_queue_t* q)
{
	int index;
	if (is_empty_cir_queue(q))
	{
	  printf("queue is empty.\n");
	  return;
	}
	
	sem_wait(&queue_sem);    
	printf("QUEUE: ");
	for (index = 0; index < QUE_SIZE; index++)
	{
	  printf(" %s ", q->data[index].ts.c_str());
	}
	printf("\n");
	
	sem_post(&queue_sem);
	
	return;
}

#ifdef __cplusplus
}
#endif

