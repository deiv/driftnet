/*
 * playaudio.h:
 * Play audio data.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2002 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef __PLAYAUDIO_H__
#define __PLAYAUDIO_H__

void do_mpeg_player(void);
void stop_mpeg_player(void);

void mpeg_submit_chunk(const unsigned char *data, const size_t len);

#endif /* __PLAYAUDIO_H__ */
