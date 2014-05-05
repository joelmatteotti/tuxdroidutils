/*
 * Tux Droid - USB Daemon
 * Copyright (C) 2007 C2ME S.A. <tuxdroid@c2me.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/* $Id: log.c 1876 2008-09-17 13:17:12Z Paul_R $ */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <syslog.h>

#include "log.h"

/** Name of log file for target LOG_TARGET_TUX */
#define LOG_FILE  "/var/log/" LOG_FILE_NAME ".log"

/** Current logging level */
static log_level_t current_level = LOG_LEVEL_NOTICE;

/** Logging level names */
static const char *level_names[] =
{
    [LOG_LEVEL_DEBUG]   = "debug",
    [LOG_LEVEL_INFO]    = "info",
    [LOG_LEVEL_NOTICE]  = "notice",
    [LOG_LEVEL_WARNING] = "warning",
    [LOG_LEVEL_ERROR]   = "error",
    [LOG_LEVEL_NONE]    = "none"
};

/** Logging level to syslog priority mapping */
static int level_prio[] =
{
    [LOG_LEVEL_DEBUG]   = LOG_DEBUG,
    [LOG_LEVEL_INFO]    = LOG_INFO,
    [LOG_LEVEL_NOTICE]  = LOG_NOTICE,
    [LOG_LEVEL_WARNING] = LOG_WARNING,
    [LOG_LEVEL_ERROR]   = LOG_ERR,
    [LOG_LEVEL_NONE]    = 0
};

/** Current logging target */
static log_target_t log_target = LOG_TARGET_SHELL;

/** Log file for target LOG_TARGET_TUX */
static FILE *log_file;

/** Whether the log has been opened */
static bool log_opened;

/**
 * Open the log.
 *
 * /param[in] target  Logging target
 *
 * /return true if successfull, false otherwise
 */
bool log_open(log_target_t target)
{
    if (log_opened)
        return true;

    switch (target)
    {
    case LOG_TARGET_SYSLOG:
        openlog(LOG_PREFIX, 0, LOG_DAEMON);
        break;

    case LOG_TARGET_TUX:
        log_file = fopen(LOG_FILE, "at");
        if (log_file == NULL)
            return false;
        break;

    case LOG_TARGET_SHELL:
        break;
    }

    log_target = target;
    log_opened = true;

    return true;
}

/**
 * Close the log.
 */
void log_close(void)
{
    if (!log_opened)
        return;

    switch (log_target)
    {
    case LOG_TARGET_SYSLOG:
        closelog();
        break;

    case LOG_TARGET_TUX:
        fclose(log_file);
        log_file = NULL;
        break;

    case LOG_TARGET_SHELL:
        break;
    }

    log_opened = false;
}

/**
 * Set the logging level.
 *
 * /param[in] new_level  New logging level
 */
void log_set_level(log_level_t new_level)
{
    assert(new_level <= LOG_LEVEL_NONE);
    current_level = new_level;
}

/**
 * Get the logging level.
 *
 * /return current logging level
 */
log_level_t log_get_level(void)
{
    return current_level;
}

/**
 * Log formatted message at the specified level.
 *
 * /param[in] at_level  Level to log the message at
 * /param[in] fmt       Message format
 * /param[in] ...       Optional message data
 *
 * If the priority of the specifed level is lower than the priority
 * of the current logging level, the message is silently dropped.
 *
 * /return true if successful, false otherwise
 */
bool log_text(log_level_t at_level, const char *fmt, ...)
{
    char text[1024], *p = text;
    size_t size = sizeof(text);
    int prio;
    time_t now;
    va_list al;
    int r;

    /* No need for the log to be 'opened' when logging to std{out,err} */
    if (log_target != LOG_TARGET_SHELL && !log_opened)
        return false;

    /* Logging at level NONE has no sense */
    assert(at_level < LOG_LEVEL_NONE);

    if (at_level < current_level)
        return true;

    /* Add date & time when LOG_TARGET_TUX */
    if (log_target == LOG_TARGET_TUX)
    {
        now = time(NULL);
        r = strftime(p, size, "%F %T ", localtime(&now));
        if (r == 0)
            return false;

        p += r;
        size -= r;
    }

    /* Only prefix non-NOTICE level messages */
    if (at_level != LOG_LEVEL_NOTICE)
    {
        r = snprintf(p, size, "%s: ", level_names[at_level]);
        if (r < 0)
            return false;

        p += r;
        size -= r;
    }

    va_start(al, fmt);
    r = vsnprintf(p, size, fmt, al);
    va_end(al);
    if (r < 0)
        return false;

    switch (log_target)
    {
    case LOG_TARGET_SYSLOG:
        prio = level_prio[at_level];
        syslog(prio, "%s", text);
        break;

    case LOG_TARGET_TUX:
        fprintf(log_file, "%s\n", text);
        break;

    case LOG_TARGET_SHELL:
        if (at_level == LOG_LEVEL_WARNING || at_level == LOG_LEVEL_ERROR)
            fprintf(stderr, "%s\n", text);
        else
            fprintf(stdout, "%s\n", text);
        break;
    }

    return true;
}
