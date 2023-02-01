/**
 * ethernetif.h
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "lwip/err.h"
#include "lwip/netif.h"
#include "cmsis_os.h"

err_t ethernetif_init(struct netif *netif);

void ethernetif_input(void* argument);
void ethernet_link_thread(void* argument );

u32_t sys_jiffies(void);
u32_t sys_now(void);

#endif
