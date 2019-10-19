/**
 * @file layer3.c
 *
 * @brief Layer 3 handling.
 * @author David Suárez
 * @date Sun, 28 Oct 2018 16:14:56 +0100
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#include "compat/compat.h"

#include <string.h>
#include <assert.h>

#ifdef __FreeBSD__
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <netinet/ip.h>
#include <netinet/ip6.h>

#include "common/log.h"
#include "layer3.h"

int layer3_find_tcp(const u_char *pkt, uint8_t nextproto, int * offset,
		struct sockaddr * src, struct sockaddr * dst, struct tcphdr * tcp)
{
	u_int16_t *sport = NULL;
	u_int16_t *dport = NULL;

	while (1) {
		switch (nextproto) {

		case IPPROTO_TCP: /* Found the TCP header , we're almost done */
			/* Copy out the TCP header */
			memcpy(tcp, pkt + *offset, sizeof(struct tcphdr));

			/* Update the sockaddr with the TCP ports */
			assert(sport && dport);
			*sport = tcp->th_sport;
			*dport = tcp->th_dport;

			/* update offset */
			*offset += tcp->th_off << 2;

			/* Success */
			return 0;


		case IPPROTO_IPIP:	/* IPIP tunnel */
		case IPPROTO_IP:	/* IP packet */
			{
				struct ip *ip;
				struct sockaddr_in *ps, *pd;

				ip = (struct ip *)(pkt + *offset);

				/* update nextproto and offset */
				nextproto = ip->ip_p;
				*offset += ip->ip_hl << 2;

				/* save the addresses in s and d */
				bzero(src, sizeof(struct sockaddr_storage));
				bzero(dst, sizeof(struct sockaddr_storage));

				ps = (struct sockaddr_in *)src;
				pd = (struct sockaddr_in *)dst;

				ps->sin_family = AF_INET;
				memcpy(&ps->sin_addr.s_addr, &ip->ip_src, sizeof(struct in_addr));
				sport = &ps->sin_port;

				pd->sin_family = AF_INET;
				memcpy(&pd->sin_addr.s_addr, &ip->ip_dst, sizeof(struct in_addr));
				dport = &pd->sin_port;
			}
			break;

		case IPPROTO_IPV6:	/* IPv6 packet */
			{
				struct ip6_hdr *ip6;
				struct sockaddr_in6 *ps, *pd;

				ip6 = (struct ip6_hdr *)(pkt + *offset);

				/* update nextproto and offset */
				nextproto = ip6->ip6_nxt;
				*offset += sizeof(struct ip6_hdr);

				/* save the addresses in s and d */
				bzero(src, sizeof(struct sockaddr_storage));
				bzero(dst, sizeof(struct sockaddr_storage));

				ps = (struct sockaddr_in6 *)src;
				pd = (struct sockaddr_in6 *)dst;

				ps->sin6_family = AF_INET6;
				memcpy(&ps->sin6_addr, &ip6->ip6_src, sizeof(struct in6_addr));
				sport = &ps->sin6_port;

				pd->sin6_family = AF_INET6;
				memcpy(&pd->sin6_addr, &ip6->ip6_dst, sizeof(struct in6_addr));
				dport = &pd->sin6_port;
			}
			break;

		case IPPROTO_DSTOPTS:	/* destination option */
		case IPPROTO_ROUTING:	/* routing header */
			{
				struct ip6_ext * ip6ext;

				ip6ext = (struct ip6_ext *)(pkt + *offset);

				/* update nextproto and offset */
				nextproto = ip6ext->ip6e_nxt;
				*offset += ip6ext->ip6e_len << 3;

			}
			break;

		/* ignored */
		case IPPROTO_ICMP:
		case IPPROTO_ICMPV6:
		case IPPROTO_IGMP:
		case IPPROTO_UDP:
			return -1;

		default:
			/* unsupported proto */
			goto end;
		}
	}

end:
	log_msg(LOG_WARNING, "unsupported protocol dataframe (%d)", nextproto);

	return -1;
}
