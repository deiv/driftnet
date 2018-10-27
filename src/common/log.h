/**
 * @file log.h
 *
 * @brief Logging functions.
 * @author David Suárez
 * @date Sun, 21 Oct 2018 18:41:11 +0200
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __LOG_H__
#define __LOG_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

/**
 * @brief List of supported log levels
 */
typedef enum {
    /** No level at all, prints unformated text to screen */
    LOG_SIMPLY = 0,

    /** Error level */
    LOG_ERROR,

    /** Warning level */
    LOG_WARNING,

    /** Info level */
    LOG_INFO
} loglevel_t;

/**
 * @brief Gets the current log level.
 *
 * @return current log level
 */
loglevel_t get_loglevel(void);

/**
 * @brief Set/Changes the current log level.
 *
 * @param level the level to set
 */

void set_loglevel(loglevel_t level);

/**
 * @brief Logs a new message.
 *
 * @param level level of the message
 * @param fmt format string
 * @param ... parameters passed to the format string
 */
void log_msg(loglevel_t level, const char *fmt, ...);

#endif /* __LOG_H__ */
