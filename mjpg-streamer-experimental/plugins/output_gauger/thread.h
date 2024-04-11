#ifndef UGEN_THREAD_H
#define UGEN_THREAD_H

#include <pthread.h>

class Runnable
{
public:
	Runnable() {};
	virtual ~Runnable() {};
	virtual void run() = 0;
};

//
// The thread implementation.
//
class Thread
{
public:
	enum State {
		START,
		RUNNING,
		WAITING,
		STOP,
		EXITED,
		DEAD
	};

	Thread();
	virtual ~Thread();

	// Start the thread.
	void start();

	// Override method for your thread.
	virtual void run();

	// Stop the thread
	void stop();

	void exited() { state = EXITED; };

	// sleep the thread
	void wait();

	int wait_timeout(int secs);

	// resume the thread
	void resume();

	// Wait at most millis for this thread to terminate.
	void join(long millis = 0);

	// Check thread if running
	bool isRunning();
	bool isExited();
	bool isWaiting();

	// Get the id of this thread.
	pthread_t getThreadId() const {
		return tid;
	}

private:
	// copy constructor/assignment operator.
	Thread(const Thread&);
	void operator=(const Thread&);

	// Thread function
	friend void *thread_func(void *);

protected:
	// The thread state.
	State state;

private:
	// Threads id
	pthread_t tid;

	// Thread lock
	pthread_mutex_t mutex;

	// condition var.
	pthread_cond_t cond;
};

#endif /*UGEN_THREAD_H*/

