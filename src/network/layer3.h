/**
 * @file layer3.h
 *
 * @brief Layer 3 handling.
 * @author David Suárez
 * @date Sun, 28 Oct 2018 16:14:56 +0100
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __LAYER3_H__
#define __LAYER3_H__

#include "compat/compat.h"

#include <netinet/tcp.h>

/**
 * layer3_find_tcp:
 *
 * Handles the network layer (layer 3) trying to find tcp packets.
 *
 * @param[in] pkt the packet.
 * @param[in] nextproto layer 3 protocol.
 * @param[in/out] offset (in) offset of l3 proto / (out) offset of TCP payload.
 * @param[out] src,dst addresses and ports of the connexion.
 * @param[out] tcp the tcp header.
 *
 * @return 0 OK, -1 if unsupported proto or no TCP.
 */
int layer3_find_tcp(const u_char *pkt, uint8_t nextproto, int *offset,
		struct sockaddr *src, struct sockaddr *dst, struct tcphdr *tcp);

#endif /* __LAYER3_H__ */
