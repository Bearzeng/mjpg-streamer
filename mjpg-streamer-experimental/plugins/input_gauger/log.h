#ifndef ___LOG_H
#define ___LOG_H

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOG_TAG
#define LOG_TAG	"UNKNOWN"
#endif

#define CMD_LOG_NAME	"cmd"
#define VST_LOG_NAME	"vst"

enum ugen_LogPriority {
	UGEN_LOG_FATAL = 1,
	UGEN_LOG_ERROR,
	UGEN_LOG_WARN,
	UGEN_LOG_INFO,
	UGEN_LOG_DEBUG,
	UGEN_LOG_VERBOSE,
};

#ifndef SLOGV
#ifdef UGEN_DEBUG
#define SLOGV(fmt, args...)	((void)SLOG(__FILE__, __FUNCTION__, __LINE__, LOG_VERBOSE, LOG_TAG, fmt, ##args))
#else
#define SLOGV(fmt, args...)	((void)0)
#endif
#endif

#ifndef SLOGD
#ifdef UGEN_DEBUG
#define SLOGD(fmt, args...)	((void)SLOG(__FILE__, __FUNCTION__, __LINE__, LOG_DEBUG, LOG_TAG, fmt, ##args))
#else
#define SLOGD(fmt, args...)	((void)0)
#endif
#endif

#ifndef SLOGI
#define SLOGI(fmt, args...)	((void)SLOG(__FILE__, __FUNCTION__, __LINE__, LOG_INFO, LOG_TAG, fmt, ##args))
#endif

#ifndef SLOGW
#define SLOGW(fmt, args...)	((void)SLOG(__FILE__, __FUNCTION__, __LINE__, LOG_WARN, LOG_TAG, fmt, ##args))
#endif

#ifndef SLOGE
#define SLOGE(fmt, args...)	((void)SLOG(__FILE__, __FUNCTION__, __LINE__, LOG_ERROR, LOG_TAG, fmt, ##args))
#endif

#ifndef SLOGF
#define SLOGF(fmt, args...)	((void)SLOG(__FILE__, __FUNCTION__, __LINE__, LOG_FATAL, LOG_TAG, fmt, ##args))
#endif

#ifndef SLOG
#define SLOG(file, func, line, level, tag, fmt, args...)	\
	log_print((char*)file, (char*)func, line, UGEN_##level, tag, fmt, ##args)
#endif

	int log_print(char* file, char* func, int line, int prio, const char* tag, const char* fmt, ...);
	int log_vprint(char* file, char* func, int line, int prio, const char* tag, const char* fmt, va_list ap);
	int init_log(const char* program_name, const char* log_file, unsigned int toggle, unsigned int level, unsigned int debug_info);
	void set_log_level(unsigned int level);
	unsigned int get_log_level();
	void set_debug_info(unsigned int debug);
	void set_log_print_std(bool val);
	void fini_log();
	void set_ugen_log_init(bool init);
	int get_ugen_log_fd();
	void set_ugen_log_fd(int fd);

#ifdef __cplusplus
	}
#endif

#endif
