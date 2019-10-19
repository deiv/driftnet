/**
 * @file pcap.h
 *
 * @brief Network packet capture handling.
 * @author David Suárez
 * @author Chris Lightfoot
 * @date Sun, 28 Oct 2018 16:14:56 +0100
 *
 * Copyright (c) 2001 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __PCAP_H__
#define __PCAP_H__

#include "connection.h"

typedef struct {
	//int pkt_offset; /* offset of IP packet within wire packet */
	int type;
	const char* name;
} datalink_info_t;

void extract_media(connection c);

#endif  /* __PCAP_H__ */
