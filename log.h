/*
 * log.h:
 * Logging functions.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef __LOG_H__
#define __LOG_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

typedef enum {LOG_ERROR = 0, LOG_WARNING, LOG_INFO} loglevel_t;

loglevel_t get_loglevel(void);
void set_loglevel(loglevel_t level);

void log_msg(loglevel_t level, const char *fmt, ...);

#endif /* __LOG_H__ */ 
