/*
 * httpd.c:
 * HTTP server.
 *
 * Copyright (c) 2012-2019 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#include "compat.h"

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>

#include "web_data.h"
#include "common/log.h"
#include "common/util.h"
#include "common/tmpdir.h"

/*
 * Tests if we have a modern libwebsockets library (>= 3.0.0). Prior versions didn't include
 *  some helpers macros.
 */
#ifndef lws_ll_fwd_insert

#define lws_ll_fwd_insert(\
 	       ___new_object,  /* pointer to new object */ \
 	       ___m_list,      /* member for next list object ptr */ \
 	       ___list_head    /* list head */ \
 	               ) {\
 	               ___new_object->___m_list = ___list_head; \
 	               ___list_head = ___new_object; \
 	       }

#define lws_ll_fwd_remove(\
 	       ___type,        /* type of listed object */ \
 	       ___m_list,      /* member for next list object ptr */ \
 	       ___target,      /* object to remove from list */ \
 	       ___list_head    /* list head */ \
 	       ) { \
 	                lws_start_foreach_llp(___type **, ___ppss, ___list_head) { \
if (*___ppss == ___target) { \
 	                                *___ppss = ___target->___m_list; \
 	                                break; \
 	                        } \
 	                } lws_end_foreach_llp(___ppss, ___m_list); \
 	       }

/*
 * This is a helper that combines the common pattern of needing to consume
 * some ringbuffer elements, move the consumer tail on, and check if that
 * has moved any ringbuffer elements out of scope, because it was the last
 * consumer that had not already consumed them.
 *
 * Elements that go out of scope because the oldest tail is now after them
 * get garbage-collected by calling the destroy_element callback on them
 * defined when the ringbuffer was created.
 */

#define lws_ring_consume_and_update_oldest_tail(\
 	               ___ring,    /* the lws_ring object */ \
 	               ___type,    /* type of objects with tails */ \
 	               ___ptail,   /* ptr to tail of obj with tail doing consuming
 	*/ \
 	               ___count,   /* count of payload objects being consumed */ \
 	               ___list_head,   /* head of list of objects with tails */ \
 	               ___mtail,   /* member name of tail in ___type */ \
 	               ___mlist  /* member name of next list member ptr in ___type
 	*/ \
 	       ) { \
 	               int ___n, ___m; \
 	       \
 	       ___n = lws_ring_get_oldest_tail(___ring) == *(___ptail); \
 	       lws_ring_consume(___ring, ___ptail, NULL, ___count); \
 	       if (___n) { \
 	               uint32_t ___oldest; \
 	               ___n = 0; \
 	               ___oldest = *(___ptail); \
 	               lws_start_foreach_llp(___type **, ___ppss, ___list_head) { \
 	                       ___m = lws_ring_get_count_waiting_elements( \
 	                                       ___ring, &(*___ppss)->tail); \
 	                       if (___m >= ___n) { \
 	                               ___n = ___m; \
 	                               ___oldest = (*___ppss)->tail; \
 	                       } \
 	               } lws_end_foreach_llp(___ppss, ___mlist); \
 	       \
 	               lws_ring_update_oldest_tail(___ring, ___oldest); \
 	       } \
 	}
#endif /* lws_ll_fwd_insert */

int server_port;
int interrupted = 0;
pthread_t server_thread;

struct msg {
    void *payload;
    size_t len;
};

struct per_session_data {
    struct per_session_data *pss_list;
    struct lws *wsi;
    uint32_t tail;
};

struct per_vhost_data {
    struct lws_context *context;
    struct lws_vhost *vhost;
    const struct lws_protocols *protocol;
    struct per_session_data *pss_list;      /* linked-list of live pss */
    struct lws_ring *ring;                  /* ringbuffer holding unsent messages */
};

int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                     void *user, void *in, size_t len);
static void destroy_message(void *_msg);

static struct lws_protocols protocols[] = {
        { "http", lws_callback_http_dummy, 0, 0 },
        {
          "images-pipe-protocol",
          ws_callback,
           sizeof(struct per_session_data),
              128,
        },
        { NULL, NULL, 0, 0 }
};

struct per_vhost_data *vhost_data;          /* our vhost */

void write_static_resources()
{
    int idx = 0;
    char* static_file = static_files[idx];

    while (static_file != NULL) {
        char* static_source_path = compose_path(STATIC_WEB_DIRECTORY, static_file);

        if (tmpfile_link_file(static_source_path) == FALSE) {
            log_msg(LOG_ERROR, "httpd: we can't link the necessary static resource");
            exit(1);
        }

        xfree(static_source_path);
        idx += 1;
        static_file = static_files[idx];
    }
}

void delete_static_resources()
{
    int idx = 0;
    char* static_file = static_files[idx];

    while (static_file != NULL) {
        tmpfile_unlink_file(static_file);
        idx += 1;
        static_file = static_files[idx];
    }
}

static void * http_server_dispatch(void *arg)
{
    char* server_root = arg;
    int n = 0;
    struct lws_context_creation_info info;
    struct lws_context *context;

    static const struct lws_protocol_vhost_options mime_types[] = {
       { &mime_types[1], NULL, ".jpeg", "image/jpeg" },
       { NULL, NULL, ".webp", "image/webp" }
    };

   const struct lws_http_mount mount = {
        (struct lws_http_mount *)NULL,	/* linked-list pointer to next*/
        "/",		                    /* mountpoint in URL namespace on this vhost */
        server_root,                    /* where to go on the filesystem for that */
        "index.html",	                /* default filename if none given */
        NULL,
        NULL,
        mime_types,
        NULL,
        0,
        0,
        0,
        0,
        0,
        0,
        LWSMPRO_FILE,	                /* mount type is a directory in a filesystem */
        1,		                        /* strlen("/"), ie length of the mountpoint */
        NULL,
#if LWS_LIBRARY_VERSION_MAJOR <= 4 && LWS_LIBRARY_VERSION_MINOR < 3
        { NULL, NULL } // sentinel
#endif
    };

    memset(&info, 0, sizeof info);

    info.port = server_port;
    info.mounts = &mount;
    info.protocols = protocols;

    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);

    context = lws_create_context(&info);

    if (!context) {
        log_msg(LOG_ERROR, "http server init failed");
        abort(); /* TODO: exit ¿? */
        return NULL;
    }

    log_msg(LOG_WARNING, "http server initializated. go to http://localhost:%d", server_port);

    while (n >= 0 && !interrupted) {
        n = lws_service(context, 1000);
    }

    lws_context_destroy(context);

    return NULL;
}

void ws_send_text(const char* text)
{
    struct msg amsg;
    size_t text_len = strlen(text);

    amsg.len = text_len;
    amsg.payload = malloc(LWS_PRE + text_len);

    if (!amsg.payload) {
        log_msg(LOG_WARNING, "httpd: dropping msg");
        return;
    }

    memcpy((char *)amsg.payload + LWS_PRE, text, text_len);

    if (!lws_ring_insert(vhost_data->ring, &amsg, 1)) {
        destroy_message(&amsg);
        log_msg(LOG_WARNING, "httpd: can't insert msg into ring buffer");
        return;
    }

    lws_start_foreach_llp(struct per_session_data **, ppss, vhost_data->pss_list) {
        lws_callback_on_writable((*ppss)->wsi);
    } lws_end_foreach_llp(ppss, pss_list);
}

static void destroy_message(void *_msg)
{
    struct msg *msg = _msg;

    free(msg->payload);
    msg->payload = NULL;
    msg->len = 0;
}

int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                 void *user, void *in, size_t len)
{
    struct per_session_data *pss =
            (struct per_session_data *) user;
    struct per_vhost_data *vhd =
            (struct per_vhost_data *)
                    lws_protocol_vh_priv_get(lws_get_vhost(wsi),
                                             lws_get_protocol(wsi));
    const struct msg *pmsg;
    int m;

    switch (reason) {
        case LWS_CALLBACK_PROTOCOL_INIT:
            vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
                                              lws_get_protocol(wsi),
                                              sizeof(struct per_vhost_data));
            vhd->context = lws_get_context(wsi);
            vhd->protocol = lws_get_protocol(wsi);
            vhd->vhost = lws_get_vhost(wsi);

            vhd->ring = lws_ring_create(sizeof(struct msg), 50,
                                        destroy_message);
            if (!vhd->ring) {
                log_msg(LOG_ERROR, "httpd: can't create message buffer");
                abort();
                return 1;
            }
            vhost_data = vhd;
            break;

        case LWS_CALLBACK_PROTOCOL_DESTROY:
            lws_ring_destroy(vhd->ring);
            break;

        case LWS_CALLBACK_ESTABLISHED:
            lws_ll_fwd_insert(pss, pss_list, vhd->pss_list);
            pss->tail = lws_ring_get_oldest_tail(vhd->ring);
            pss->wsi = wsi;
            break;

        case LWS_CALLBACK_CLOSED:
            lws_ll_fwd_remove(struct per_session_data, pss_list,
                pss, vhd->pss_list);
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            pmsg = lws_ring_get_element(vhd->ring, &pss->tail);
            if (!pmsg) {
                break;
            }

            /* notice we allowed for LWS_PRE in the payload already */
            m = lws_write(wsi, pmsg->payload + LWS_PRE, pmsg->len, LWS_WRITE_TEXT);

            if (m < (int)pmsg->len) {
                log_msg(LOG_WARNING, "httpd: can't write to ws socket\n", m);
                return -1;
            }

            lws_ring_consume_and_update_oldest_tail(
                vhd->ring,	             /* lws_ring object */
                struct per_session_data, /* type of objects with tails */
                &pss->tail,	             /* tail of guy doing the consuming */
                1,		                 /* number of payload objects being consumed */
                vhd->pss_list,	         /* head of list of objects with tails */
                tail,		             /* member name of tail in objects with tails */
                pss_list	             /* member name of next object in objects with tails */
            );

            if (lws_ring_get_element(vhd->ring, &pss->tail)) {
                lws_callback_on_writable(pss->wsi);
            }
            break;

        default:
            break;
    }

    return 0;
}

void init_http_display(const char* server_root, int port)
{
    server_port = port;

    write_static_resources();

    pthread_create(&server_thread, NULL, http_server_dispatch, (void*)server_root);
}

void stop_http_display()
{
    interrupted = 1;

    /*
     * XXX: TODO: not cancel on first terminate signal (crtl-c), lets the websocket library clean up itself (it takes
     *              some seconds). Force cancel on second or more signals.
     */
    pthread_cancel(server_thread);
    pthread_join(server_thread, NULL);

    delete_static_resources();
}
