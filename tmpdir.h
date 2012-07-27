/*
 * tmpdir.h:
 * Temporary directory helpers.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __TMPDIR_H__
#define __TMPDIR_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

typedef enum { TMPDIR_APP_OWNED = 0, TMPDIR_USER_OWNED = 1 } tmpdir_type_t;

void set_tmpdir(const char *dir, tmpdir_type_t type, int max_files);
const char* get_tmpdir(void);
void clean_tmpdir();

const char* make_tmpdir(void);
int check_dir_is_rw(const char* tmpdir);

int tmpfiles_limit_reached(void);

#endif /* __TMPDIR_H__ */
