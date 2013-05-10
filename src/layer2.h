/*
 * layer2.h:
 *
 * Layer2 handling.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __LAYER2_H__
#define __LAYER2_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "compat.h"

#include "packetcapture.h" /* for datalink_info_t */

/**
 * handle_link_layer:
 *
 * Handles the data link layer (layer2) returning information off the next protocol and
 * the offset of next header in the packet.
 *
 * @param[in] info data link information.
 * @param[in] pkt the captured packet.
 * @param[out] nextproto the protocol of next header (as IP protocol).
 * @param[out] offsetnext the offset of next header in packet.
 *
 * @return 0 if OK, -1 if error / unsupported proto
 */
int handle_link_layer(datalink_info_t *info, const u_char *pkt, uint8_t *nextproto, int *offsetnext);

#endif /* __LAYER2_H__ */
