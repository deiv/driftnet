/*
 * options.h:
 * Options parsing.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <unistd.h>
#if HAVE_STRING_H
    #include <string.h>
#endif

#include "compat.h"
#include "log.h"
#include "driftnet.h"
#include "packetcapture.h"

#include "options.h"

options_t options = {NULL, FALSE, 0, TRUE, FALSE, FALSE, FALSE, TRUE,
        NULL, NULL, NULL, m_image, NULL, FALSE, FALSE
#ifndef NO_DISPLAY_WINDOW
    ,NULL
#endif
};

static void validate_options(options_t* options);
static void usage(FILE *fp);

/*
 * Handle command-line options
 */
options_t* parse_options(int argc, char *argv[])
{
    char optstring[] = "abd:f:hi:M:m:pSsvx:";
    int c;

    opterr = 0;
    while ((c = getopt(argc, argv, optstring)) != -1) {
        switch(c) {
            case 'h':
                usage(stdout);
                exit (0);

            case 'i':
                if (options.dumpfile) {
                    log_msg(LOG_ERROR, "can't specify -i and -f");
                    unexpected_exit (-1);
                }
                options.interface = optarg;
                break;

            case 'v':
                options.verbose = TRUE;
                break;

            case 'b':
                if (!isatty(1))
                    log_msg(LOG_WARNING, "can't beep unless standard output is a terminal");
                else
                    options.beep = TRUE;
                break;

            case 'p':
                options.promisc = FALSE;
                break;

            case 's':
                options.extract_type |= m_audio;
                break;

            case 'S':
                options.extract_type = m_audio;
                break;

            case 'M':
                options.audio_mpeg_player = optarg;
                options.mpeg_player_specified = TRUE;
                break;

            case 'a':
                options.adjunct = TRUE;
                break;

            case 'm':
                options.max_tmpfiles = atoi(optarg);
                if (options.max_tmpfiles <= 0) {
                    log_msg(LOG_ERROR, "`%s' does not make sense for -m", optarg);
                    unexpected_exit (-1);
                }
                break;

            case 'd':
                options.tmpdir = strdup(optarg);
                options.tmpdir_especified = TRUE; /* so we don't delete it. */
                break;

            case 'f':
                if (options.interface) {
                    log_msg(LOG_ERROR, "can't specify -i and -f");
                    unexpected_exit (-1);
                }
                options.dumpfile = optarg;
                break;

#ifndef NO_DISPLAY_WINDOW
            case 'x':
                options.savedimgpfx = optarg;
                options.newpfx = TRUE;
                break;
#endif

            case '?':
            default:
                if (strchr(optstring, optopt))
                    log_msg(LOG_ERROR, "option -%c requires an argument", optopt);
                else
                    log_msg(LOG_ERROR, "unrecognised option -%c", optopt);
                usage(stderr);
                unexpected_exit (1);
        }
    }

    /* Build up filter. */
    if (optind < argc) {
        if (options.dumpfile)
            log_msg(LOG_WARNING, "filter code ignored with dump file");
        else {
            char **a;
            int l;
            for (a = argv + optind, l = sizeof("tcp and ()"); *a; l += strlen(*a) + 1, ++a);
            options.filterexpr = calloc(l, 1);
            strcpy(options.filterexpr, "tcp and (");
            for (a = argv + optind; *a; ++a) {
                strcat(options.filterexpr, *a);
                if (*(a + 1)) strcat(options.filterexpr, " ");
            }
            strcat(options.filterexpr, ")");
        }
    } else options.filterexpr = "tcp";

    log_msg(LOG_INFO, "using filter expression `%s'", options.filterexpr);

#ifndef NO_DISPLAY_WINDOW
    if (options.newpfx && !options.adjunct)
        log_msg(LOG_INFO, "using saved image prefix `%s'", options.savedimgpfx);
#endif

    validate_options(&options);

    return &options;
}

void validate_options(options_t* options)
{
#ifdef NO_DISPLAY_WINDOW
    if (!options->adjunct) {
        /*
         * TODO: assume adjuct mode by default if we were compiled without
         * display support.
         */
        log_msg(LOG_ERROR, "this version of driftnet was compiled without display support");
        log_msg(LOG_ERROR, "use the -a option to run it in adjunct mode");
        unexpected_exit (-1);
    }
#endif /* !NO_DISPLAY_WINDOW */

    if (!options->dumpfile) {
        if (!options->interface) {
            /* TODO: on linux works "any" for all interfaces */
            options->interface = get_default_interface();
        }
    }

    /* Let's not be too fascist about option checking.... */
    if (options->max_tmpfiles && !options->adjunct) {
        log_msg(LOG_WARNING, "-m only makes sense with -a");
        options->max_tmpfiles = 0;
    }

    if (options->adjunct && options->newpfx)
        log_msg(LOG_WARNING, "-x ignored -a");

    if (options->mpeg_player_specified && !(options->extract_type & m_audio))
        log_msg(LOG_WARNING, "-M only makes sense with -s");

    if (options->mpeg_player_specified && options->adjunct)
        log_msg(LOG_WARNING, "-M ignored with -a");

    if (options->max_tmpfiles && options->adjunct)
        log_msg(LOG_INFO, "a maximum of %d images will be buffered", options->max_tmpfiles);

    if (options->beep && options->adjunct)
        log_msg(LOG_WARNING, "can't beep in adjunct mode");
}

/* usage:
 * Print usage information. */
void usage(FILE *fp)
{
    fprintf(fp,
"driftnet, version %s\n"
"Capture images from network traffic and display them in an X window.\n"
#ifdef NO_DISPLAY_WINDOW
"\n"
"Actually, this version of driftnet was compiled with the NO_DISPLAY_WINDOW\n"
"option, so that it can only be used in adjunct mode. See below.\n"
#endif /* NO_DISPLAY_WINDOW */
"\n"
"Synopsis: driftnet [options] [filter code]\n"
"\n"
"Options:\n"
"\n"
"  -h               Display this help message.\n"
"  -v               Verbose operation.\n"
"  -b               Beep when a new image is captured.\n"
"  -i interface     Select the interface on which to listen (default: all\n"
"                   interfaces).\n"
"  -f file          Instead of listening on an interface, read captured\n"
"                   packets from a pcap dump file; file can be a named pipe\n"
"                   for use with Kismet or similar.\n"
"  -p               Do not put the listening interface into promiscuous mode.\n"
"  -a               Adjunct mode: do not display images on screen, but save\n"
"                   them to a temporary directory and announce their names on\n"
"                   standard output.\n"
"  -m number        Maximum number of images to keep in temporary directory\n"
"                   in adjunct mode.\n"
"  -d directory     Use the named temporary directory.\n"
"  -x prefix        Prefix to use when saving images.\n"
"  -s               Attempt to extract streamed audio data from the network,\n"
"                   in addition to images. At present this supports MPEG data\n"
"                   only.\n"
"  -S               Extract streamed audio but not images.\n"
"  -M command       Use the given command to play MPEG audio data extracted\n"
"                   with the -s option; this should process MPEG frames\n"
"                   supplied on standard input. Default: `mpg123 -'.\n"
"\n"
"Filter code can be specified after any options in the manner of tcpdump(8).\n"
"The filter code will be evaluated as `tcp and (user filter code)'\n"
"\n"
"You can save images to the current directory by clicking on them.\n"
"\n"
"Adjunct mode is designed to be used by other programs which want to use\n"
"driftnet to gather images from the network. With the -m option, driftnet will\n"
"silently drop images if more than the specified number of images are saved\n"
"in its temporary directory. It is assumed that some other process is\n"
"collecting and deleting the image files.\n"
"\n"
"driftnet, copyright (c) 2001-2 Chris Lightfoot <chris@ex-parrot.com>\n"
"home page: http://www.ex-parrot.com/~chris/driftnet/\n"
"\n"
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 2 of the License, or\n"
"(at your option) any later version.\n"
"\n",
            DRIFTNET_VERSION);
}
