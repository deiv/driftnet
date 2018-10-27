/*
 * uid.c:
 * User and group handling.
 *
 * Copyright (c) 2015 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <pwd.h>
#include <sys/types.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "common/log.h"

#include "uid.h"

/*
 * Drop root privileges.
 */
void drop_root(const char *username)
{
    struct passwd *pw = NULL;

    pw = getpwnam(username);

    if (!pw) {
        log_msg(LOG_ERROR, "the specified user %s, not exists", username);
        abort(); /* TODO: exit ¿? */
    }

    if (initgroups(pw->pw_name, pw->pw_gid) != 0
          || setgid(pw->pw_gid) != 0
          || setuid(pw->pw_uid) != 0) {

        log_msg(LOG_ERROR, "couldn't drop privileges to '%.32s' uid=%lu gid=%lu: %s",
            username,
            (unsigned long)pw->pw_uid,
            (unsigned long)pw->pw_gid,
            strerror(errno));
        abort(); /* TODO: exit ¿? */

    } else {
        log_msg(LOG_INFO, "dropped privileges to user %s", username);
    }
}
