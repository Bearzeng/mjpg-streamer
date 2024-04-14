#include <time.h>
#include <stdio.h>
#define HAVE_PTHREADS
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
#include <libgen.h>
#include <string.h>


#define LOG_TAG	"LOG"
#include "log.h"

#define LOG_BUF_SIZE		824
#define MAX_LOG_BUF_SIZE	1024
#define MAX_LOG_FILE_SIZE	10*1024*1024
#define DATE_STR		30

static const char* prio_string[] = {
	"UNKNOW",
	"FATAL",
	"ERROR",
	"WARN",
	"INFO",
	"DEBUG",
	"VERBOSE",
};

static unsigned int ugen_log_toggle = 0;
static const char* mod_string[] = {
	"UNKNOW",
	"LOGIC",
	"BASE",
	"DB",
	"IPC",
	"CONFIG",
	"FE",
	"MODULE",
	"PROTOBUF",
	"BE",
};


static char real_log_file_name[MAX_LOG_BUF_SIZE];

static int __write_to_log_console(char* file, char* func, int line, int prio, const char* tag, const char* msg, int len);
static int (*write_to_log)(char* file, char* func, int line, int prio, const char* tag, const char* msg, int len) =
    __write_to_log_console;

static unsigned int ugen_log_level = UGEN_LOG_DEBUG;
void set_log_level(unsigned int level)
{
	ugen_log_level = level;
}

unsigned int get_log_level()
{
	return ugen_log_level;
}

static bool ugen_log_debug_info = true;
void set_debug_info(unsigned int debug)
{
	ugen_log_debug_info = ((debug == 1) ? true: false);
}

static bool ugen_log_print_std = true;
void set_log_print_std(bool val)
{
	ugen_log_print_std = val;
}

static int fd = 0;
int get_ugen_log_fd()
{
	return fd;
}

void set_ugen_log_fd(int id)
{
	fd = id;
}

static bool ugen_log_init = false;
void set_ugen_log_init(bool init)
{
	ugen_log_init = init;
}

static unsigned int ugen_log_file_cnt = 0;

static void get_time_str(char* buf, int len, int flag)
{
	time_t  tt;
	if (NULL == buf)
		return;

	memset(buf, 0, len);
	tt = time(0);
	strftime(buf, len, "%Y-%m-%d_%H:%M:%S", localtime(&tt));
	/*CDateTime date;


	memset(buf, 0, len);

	if (0 == flag)
		snprintf(buf, len, "%s", date.GetDetail().c_str());
	else
		snprintf(buf, len, "%s", date.GetShortDetail().c_str());
		*/
}

void set_log_file(bool save_old)
{
	char	new_name[LOG_BUF_SIZE];
	char	date[DATE_STR];
	time_t  tt;

	if (save_old) {
		memset(new_name, 0, LOG_BUF_SIZE);
		memset(date, 0, DATE_STR);
		get_time_str(date, DATE_STR, 0);
		snprintf(new_name, LOG_BUF_SIZE, "%s_%s", real_log_file_name, date);
		if (0 == access(real_log_file_name, 0))
			if (rename(real_log_file_name, new_name) != 0)
				printf("Fail to rename %s to %s.\n", real_log_file_name, new_name);
	}

#ifdef ANDROID
	if ((fd = open(real_log_file_name, O_WRONLY | O_CREAT)) == -1) {
#else
	if ((fd = open(real_log_file_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
#endif
		printf("Fail to open log file %s\n", real_log_file_name);
		return;
	}
}

int init_log(const char* program_name, const char* log_file, unsigned int toggle, unsigned int level, unsigned int debug_info)
{
	if (ugen_log_init) {
		printf("log module is initialized\n");
		return 0;
	}

	if (level > UGEN_LOG_VERBOSE || level < UGEN_LOG_FATAL) {
		printf("Log level exceed its range, min:%d, max:%d\n", UGEN_LOG_FATAL, UGEN_LOG_VERBOSE);
		return -1;
	} else
		ugen_log_level = level;

	if (1 == debug_info) {
		ugen_log_debug_info = true;
	} else if (0 == debug_info) {
		ugen_log_debug_info = false;
	} else {
		printf("Log debug info only accepts '1' or '0', it means 'true' or 'false'.\n");
		return -1;
	}

	if (toggle > sizeof(mod_string)/sizeof(char*)) {
		printf("Log module toggle exceed its rang, max:%lu\n", (unsigned long)(sizeof(mod_string)/sizeof(char*)));
		return -1;
	}
	ugen_log_toggle = toggle;

	if (NULL == log_file) {
		printf("Log file path should not be null!\n");
		return -1;
	}

	memset(real_log_file_name, 0, LOG_BUF_SIZE);
	snprintf(real_log_file_name, LOG_BUF_SIZE, "%s", log_file);
	if (strncmp(CMD_LOG_NAME, program_name, strlen(CMD_LOG_NAME)) == 0)
		set_log_file(false);
	else
		set_log_file(true);

	ugen_log_init = true;

	return 0;
}

void fini_log()
{
	if (0 != fd)
		close(fd);

	fd = 0;
}

static inline int log_write(char* file, char* func, int line, int prio, const char* tag, const char* msg, int len)
{
	return write_to_log(file, func, line, prio, tag, msg, len);
}

//static Mutex logM;
int log_print(char* file, char* func, int line, int prio, const char* tag, const char* fmt, ...)
{
	int	ret, total = 1024, len = 0;
	va_list	ap;
	char*	buf = NULL;

	//logM.lock();
	while (total > len) {
		if (NULL != buf)
			free(buf);

		len += 1024;
		buf = (char*)malloc(len);
		if (NULL == buf)
			return -1;
		memset(buf, 0, len);
		va_start(ap, fmt);
		total = vsnprintf(buf, len, fmt, ap);
		va_end(ap);
	}

	ret = log_write(file, func, line, prio, tag, buf, total);
	free(buf);
	//logM.unlock();
	return ret;
}

int log_vprint(char* file, char* func, int line, int prio, const char* tag, const char* fmt, va_list ap)
{
	int	ret, total, len = 1024;
	char*	buf = NULL;

	buf = (char*)malloc(len);
	if (NULL == buf)
		return -1;
	memset(buf, 0, len);
	total = vsnprintf(buf, len, fmt, ap);

	ret = log_write(file, func, line, prio, tag, buf, total);
	free(buf);
	return ret;
}

static int write_to_file(char* buf, int len)
{
	char*	ptr = buf;
	int	bytes_write = 0;

	if (fd <= 0)
		return 0;

	while ((bytes_write = write(fd, ptr, len)) != 0) {
		if ((bytes_write == -1) && (errno != EINTR))
			break;
		else if (bytes_write == len)
			break;
		else if (bytes_write > 0) {
			ptr += bytes_write;
			len -= bytes_write;
		}
	}

	if (bytes_write == -1) {
		printf("Failed to write buf to log file, buf:%s, len:%d.\n", buf, len);
		return -2;
	}

	ugen_log_file_cnt += bytes_write;
	if (ugen_log_file_cnt >= MAX_LOG_FILE_SIZE) {
		ugen_log_file_cnt = 0;
		close(fd);
		set_log_file(true);
	}

	return 0;
}

static int __write_to_log_console(char* file, char* func, int line, int prio, const char* tag, const char* msg, int len)
{
	int	ret = 0, buf_len = len + 512, total;
	char	*buf;
	char	date[DATE_STR];

	if (ugen_log_toggle != 0
	    && strncmp(tag, mod_string[ugen_log_toggle], strlen(tag)) != 0)
		return ret;

	if ((unsigned int)prio <= ugen_log_level) {
		buf = (char*)malloc(buf_len);
		if (NULL == buf)
			return -1;
		memset(buf, 0, buf_len);
		memset(date, 0, DATE_STR);
		get_time_str(date, DATE_STR, 1);
		if (ugen_log_print_std) {
			fprintf(stderr, "[%s][%s] %s\n", date, prio_string[prio], msg);
		}
		memset(date, 0, DATE_STR);
		get_time_str(date, DATE_STR, 0);
		if (ugen_log_debug_info) {
			total = snprintf(buf, buf_len, "%s [file:%s, func:%s, line:%d] [%s] %s\n", date, basename(file), func, line, prio_string[prio], msg);
		} else {
			total = snprintf(buf, buf_len, "%s [%s] %s\n", date, prio_string[prio], msg);
		}
		ret = write_to_file(buf, total);
		free(buf);
	}

	return ret;
}
