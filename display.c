/*
 * display.c:
 * Display images gathered by driftnet.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: display.c,v 1.7 2002/02/15 12:35:11 chris Exp $";

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "driftnet.h"
#include "img.h"

/* The border, in pixels, around images displayed in the window. */
#define BORDER  6

extern int verbose; /* in driftnet.c */

static GtkWidget *window, *darea;

static int width, height, wrx, wry, rowheight;
static img backing_image;

struct imgrect {
    char *filename;
    int x, y, w, h;
};

int nimgrects;
struct imgrect *imgrects;

gint delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    if (verbose)
        fprintf(stderr, PROGNAME ": display child shutting down\n");
    return FALSE;   /* do destroy window */
}

void make_backing_image() {
    img I;
    I = img_new_blank(width, height);
    img_alloc(I);
    if (backing_image) {
        int w2, h2;
        struct imgrect *ir;

        /* Copy old contents of backing image to ll corner of new one. */
        w2 = backing_image->width;
        if (w2 > width) w2 = width;
        h2 = backing_image->height;
        if (h2 > height) h2 = height;

        img_simple_blt(I, 0, height - h2, backing_image, 0, backing_image->height - h2, w2, h2);

        /* Move all of the image rectangles. */
        for (ir = imgrects; ir < imgrects + nimgrects; ++ir)
            if (ir->filename)
                ir->y += backing_image->height - height;

        /* Adjust placement of new images. */
        if (wrx >= w2) wrx = w2;
        
        img_delete(backing_image);
    }
    backing_image = I;
    wrx = BORDER;
    wry = height - BORDER;
    rowheight = 2 * BORDER;
}

void update_window() {
    if (backing_image) {
        GdkGC *gc = gdk_gc_new(darea->window);
        gdk_draw_rgb_32_image(darea->window, gc, 0, 0, width, height, GDK_RGB_DITHER_NORMAL, (guchar*)backing_image->flat, sizeof(pel) * width);
        gdk_gc_destroy(gc);
    }
}

void scroll_backing_image(const int dy) {
    pel **row1, **row2;
    struct imgrect *ir;
    
    for (row1 = backing_image->data, row2 = backing_image->data + dy;
         row2 < backing_image->data + height; ++row1, ++row2)
        memcpy(*row1, *row2, width * sizeof(pel));

    for (row2 = row1; row2 < backing_image->data + height; ++row2)
        memset(*row2, 0, width * sizeof(pel));

    for (ir = imgrects; ir < imgrects + nimgrects; ++ir) {
        if (ir->filename) {
            ir->y -= dy;

            /* scrolled off bottom, no longer in use. */
            if ((ir->y + ir->h) < 0) {
                unlink(ir->filename);
                free(ir->filename);
                memset(ir, 0, sizeof *ir);
            }
        }
    }
}

void add_image_rectangle(const char *filename, const int x, const int y, const int w, const int h) {
    struct imgrect *ir;
    for (ir = imgrects; ir < imgrects + nimgrects; ++ir) {
        if (!ir->filename)
            break;
    }
    if (ir == imgrects + nimgrects) {
        imgrects = realloc(imgrects, 2 * nimgrects * sizeof *imgrects);
        memset(imgrects + nimgrects, 0, nimgrects * sizeof *imgrects);
        ir = imgrects + nimgrects;
        nimgrects *= 2;
    }
    ir->filename = strdup(filename);
    ir->x = x;
    ir->y = y;
    ir->w = w;
    ir->h = h;
}

struct imgrect *find_image_rectangle(const int x, const int y) {
    struct imgrect *ir;
    for (ir = imgrects; ir < imgrects + nimgrects; ++ir)
        if (ir->filename && x >= ir->x && x < ir->x + ir->w && y >= ir->y && y < ir->y + ir->h)
            return ir;
    return NULL;
}

void expose_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    gdk_window_get_size(darea->window, &width, &height);
    if (!backing_image || backing_image->width != width || backing_image->height != height)
        make_backing_image();

    update_window();
}

void configure_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    gdk_window_get_size(darea->window, &width, &height);
    if (!backing_image || backing_image->width != width || backing_image->height != height)
        make_backing_image();

    update_window();
}

char *savedimgpfx = "driftnet-";

void save_image(struct imgrect *ir) {
    static char *name;
    static int num;
    int fd1, fd2;
    char buf[8192];
    ssize_t l;

    if (!name)
        name = calloc(strlen(savedimgpfx) + 16, 1);

    sprintf(name, "%s%d%s", savedimgpfx, num++, strrchr(ir->filename, '.'));
    fprintf(stderr, PROGNAME": saving `%s' as `%s'\n", ir->filename, name);

    fd1 = open(ir->filename, O_RDONLY);
    if (fd1 == -1) {
        fprintf(stderr, PROGNAME": %s: %s\n", ir->filename, strerror(errno));
        return;
    }
    
    fd2 = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd2 == -1) {
        fprintf(stderr, PROGNAME": %s: %s\n", name, strerror(errno));
        close(fd1);
        return;
    }

    /* XXX interrupts */
    while ((l = read(fd1, buf, sizeof buf)) > 0) {
        if (write(fd2, buf, l) == -1) {
            fprintf(stderr, PROGNAME": %s: %s\n", name, strerror(errno));
            close(fd1);
            close(fd2);
            return;
        }
    }

    if (l == -1)
        fprintf(stderr, PROGNAME": %s: %s\n", ir->filename, strerror(errno));

    close(fd1);
    close(fd2);
}

static struct {
    int x, y;
} click;

void button_press_event(GtkWidget *widget, GdkEventButton *event) {
    click.x = (int)event->x;
    click.y = (int)event->y;
}

void button_release_event(GtkWidget *widget, GdkEventButton *event) {
    struct imgrect *ir;
    ir = find_image_rectangle(click.x, click.y);
    if (ir && ir == find_image_rectangle((int)event->x, (int)event->y))
        save_image(ir);
}

void destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

extern int dpychld_fd;  /* in driftnet.c */

gboolean pipe_event(GIOChannel chan, GIOCondition cond, gpointer data) {
    struct pipemsg m = {0};
    ssize_t rr;
    while ((rr = read(dpychld_fd, &m, sizeof(m))) == sizeof(m)) {
        int saveimg = 0;
        if (verbose)
            fprintf(stderr, PROGNAME": received image %s of size %d\n", m.filename, m.len);
        /* checks to see whether this looks like an image we're interested in. */
        if (m.len > 256) {
            /* small images are probably bollocks. */
            img i = img_new();
            if (!img_load_file(i, m.filename, header, unknown))
                fprintf(stderr, PROGNAME": %s: bogus image (err = %d)\n", m.filename, i->err);
            else {
                if (i->width > 8 && i->height > 8) {
                    if (img_load(i, full, i->type)) {
                        /* slot in the new image at some plausible place. */
                        int w, h;
                        if (i->width > width - 2 * BORDER) w = width - 2 * BORDER;
                        else w = i->width;
                        if (i->height > height - 2 * BORDER) h = height - 2 * BORDER;
                        else h = i->height;

                        /* is there space on this row? */
                        if (width - wrx < w) {
                            /* no */
                            scroll_backing_image(h + BORDER);
                            wrx = BORDER;
                            rowheight = h + BORDER;
                        }
                        if (rowheight < h + BORDER) {
                            scroll_backing_image(h + BORDER - rowheight);
                            rowheight = h + BORDER;
                        }

                        img_simple_blt(backing_image, wrx, wry - h, i, 0, 0, w, h);
                        add_image_rectangle(m.filename, wrx, wry - h, w, h);
                        saveimg = 1;

                        update_window();

                        wrx += w + BORDER;
                    } else fprintf(stderr, PROGNAME": %s: bogus image (err = %d)\n", m.filename, i->err);
                } else if (verbose) fprintf(stderr, PROGNAME": %s: image dimensions (%d x %d) too small to bother with\n", m.filename, i->width, i->height);
            }

            img_delete(i);
        } else if (verbose) fprintf(stderr, PROGNAME": image data too small (%d bytes) to bother with\n", (int)m.len);

        if (!saveimg)
            unlink(m.filename);
    }
    if (rr == -1 && errno != EINTR && errno != EAGAIN) {
        perror(PROGNAME": read");
        gtk_main_quit();
    } else if (rr == 0) {
        gtk_main_quit();
    }
    return TRUE;
}

int dodisplay(int argc, char *argv[]) {
    GIOChannel *chan;

    /* have our main loop poll the pipe file descriptor */
    chan = g_io_channel_unix_new(dpychld_fd);
    g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP, (GIOFunc)pipe_event, NULL);
    fcntl(dpychld_fd, F_SETFL, O_NONBLOCK);

    /* set up list of image rectangles. */
    imgrects = calloc(nimgrects = 16, sizeof *imgrects);
       
    /* do some init thing */
    gtk_init(&argc, &argv);
    gdk_rgb_init();

    gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
    gtk_widget_set_default_visual(gdk_rgb_get_visual());

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_usize(window, 0, 0);

    darea = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), darea);
    gtk_widget_set_events(darea, GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);

    gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(delete_event), NULL);
    gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(destroy), NULL);

    gtk_signal_connect(GTK_OBJECT(darea), "expose-event", GTK_SIGNAL_FUNC(expose_event), NULL);
    gtk_signal_connect(GTK_OBJECT(darea), "configure_event", GTK_SIGNAL_FUNC(expose_event), NULL);
    
    /* mouse button press/release for saving images */
    gtk_signal_connect(GTK_OBJECT(darea), "button_press_event", GTK_SIGNAL_FUNC(button_press_event), NULL);
    gtk_signal_connect(GTK_OBJECT(darea), "button_press_event", GTK_SIGNAL_FUNC(button_release_event), NULL);
    
    gtk_widget_show_all(window);
    
    gtk_main();
                         
    return 0;
}
