/*
 * pcap.h:
 * Network packet capture handling.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __PACKETCAPTURE_H__
#define __PACKETCAPTURE_H__

void packetcapture_open_live(char* interface, char* filterexpr, int promisc);
void packetcapture_open_offline(char* dumpfile);
void packetcapture_close(void);

inline char* get_default_interface();

inline void packetcapture_dispatch(void);

#endif  /* __PACKETCAPTURE_H__ */
