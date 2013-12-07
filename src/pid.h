/*
 * pid.h
 *
 * Handles the pid file on adjunct mode
 *
 * Copyright (c) 2013 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __PID_H__
#define __PID_H__

void create_pidfile(void);
void close_pidfile(void);

#endif /* __PID_H__ */
