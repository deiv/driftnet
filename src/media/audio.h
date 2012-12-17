/*
 * audio.h:
 * Attempt to extract audio frames embedded in buffers.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef __AUDIO_H__
#define __AUDIO_H__

unsigned char *find_mpeg_stream(const unsigned char *data, const size_t len,
        unsigned char **mpegdata, size_t *mpeglen);

#endif /* __AUDIO_H__ */
