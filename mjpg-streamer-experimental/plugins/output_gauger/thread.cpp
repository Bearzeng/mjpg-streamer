#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "thread.h"

void* thread_func(void* param)
{
	Thread* thread = (Thread*)param;
	thread->state = Thread::RUNNING;
	thread->run();
	return 0;
}

Thread::Thread()
{
	tid = 0;
	state = STOP;
	pthread_cond_init(&cond, NULL);
	pthread_mutex_init(&mutex, NULL);
}

Thread::~Thread()
{
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}

void Thread::start()
{
	if (state == RUNNING)
		return;

	// create a thread with function
	pthread_create(&tid, NULL, thread_func, (void*)this);
	printf("thread(%lu) start state=%d\n", (unsigned long)tid, state);
}

void Thread::wait()
{
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond, &mutex);
	pthread_mutex_unlock(&mutex);
}

int Thread::wait_timeout(int secs)
{
	int retCode;
	struct timeval	now;
	struct timespec	timeout;

	gettimeofday(&now, NULL);
	timeout.tv_sec = now.tv_sec + secs;
	timeout.tv_nsec = now.tv_usec * 1000;
	pthread_mutex_lock(&mutex);
	retCode = pthread_cond_timedwait(&cond, &mutex, &timeout);
	pthread_mutex_unlock(&mutex);
	return retCode;
}

void Thread::resume()
{
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

void Thread::run()
{
}

void Thread::stop()
{
	printf("thread(%lu) stop() state=%d\n", (unsigned long)tid, state);
	state = STOP;
	//pthread_exit(0);
	//pthread_cancel(tid);
}

void Thread::join(long millis)
{
	//wait for the running thread to exit
	void *result;
	printf("thread(%lu) join() state=%d\n", (unsigned long)tid, state);

	if (pthread_join(tid, &result) == 0) {
		return;
	}
}

bool Thread::isRunning()
{
	return (state == RUNNING) ? true : false;
}

bool Thread::isExited()
{
	return (state == EXITED) ? true : false;
}

bool Thread::isWaiting()
{
	return (state == WAITING) ? true : false;
}
