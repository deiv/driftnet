/*
 * driftnet.c:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2001 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "common/log.h"
#include "options.h"
#include "common/tmpdir.h"
#include "pid.h"
#include "connection.h"
#include "packetcapture.h"
#include "playaudio.h"
#include "uid.h"
#ifndef NO_DISPLAY_WINDOW
    #include "display.h"
#endif
#ifndef NO_HTTP_DISPLAY
    #include "httpd.h"
#endif

#include "driftnet.h"

static void terminate_on_signal(int s);
static void setup_signals(void);
static void *capture_thread(void *v);

void unexpected_exit(int ret)
{
    /* clean things a litle */
#ifndef NO_HTTP_DISPLAY
    stop_http_display();
#endif
    packetcapture_close();
    connection_free_slots();
    clean_tmpdir();
    close_pidfile();

	exit(ret);
}

/* terminate_on_signal:
 * Terminate on receipt of an appropriate signal. */
sig_atomic_t foad;

void terminate_on_signal(int s)
{
    extern pid_t mpeg_mgr_pid; /* in playaudio.c */

    /* Pass on the signal to the MPEG player manager so that it can abort,
     * since it won't die when the pipe into it dies. */
    if (mpeg_mgr_pid)
        kill(mpeg_mgr_pid, s);
    foad = s;
}

/*
 * Set up signal handlers.
 */
void setup_signals(void) {
    int *p;
    /* Signals to ignore. */
    int ignore_signals[] = {SIGPIPE, 0};
    /* Signals which mean we should quit, killing the display child if
     * applicable. */
    int terminate_signals[] = {SIGTERM, SIGINT, /*SIGSEGV,*/ SIGBUS, SIGCHLD, 0};
    struct sigaction sa;

    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    for (p = ignore_signals; *p; ++p) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_IGN;
        sigaction(*p, &sa, NULL);
    }

    for (p = terminate_signals; *p; ++p) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate_on_signal;
        sigaction(*p, &sa, NULL);
    }
}

/*
 * Thread in which packet capture runs.
 */
void *capture_thread(void *v)
{
    while (!foad)
        packetcapture_dispatch();

    return NULL;
}

static void print_exit_reason(void)
{
    if (foad == SIGCHLD) {
        pid_t pp;
        int st;

        while ((pp = waitpid(-1, &st, WNOHANG)) > 0) {
            if (WIFEXITED(st))
                log_msg(LOG_INFO, "child process %d exited with status %d", (int)pp, WEXITSTATUS(st));
            else if (WIFSIGNALED(st))
                log_msg(LOG_INFO, "child process %d killed by signal %d", (int)pp, WTERMSIG(st));
            else
                log_msg(LOG_INFO, "child process %d died, not sure why", (int)pp);

        }

    } else
        log_msg(LOG_INFO, "caught signal %d", foad);
}

/*
 * Entry point. Process command line options, start up pcap and enter capture loop.
 */
int main(int argc, char *argv[])
{
    pthread_t packetth;
    options_t *options;

    options = parse_options(argc, argv);
	
	if (options->verbose)
        set_loglevel(LOG_INFO);
	
	if (options->list_interfaces == 1) {
		packetcapture_list_interfaces();
		return 0;
	}

    /* Start up pcap as soon as posible to later drop root privileges. */
    if (options->dumpfile)
        packetcapture_open_offline(options->dumpfile);
    else
        packetcapture_open_live(options->interface, options->filterexpr, options->promisc, options->monitor_mode);

    /* If we are root and an username was specified, drop privileges to that user */
    if (getuid() == 0 || geteuid() == 0) {
        if (options->drop_username) {
            drop_root(options->drop_username);
        }
    }

    if (options->adjunct)
        create_pidfile();

    /*
     * In adjunct mode, it's important that the attached program gets
     * notification of images in a timely manner. Make stdout line-buffered
     * for this reason.
     */
    if (options->adjunct)
        setvbuf(stdout, NULL, _IOLBF, 0);

    /*
     * If a directory name has not been specified, then we need to create one.
     * Otherwise, check that it's a directory into which we may write files.
     */
    if (options->tmpdir) {
        if (check_dir_is_rw(options->tmpdir) == FALSE) {
            log_msg(LOG_ERROR, "we can't write to the temporary directory");
            exit(1);
        }
        set_tmpdir(options->tmpdir, TMPDIR_USER_OWNED, options->max_tmpfiles, options->adjunct);

    } else {
        /* need to make a temporary directory. */
        const char* tmp_dir = make_tmpdir();

        if (tmp_dir == NULL) {
            log_msg(LOG_ERROR, "can't make a new temporary directory");
            exit(1);
        }
        set_tmpdir(tmp_dir, TMPDIR_APP_OWNED, options->max_tmpfiles, options->adjunct);
    }

    setup_signals();

    /* Start up the audio player, if required. */
    if (!options->adjunct && (options->extract_type & m_audio))
        do_mpeg_player();

#ifndef NO_DISPLAY_WINDOW
    /* Possibly fork to start the display child process */
    if (options->enable_gtk_display && !options->adjunct && (options->extract_type & m_image))
        do_image_display(options->savedimgpfx, options->beep);

#endif /* !NO_DISPLAY_WINDOW */

#ifndef NO_HTTP_DISPLAY
    if (options->enable_http_display && !options->adjunct) {
        init_http_display(get_tmpdir(), options->http_server_port);
    }
#endif
    if (options->adjunct) {
        log_msg(LOG_INFO, "operating in adjunct mode");
    }

    init_mediadrv(options->extract_type, !options->adjunct, options->enable_http_display, options->enable_gtk_display);

    connection_alloc_slots();

    /*
     * Actually start the capture stuff up. Unfortunately, on many platforms,
     * libpcap doesn't have read timeouts, so we start the thing up in a
     * separate thread. Yay!
     */
    pthread_create(&packetth, NULL, capture_thread, NULL);

    while (!foad)
        sleep(1);

    if (options->verbose)
        print_exit_reason();

#ifndef NO_HTTP_DISPLAY
    if (options->enable_http_display) {
        stop_http_display();
    }
#endif

    pthread_cancel(packetth); /* make sure thread quits even if it's stuck in pcap_dispatch */
    pthread_join(packetth, NULL);

    /* Clean up. */
    /*    pcap_freecode(pc, &filter);*/ /* not on some systems... */
    packetcapture_close();

    /* Easier for memory-leak debugging if we deallocate all this here.... */
    connection_free_slots();

    clean_tmpdir();

    if (options->adjunct)
        close_pidfile();

    return 0;
}
