/**
 * @file network.h
 *
 * @brief Network packet capture handling.
 * @author David Suárez
 * @date Sun, 28 Oct 2018 16:14:56 +0100
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef DRIFTNET_NETWORK_H
#define DRIFTNET_NETWORK_H

#include <stddef.h>

#include "media/media.h"


/**
 * @brief list the network interfaces
 *
 * @return TRUE if all ok, FALSE if error
 */
int network_list_interfaces(void);

/**
 * @brief Gets the default network interface for capturing
 *
 * @return interface name
 */
char* network_get_default_interface(void);

/**
 * @brief Opens an interface for live capturing
 *
 * @param interface name of the interface
 * @param filterexpr bpf filter
 * @param promisc start in promisc mode ?
 * @param monitor_mode start in monitor mode ?
 * @return  TRUE if all ok, FALSE if error
 */
int network_open_live(char *interface, char *filterexpr, int promisc, int monitor_mode);

/**
 * @brief Opens a .pcap file for offline capturing
 *
 * @param dumpfile Path to dump file
 * @return TRUE if all ok, FALSE if error
 */
int network_open_offline(char *dumpfile);

/**
 * @brief Start capturing packets and handling inbound connections
 */
void network_start(mediadrv_t** drivers);

/**
 * @brief Stops the packet capturing
 */
void network_close(void);

#endif //DRIFTNET_NETWORK_H
