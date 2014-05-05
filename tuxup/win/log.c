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

/* $Id: log.c 1866 2008-09-17 10:24:55Z Paul_R $ */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

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

/** Current logging target */
static log_target_t log_target = LOG_TARGET_SHELL;

/** Log file for target LOG_TARGET_TUX */
static FILE *log_file;

/** Whether the log has been opened */
static bool log_opened;

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
