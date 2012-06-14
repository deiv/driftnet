/*
 * playaudio.c:
 * Play audio data.
 *
 * The game here is that we use threads and fork. Two things you never want to
 * see together in the same sentence. Presently we only do MPEG data. Because
 * we can't assume that mpg123 or whatever isn't going to roll over and play
 * dead if we present it with random stuff we picked up off the network, we
 * arrange to restart it if it dies.
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: playaudio.c,v 1.4 2002/06/04 19:09:02 chris Exp $";

#include <sys/types.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/wait.h>

#include "driftnet.h"

extern int verbose; /* in driftnet.c */

/* The program we use to play MPEG data. Can be changed with -M */
char *audio_mpeg_player = "mpg123 -";

static pthread_mutex_t mpeg_mtx = PTHREAD_MUTEX_INITIALIZER;

#define m_lock      pthread_mutex_lock(&mpeg_mtx)
#define m_unlock    pthread_mutex_unlock(&mpeg_mtx)

/* audiochunk:
 * A bit of audio which we are going to throw at the decoder. list represents
 * the list of all chunks which are available, wr the place that we insert new
 * data that we've obtained and rd the place that we're reading data to send
 * into the decoder. */
typedef struct _audiochunk {
    unsigned char *data;
    size_t len;
    struct _audiochunk *next;
} *audiochunk;

static audiochunk list, wr, rd;

/* audiochunk_new:
 * Allocate a buffer and copy some data into it. */
static audiochunk audiochunk_new(const unsigned char *data, const size_t len) {
    audiochunk A;
    A = calloc(1, sizeof *A);
    A->len = len;
    if (data) {
        A->data = malloc(len);
        memcpy(A->data, data, len);
    }
    return A;
}

/* audiochunk_delete:
 * Free memory from an audiochunk. */
static void audiochunk_delete(audiochunk A) {
    free(A->data);
    free(A);
}

/* audiochunk_write:
 * Write the contents of an audiochunk down a file descriptor. Returns 0 on
 * success or -1 on failure. */
#define WRCHUNK     1024
static int audiochunk_write(const audiochunk A, int fd) {
    const unsigned char *p;
    ssize_t n;
    if (A->len == 0)
        return 0;
    p = A->data;
    do {
        size_t d = WRCHUNK;
        if (p + d > A->data + A->len)
            d = A->data + A->len - p;
            
        n = write(fd, p, d);
        if (n == -1 && errno != EINTR)
            return -1;
        else
            p += d;
    } while (p < A->data + A->len);
    return 0;
}

/* How much data we have buffered; if this rises too high, we start silently
 * dropping data. */
static size_t buffered;
#define MAX_BUFFERED    (8 * 1024 * 1024)   /* 8Mb */

/* mpeg_submit_chunk:
 * Put some MPEG data into the queue to be played. */
void mpeg_submit_chunk(const unsigned char *data, const size_t len) {
    audiochunk A;
    
    m_lock;

    if (buffered > MAX_BUFFERED) {
        if (verbose)
            fprintf(stderr, PROGNAME": MPEG buffer full with %d bytes\n", buffered);
        goto finish;
    }
    A = audiochunk_new(data, len);
    wr->next = A;
    wr = wr->next;

    buffered += len;
    
finish:
    m_unlock;
}

/* mpeg_play:
 * Play MPEG data. This runs in a separate thread. The parameter is the
 * audiochunk from which we start reading data. */
int mpeg_fd;     /* the file descriptor into which we write data. */

static void *mpeg_play(void *a) {
    audiochunk A;
    A = (audiochunk)a;
    
    while (1) { /*(!foad) {*/
        audiochunk A;
 
        m_lock;
        A = rd->next;
        m_unlock;
        
        if (A) {
            /* Got some data, submit it to the encoder. */
            if (audiochunk_write(A, mpeg_fd) == -1)
                fprintf(stderr, PROGNAME": write to MPEG player: %s\n", strerror(errno));

            m_lock;
            buffered -= A->len;
            audiochunk_delete(rd);
            rd = A;
            m_unlock;
        } else {
            /* No data, sleep for a little bit. */
            struct timespec tm;
            tm.tv_sec  = 0;
            tm.tv_nsec = 100000000;  /* 0.1s */
            nanosleep(&tm, NULL);
        }
    }

    return NULL;
}

/* mpeg_player_manager:
 * Main loop of child process which keeps an MPEG player running. */
static void mpeg_player_manager(void) {
    extern sig_atomic_t foad; /* in driftnet.c */
    struct sigaction sa = {0};
    pid_t mpeg_pid;

    sa.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &sa, NULL);
    
    while (!foad) {
        time_t whenstarted;
        int st;

        fprintf(stderr, PROGNAME": starting MPEG player `%s'\n", audio_mpeg_player);

        whenstarted = time(NULL);
        switch ((mpeg_pid = fork())) {
            case 0:
                execl("/bin/sh", "/bin/sh", "-c", audio_mpeg_player, NULL);
                fprintf(stderr, PROGNAME": exec: %s\n", strerror(errno));
                exit(-1);
                break;

            case -1:
                fprintf(stderr, PROGNAME": fork: %s\n", strerror(errno));
                exit(-1);  /* gah, not much we can do now. */
                break;
            
            default:
                /* parent. */
                if (verbose)
                    fprintf(stderr, PROGNAME": MPEG player has PID %d\n", (int)mpeg_pid);
                break;
        }

        /* wait for it to exit. */
        waitpid(mpeg_pid, &st, 0);
        mpeg_pid = 0;

        if (verbose) {
            if (WIFEXITED(st))
                fprintf(stderr, PROGNAME": MPEG player exited with status %d\n", WEXITSTATUS(st));
            else if (WIFSIGNALED(st))
                fprintf(stderr, PROGNAME": MPEG player killed by signal %d\n", WTERMSIG(st));
            /* else ?? */
        }


        if (!foad && time(NULL) - whenstarted < 5) {
            /* The player expired very quickly. Probably something's wrong;
             * sleep for a bit and hope the problem goes away. */
            fprintf(stderr, PROGNAME": MPEG player expired after %d seconds, sleeping for a bit\n", (int)(time(NULL) - whenstarted));
            sleep(5);
        }
    }
    if (mpeg_pid)
        kill(mpeg_pid, SIGTERM);
}

/* do_mpeg_player:
 * Fork and start a process which keeps an MPEG player running. Then start a
 * thread which passes data into the player. */
pid_t mpeg_mgr_pid;

void do_mpeg_player(void) {
    int pp[2];
    pthread_t thr;

    rd = wr = list = audiochunk_new(NULL, 0);

    pipe(pp);
    
    mpeg_mgr_pid = fork();
    if (mpeg_mgr_pid == 0) {
        close(pp[1]);
        dup2(pp[0], 0); /* make pipe our standard input */
        mpeg_player_manager();
        exit(-1);
    } else {
        close(pp[0]);
        mpeg_fd = pp[1];
        pthread_create(&thr, NULL, mpeg_play, rd);
    }

    /* away we go... */
}
