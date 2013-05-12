#include "driftnet.h"

#include <string.h>
#include <assert.h>

#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#include <pcap.h>

#include "log.h"
#include "layer2.h"

int handle_link_layer(datalink_info_t *info, const u_char *pkt, uint8_t *nextproto,
			int *offsetnext)
{
	unsigned short llnextproto = 0;
	struct ethhdr *eptr;

	switch (info->type) {

		/* Ethernet packet */
		case DLT_EN10MB:

			eptr = (struct ethhdr *) pkt;
			*offsetnext = sizeof(struct ethhdr);
			llnextproto = ntohs(eptr->h_proto);

			switch (llnextproto) {
				case ETH_P_IP: /* Next header is IP */
					*nextproto = IPPROTO_IP;
					break;

				case ETH_P_IPV6: /* Next header is IPv6 */
					*nextproto = IPPROTO_IPV6;
					break;

				default:
					log_msg(LOG_WARNING,
							"link-level next protocol (%hd) is not supported",
							llnextproto);

					return -1;
			}

			break;

		default:
			/* unsupported packet */
			log_msg(LOG_WARNING, "link-level (%s) header is not supported",
					info->name);
			return -1;
	}

	return 0;
}
