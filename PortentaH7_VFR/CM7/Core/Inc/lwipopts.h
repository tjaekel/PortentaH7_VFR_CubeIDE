/**
 * lwipopts.h
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#ifndef __LWIPOPTS__H__
#define __LWIPOPTS__H__

#include "main.h"

/*-----------------------------------------------------------------------------*/
/* Current version of LwIP supported by CubeMx: 2.1.2 -*/
/*-----------------------------------------------------------------------------*/

#ifdef __cplusplus
 extern "C" {
#endif

 /**
  * NO_SYS==1: Provides VERY minimal functionality. Otherwise,
  * use lwIP facilities.
  */
 #define NO_SYS                  0

 /* ---------- Memory options ---------- */
 /* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
    lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
    byte alignment -> define MEM_ALIGNMENT to 2. */
 #define MEM_ALIGNMENT           4

 /* MEM_SIZE: the size of the heap memory. If the application will send
 a lot of data that needs to be copied, this should be set high. */
 /*
  * even we have 32KB free - it does not work with full 32K!
  */
 #define MEM_SIZE                32752L		//9000L		//(32K - 16!!!)  XXXXX ! crashes!
 /* REMARK: based on linker script: we have 32K Tx and Rx buffers, 21 descriptors for Rx, Tx */

 /* Relocate the LwIP RAM heap pointer: LwIPHeapSection */
 ////#define LWIP_RAM_HEAP_POINTER   (0x30040000) /* see MPU config - max. 32KB left, for Tx Buffers */

 /* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
    sends a lot of data out of ROM (or other static memory), this
    should be set high. */
 #define MEMP_NUM_PBUF           12

 /* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
    per active UDP "connection". */
 #define MEMP_NUM_UDP_PCB        4

 /* MEMP_NUM_TCP_PCB: the number of simultaneously active TCP
    connections. */
 #define MEMP_NUM_TCP_PCB        2

 /* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP
    connections. */
 #define MEMP_NUM_TCP_PCB_LISTEN 2

 /* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
    segments. */
 #define MEMP_NUM_TCP_SEG        16

 /* MEMP_NUM_SYS_TIMEOUT: the number of simultaneously active
    timeouts. */
 ////#define MEMP_NUM_SYS_TIMEOUT    8

 /* ---------- Fragmentation handling ------------- */
 #define IP_FRAG				1
 #define IP_REASSEMBLY   		1
 #define IP_OPTIONS_ALLOWED   	1

 /* ---------- Pbuf options ---------- */
 /* PBUF_POOL_SIZE: the number of buffers in the pbuf pool.
    @ note: used to allocate Tx pbufs only */
 #define PBUF_POOL_SIZE          21

 /* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. */
 #define PBUF_POOL_BUFSIZE       1528	//1528

 /* LWIP_SUPPORT_CUSTOM_PBUF == 1: to pass directly MAC Rx buffers to the stack
    no copy is needed */
 #define LWIP_SUPPORT_CUSTOM_PBUF      1

 /* ---------- IPv4 options ---------- */
 #define LWIP_IPV4                1

 /* ---------- TCP options ---------- */
 #define LWIP_TCP                1
 #define TCP_TTL                 255

 /* Controls if TCP should queue segments that arrive out of
    order. Define to 0 if your device is low on memory. */
 #define TCP_QUEUE_OOSEQ         1

 /* TCP Maximum segment size. */
 #define TCP_MSS                 (1500 - 40)	  /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */

 /* TCP sender buffer space (bytes). */
 #define TCP_SND_BUF             (4*TCP_MSS)

 /* TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
    as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work. */
 #define TCP_SND_QUEUELEN        (2*TCP_SND_BUF/TCP_MSS)
 /* multiply of TCP_SND_BUF and TCP_SND_QUEUELEN must be <= MEMP_NUM_TCP_SEG*/

 /* TCP receive window. */
 #define TCP_WND                 (4*TCP_MSS)

 /* ---------- ICMP options ---------- */
 #define LWIP_ICMP               1


 /* ---------- DHCP options ---------- */
 #define LWIP_DHCP               1

 /* ---------- UDP options ---------- */
 #define LWIP_UDP                1
 #define UDP_TTL                 255

 /* ---------- link callback options ---------- */
 /* LWIP_NETIF_LINK_CALLBACK==1: Support a callback function from an interface
  * whenever the link changes (i.e., link down)
  */
 #define LWIP_NETIF_LINK_CALLBACK        1

/* STM32CubeMX Specific Parameters (not defined in opt.h) ---------------------*/
/* Parameters set in STM32CubeMX LwIP Configuration GUI -*/
/*----- WITH_RTOS enabled (Since FREERTOS is set) -----*/
#define WITH_RTOS 1
/*----- CHECKSUM_BY_HARDWARE enabled -----*/
#define CHECKSUM_BY_HARDWARE 1
/*-----------------------------------------------------------------------------*/

/* LwIP Stack Parameters (modified compared to initialization value in opt.h) -*/
/* Parameters set in STM32CubeMX LwIP Configuration GUI -*/
/*----- Value in opt.h for LWIP_DHCP: 0 -----*/
////#define LWIP_DHCP 1

/*----- Default value in ETH configuration GUI in CubeMx: 1524 -----*/
#define ETH_RX_BUFFER_SIZE 1536
/*----- Value in opt.h for MEM_ALIGNMENT: 1 -----*/
#define MEM_ALIGNMENT 4
/*----- Default Value for H7 devices: 0x30044000 -----*/
#define LWIP_RAM_HEAP_POINTER 0x30040000				//changed: see MPCU config and linker script
/*----- Value in opt.h for MEMP_NUM_SYS_TIMEOUT: (LWIP_TCP + IP_REASSEMBLY + LWIP_ARP + (2*LWIP_DHCP) + LWIP_AUTOIP + LWIP_IGMP + LWIP_DNS + (PPP_SUPPORT*6*MEMP_NUM_PPP_PCB) + (LWIP_IPV6 ? (1 + LWIP_IPV6_REASS + LWIP_IPV6_MLD) : 0)) -*/
#define MEMP_NUM_SYS_TIMEOUT 5
/*----- Value supported for H7 devices: 1 -----*/
#define LWIP_SUPPORT_CUSTOM_PBUF 1
/*----- Value in opt.h for LWIP_ETHERNET: LWIP_ARP || PPPOE_SUPPORT -*/
#define LWIP_ETHERNET 1
/*----- Value in opt.h for LWIP_DNS_SECURE: (LWIP_DNS_SECURE_RAND_XID | LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING | LWIP_DNS_SECURE_RAND_SRC_PORT) -*/
#define LWIP_DNS_SECURE 7
/*----- Value in opt.h for TCP_SND_QUEUELEN: (4*TCP_SND_BUF + (TCP_MSS - 1))/TCP_MSS -----*/
////#define TCP_SND_QUEUELEN 9
/*----- Value in opt.h for TCP_SNDLOWAT: LWIP_MIN(LWIP_MAX(((TCP_SND_BUF)/2), (2 * TCP_MSS) + 1), (TCP_SND_BUF) - 1) -*/
#define TCP_SNDLOWAT 1071
/*----- Value in opt.h for TCP_SNDQUEUELOWAT: LWIP_MAX(TCP_SND_QUEUELEN)/2, 5) -*/
#define TCP_SNDQUEUELOWAT 5
/*----- Value in opt.h for TCP_WND_UPDATE_THRESHOLD: LWIP_MIN(TCP_WND/4, TCP_MSS*4) -----*/
#define TCP_WND_UPDATE_THRESHOLD 536
/*----- Value in opt.h for LWIP_NETIF_LINK_CALLBACK: 0 -----*/
#define LWIP_NETIF_LINK_CALLBACK 1
/*----- Value in opt.h for TCPIP_THREAD_STACKSIZE: 0 -----*/
#define TCPIP_THREAD_STACKSIZE 1024
/*----- Value in opt.h for TCPIP_THREAD_PRIO: 1 -----*/
#define TCPIP_THREAD_PRIO 24		//24 = Normal, 32 = AboveNormal
/*----- Value in opt.h for TCPIP_MBOX_SIZE: 0 -----*/
#define TCPIP_MBOX_SIZE 6
/*----- Value in opt.h for SLIPIF_THREAD_STACKSIZE: 0 -----*/
#define SLIPIF_THREAD_STACKSIZE 1024			//XXXX
/*----- Value in opt.h for SLIPIF_THREAD_PRIO: 1 -----*/
#define SLIPIF_THREAD_PRIO 3
/*----- Value in opt.h for DEFAULT_THREAD_STACKSIZE: 0 -----*/
#define DEFAULT_THREAD_STACKSIZE 1024
/*----- Value in opt.h for DEFAULT_THREAD_PRIO: 1 -----*/
#define DEFAULT_THREAD_PRIO 3
/*----- Value in opt.h for DEFAULT_UDP_RECVMBOX_SIZE: 0 -----*/
#define DEFAULT_UDP_RECVMBOX_SIZE 6
/*----- Value in opt.h for DEFAULT_TCP_RECVMBOX_SIZE: 0 -----*/
#define DEFAULT_TCP_RECVMBOX_SIZE 6
/*----- Value in opt.h for DEFAULT_ACCEPTMBOX_SIZE: 0 -----*/
#define DEFAULT_ACCEPTMBOX_SIZE 6
/*----- Value in opt.h for RECV_BUFSIZE_DEFAULT: INT_MAX -----*/
#define RECV_BUFSIZE_DEFAULT 2000000000
/*----- Value in opt.h for LWIP_STATS: 1 -----*/
#define LWIP_STATS 0
/*----- Value in opt.h for CHECKSUM_GEN_IP: 1 -----*/
#define CHECKSUM_GEN_IP 0
/*----- Value in opt.h for CHECKSUM_GEN_UDP: 1 -----*/
#define CHECKSUM_GEN_UDP 0
/*----- Value in opt.h for CHECKSUM_GEN_TCP: 1 -----*/
#define CHECKSUM_GEN_TCP 0
/*----- Value in opt.h for CHECKSUM_GEN_ICMP6: 1 -----*/
#define CHECKSUM_GEN_ICMP6 0
/*----- Value in opt.h for CHECKSUM_CHECK_IP: 1 -----*/
#define CHECKSUM_CHECK_IP 0
/*----- Value in opt.h for CHECKSUM_CHECK_UDP: 1 -----*/
#define CHECKSUM_CHECK_UDP 0
/*----- Value in opt.h for CHECKSUM_CHECK_TCP: 1 -----*/
#define CHECKSUM_CHECK_TCP 0
/*----- Value in opt.h for CHECKSUM_CHECK_ICMP6: 1 -----*/
#define CHECKSUM_CHECK_ICMP6 0
/*-----------------------------------------------------------------------------*/
 /*
    ----------------------------------------------
    ---------- Sequential layer options ----------
    ----------------------------------------------
 */
 /**
  * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
  */
 #define LWIP_NETCONN                    1

 /*
    ------------------------------------
    ---------- Socket options ----------
    ------------------------------------
 */
 /**
  * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
  */
 #define LWIP_SOCKET                     0

 /*
    ------------------------------------
    ---------- httpd options ----------
    ------------------------------------
 */
 /** Set this to 1 to include "fsdata_custom.c" instead of "fsdata.c" for the
  * file system (to prevent changing the file included in CVS) */
#define HTTPD_USE_CUSTOM_FSDATA   1


#ifdef __cplusplus
}
#endif
#endif /*__LWIPOPTS__H__ */
