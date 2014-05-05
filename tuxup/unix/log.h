#ifndef __USBDAEMON_LOG_H__
#define __USBDAEMON_LOG_H__

#include <stdbool.h>

/** All logged messages are prefixed with this text */
#define LOG_PREFIX  "tuxup"

/** Name of the log file, without extension */
#define LOG_FILE_NAME  "tuxup_prod"

/** Logging target */
typedef enum log_target
{
    LOG_TARGET_SYSLOG,
    LOG_TARGET_TUX,
    LOG_TARGET_SHELL
} log_target_t;

extern bool log_open(log_target_t target);
extern void log_close(void);

/** Logging levels, in increasing priorities */
typedef enum log_level
{
    LOG_LEVEL_DEBUG, /**> Show everything including debug messages */
    LOG_LEVEL_INFO, /**> Show verbose information */
    LOG_LEVEL_NOTICE, /**> Show normal user messages */
    LOG_LEVEL_WARNING, /**> Show warnings and errors */
    LOG_LEVEL_ERROR, /**> Show errors only */
    LOG_LEVEL_NONE /**> Quiet mode, show nothing */
} log_level_t;

extern void log_set_level(log_level_t new_level);
extern log_level_t log_get_level(void);

extern bool log_text(log_level_t at_level, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#define log_debug(fmt, ...)  log_text(LOG_LEVEL_DEBUG, (fmt), ## __VA_ARGS__)
#define log_info(fmt, ...)  log_text(LOG_LEVEL_INFO, (fmt), ## __VA_ARGS__)
#define log_notice(fmt, ...)  log_text(LOG_LEVEL_NOTICE, (fmt), ## __VA_ARGS__)
#define log_warning(fmt, ...)  log_text(LOG_LEVEL_WARNING, (fmt), ## __VA_ARGS__)
#define log_error(fmt, ...)  log_text(LOG_LEVEL_ERROR, (fmt), ## __VA_ARGS__)

#endif /* __USBDAEMON_LOG_H__ */
