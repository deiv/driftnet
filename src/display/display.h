/*
 * display.h:
 * Display images gathered by driftnet.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2001 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

void do_image_display(char *img_prefix, int beep);

void display_send_img(const char *name, size_t len);

#endif /* __DISPLAY_H__ */
