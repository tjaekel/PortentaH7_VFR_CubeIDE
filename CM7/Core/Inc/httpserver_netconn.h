/**
 * httpserver_netconn.h
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#ifndef __HTTPSERVER_NETCONN_H__
#define __HTTPSERVER_NETCONN_H__

#include "lwip/api.h"

#define PRIVATE_IPADDR
/*Static IP ADDRESS*/
#ifdef PRIVATE_IPADDR
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   1
#define IP_ADDR3   169
#else
//Lab assigned IP address
#define IP_ADDR0   10
#define IP_ADDR1   59
#define IP_ADDR2   144
#define IP_ADDR3   26
#endif

#define IPADDR	((IP_ADDR3 << 24) | (IP_ADDR2 << 16) | (IP_ADDR1 << 8) | IP_ADDR0)

/*NETMASK*/
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

/*Gateway Address*/
#ifdef PRIVATE_IPADDR
#define GW_ADDR0   192
#define GW_ADDR1   168
#define GW_ADDR2   1
#define GW_ADDR3   1
#else
#define GW_ADDR0   10
#define GW_ADDR1   59
#define GW_ADDR2   144
#define GW_ADDR3   1
#endif

#define HTTPD_BUF_SIZE_OUT	(2*MEM_POOL_SEG_SIZE)		/* size for HTTPD output print buffer */
#define HTTPD_BUF_SIZE_IN	(4*(64 * 1024 + 32))		/* buffer for binary command input */

int HTTPD_OutInit(void);
void HTTPD_PrintOut(unsigned char *buf, int len);
void http_server_netconn_init(void);
void DynWebPage(struct netconn *conn);

#endif /* __HTTPSERVER_NETCONN_H__ */
