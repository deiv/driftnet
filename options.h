/*
 * options.h:
 * Options parsing.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "driftnet.h" /* for enum mediatype */

typedef struct {
    const char *tmpdir;
    int tmpdir_especified;
    int max_tmpfiles;
    int extract_images;
    int verbose;
    int adjunct;
    int beep;
    int promisc;
    char *dumpfile;
    char *interface;
    char *filterexpr;
    enum mediatype extract_type;
    char *audio_mpeg_player;
    int mpeg_player_specified;
    int newpfx;
#ifndef NO_DISPLAY_WINDOW
    char *savedimgpfx;
#endif
} options_t;

options_t* parse_options(int argc, char *argv[]);
/* 
 * Allow global access to options...
 * This will get removed when code refactoring are complete.
 */
inline options_t* get_options(void);

#endif /* __OPTIONS_H__ */

