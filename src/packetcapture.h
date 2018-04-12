/*
 * pcap.h:
 * Network packet capture handling.
 *
 * Copyright (c) 2001 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __PACKETCAPTURE_H__
#define __PACKETCAPTURE_H__

typedef struct {
	//int pkt_offset; /* offset of IP packet within wire packet */
	int type;
	const char* name;
} datalink_info_t;

void packetcapture_list_interfaces(void);
void packetcapture_open_live(char* interface, char* filterexpr, int promisc);
void packetcapture_open_offline(char* dumpfile);
void packetcapture_close(void);

void packetcapture_dispatch(void);

char* get_default_interface(void);

#endif  /* __PACKETCAPTURE_H__ */
