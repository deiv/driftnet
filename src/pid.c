/*
 * pid.c
 *
 * Handles the pid file on adjunct mode
 *
 * Copyright (c) 2013 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "compat.h"

#ifdef __FreeBSD__
#include <sys/stat.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>
#include <unistd.h>

#include "log.h"

/*
 * XXX: create the pid in tmp. To create on '/var/run' we need proper permisions.
 */
#define PID_FILE "/tmp/driftnet.pid"
#define PID_BUFSIZE 64

static int pidfile_fd = -1;

void create_pidfile(void)
{
    int flags;
    char buf[PID_BUFSIZE];
    struct flock fl;

    pidfile_fd = open(PID_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (pidfile_fd == -1) {
        log_msg(LOG_ERROR, "Could not open/create PID file %s", PID_FILE);
        exit (-1);
    }

	/*
	 * Set the close-on-exec file descriptor flag
	 *
     * Instead of the following steps, we could (on Linux) have opened the
	 * file with O_CLOEXEC flag. However, not all systems support open()
	 * O_CLOEXEC (which was standardized only in SUSv4), so instead we use
	 * fcntl() to set the close-on-exec flag after opening the file
     */
	flags = fcntl(pidfile_fd, F_GETFD);
	if (flags == -1) {
		log_msg(LOG_ERROR, "Could not get flags for PID file %s", PID_FILE);
		exit (-1);
	}

	flags |= FD_CLOEXEC;

	if (fcntl(pidfile_fd, F_SETFD, flags) == -1) {
		log_msg(LOG_ERROR, "Could not set flags for PID file %s", PID_FILE);
		exit (-1);
	}

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if ( fcntl(pidfile_fd, F_SETLK, &fl) == -1) {

        if (errno  == EAGAIN || errno == EACCES) {
        	log_msg(LOG_WARNING, "PID file '%s' is locked; probably the program is already running", PID_FILE);
        	log_msg(LOG_WARNING, "if not, try to remove the file %s", PID_FILE);
            exit (0);

        } else {
        	log_msg(LOG_ERROR, "Unable to lock PID file '%s'", PID_FILE);
        	exit (-1);
        }
    }

    if (ftruncate(pidfile_fd, 0) == -1) {
    	log_msg(LOG_ERROR, "Could not truncate PID file '%s'", PID_FILE);
    	close (pidfile_fd);
    	exit (-1);
    }

    snprintf(buf, PID_BUFSIZE, "%ld\n", (long) getpid());
    if (write(pidfile_fd, buf, strlen(buf)) != strlen(buf)) {
    	log_msg(LOG_ERROR, "writing to PID file '%s'", PID_FILE);
    	close (pidfile_fd);
    	exit (-1);
    }
}

void close_pidfile(void)
{
	if (pidfile_fd > 0) {
		close (pidfile_fd);
		pidfile_fd = -1;

        if (unlink(PID_FILE)) {
            log_msg(LOG_ERROR, "cannot delete pidfile %s: %s", PID_FILE, strerror(errno));
        }
	}
}
