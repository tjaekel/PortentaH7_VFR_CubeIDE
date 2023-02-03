/**
 * app_ethernet.h
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#ifndef __APP_ETHERNET_H
#define __APP_ETHERNET_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "lwip/netif.h"

/* DHCP process states */
#define DHCP_OFF                   (uint8_t) 0
#define DHCP_START                 (uint8_t) 1
#define DHCP_WAIT_ADDRESS          (uint8_t) 2
#define DHCP_ADDRESS_ASSIGNED      (uint8_t) 3
#define DHCP_TIMEOUT               (uint8_t) 4
#define DHCP_LINK_DOWN             (uint8_t) 5

void ethernet_link_status_updated(struct netif *netif);
#if LWIP_DHCP
void DHCP_Thread(void *argument);
#endif  

unsigned long ETH_GetIPAddr(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_ETHERNET_H */
