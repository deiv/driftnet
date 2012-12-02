/*
 * tmpdir.c:
 * Temporary directory helpers.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "compat.h"

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <assert.h>

#include "util.h"
#include "log.h"

#include "tmpdir.h"

/*
 * 'P_tmpdir' is an XSI (X/Open System Interfaces) extension to POSIX which
 * need not be provided by otherwise conforming implementations.
 */
#ifdef P_tmpdir
	#define DEFAULT_TMPDIR P_tmpdir
#else
	#define DEFAULT_TMPDIR "/tmp"
#endif

#define TEMPFILE_PREFIX "driftnet-"

typedef struct {
    const char *path;
    tmpdir_type_t type;
    int max_files;
} tmpdir_t;

static tmpdir_t tmpdir = {NULL, TMPDIR_USER_OWNED, 0};

static int is_tempfile(const char* p);
static int count_tmpfiles(void);

void set_tmpdir(const char *dir, tmpdir_type_t type, int max_files)
{
    assert (tmpdir.path == NULL);    /* we only be called once */
    assert (dir != NULL);

    tmpdir.path      = dir;
    tmpdir.type      = type;
    tmpdir.max_files = max_files;

    log_msg(LOG_INFO, "using temporary file directory %s", tmpdir.path);
}

inline const char* get_tmpdir(void)
{
    assert (tmpdir.path != NULL);

    return tmpdir.path;
}

const char* make_tmpdir(void)
{
    char *systmp;
	char *template;
    int len;
    int n;

    #define TEMPLATE_FILENAME "/drifnet-XXXXXX"

	/* NOTE: don't use TMPDIR if program is SUID or SGID enabled. */
	if (!(systmp = getenv("TMPDIR")))
		if (!(systmp = getenv("TEMP")))
            if (!(systmp = getenv("TMP")))
				systmp = DEFAULT_TMPDIR;

    len  = strlen(systmp);
    len += strlen(TEMPLATE_FILENAME);
    len += 1; /* for null */

	template = xmalloc(len);

    n = snprintf(template, len, "%s"TEMPLATE_FILENAME, systmp);

    if (n > -1 && n < len) {
        /* we have it. */
        char *tmp;

		tmp = mkdtemp(template);

		if (tmp == NULL) {
			xfree(template); /* useless... */
			log_msg(LOG_ERROR, "make_tmpdir(), mkdtemp: %s", strerror(errno));
            exit (-1);
		}

		return tmp;
	}

    if (n < 0) {
        /* we have an error on snprintf */
        log_msg(LOG_ERROR, "make_tmpdir(), snprintf: %s", strerror(errno));

    } else {
        /* logic error... */
        log_msg(LOG_ERROR, "make_tmpdir(), internal error");
    }

    exit (-1);
}

/*
 * clean_tmpdir:
 *   Ensure that our temporary directory is clear of any files.
 */
void clean_tmpdir()
{
    DIR *d;

    assert (tmpdir.path != NULL);

    /*
     * If user_specified is true, the user specified a particular temporary
     * directory. We presume that the user doesn't want the directory removed
     * and that we shouldn't nuke any files in that directory which don't look
     * like ours
     *
     * If not, remove it.
     */
    d = opendir(tmpdir.path);

    if (d) {
        struct dirent *de;
        char *buf;
        size_t buflen;

        buf = xmalloc(buflen = strlen(tmpdir.path) + 64);

        while ((de = readdir(d))) {
            if (is_tempfile(de->d_name)) {
                if (buflen < strlen(tmpdir.path) + strlen(de->d_name) + 1)
                    buf = xrealloc(buf, buflen = strlen(tmpdir.path) + strlen(de->d_name) + 64);

                sprintf(buf, "%s/%s", tmpdir.path, de->d_name);
                unlink(buf);
            }
        }
        closedir(d);

        xfree(buf);
    }

    if (tmpdir.type == TMPDIR_APP_OWNED) {
        if ( rmdir(tmpdir.path) == -1 && errno != ENOENT) /* lame attempt to avoid race */
            log_msg(LOG_ERROR, "rmdir(%s): %s", tmpdir.path, strerror(errno));
    }

    xfree((void*)tmpdir.path);    /* we don't need it anymore */
    tmpdir.path = NULL;
}

int check_dir_is_rw(const char* dir)
{
    struct stat st;

    if (stat(dir, &st) == -1) {
        log_msg(LOG_ERROR, "stat(%s): %s", dir, strerror(errno));
        exit (-1);

    } else if (!S_ISDIR(st.st_mode)) {
        log_msg(LOG_ERROR, "%s: not a directory", dir);
        exit (-1);

    /* access is unsafe but we don't really care. */
    } else if (access(dir, R_OK | W_OK) != 0) {
        log_msg(LOG_ERROR, "%s: %s", dir, strerror(errno));
        exit (-1);
    }

    return 0;
}

const char* tmpfile_write(const char* mname, const unsigned char *data, const size_t len)
{
    static char name[TMPNAMELEN] = {0};
    char *buf;
    int fd;

    buf = xmalloc(strlen(tmpdir.path) + TMPNAMELEN);
    sprintf(name, TEMPFILE_PREFIX"%08x%08x.%s", (unsigned int)time(NULL), rand(), mname);
    sprintf(buf, "%s/%s", tmpdir.path, name);

    fd = open(buf, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        log_msg(LOG_WARNING, "can't open %s for writing", buf);
        return NULL;
    }
    write(fd, data, len);
    close(fd);

    xfree(buf);

    return name;
}

/* count_temporary_files:
 * How many of our files remain in the temporary directory? We do this a
 * maximum of once every five seconds. */
int count_tmpfiles(void)
{
    static int num;
    static time_t last_counted;
    if (last_counted < time(NULL) - 5) {
        DIR *d;
        struct dirent *de;
        num = 0;
        d = opendir(tmpdir.path);
        if (d) {
            while ((de = readdir(d))) {
                if (is_tempfile(de->d_name))
                    ++num;
            }
            closedir(d);
            last_counted = time(NULL);
        }
    }
    return num;
}

static int is_tempfile(const char* file)
{
    assert (file != NULL);

    char *p = strrchr(file, '.');    /* get the file extension */

    /* XXX: get media files extensions from their respect media driver */
    if (p && (strncmp (file, TEMPFILE_PREFIX, 9) == 0) &&
        (strcmp (p, ".jpeg")==0 ||
         strcmp (p, ".gif")==0 ||
         strcmp (p, ".mp3")==0 ||
         strcmp (p, ".png")==0 )) {

        return TRUE;
    }

    return FALSE;
}

int tmpfiles_limit_reached(void)
{
    if (tmpdir.max_files == 0 || count_tmpfiles() < tmpdir.max_files) {
        return FALSE;
    }

    return TRUE;
}

