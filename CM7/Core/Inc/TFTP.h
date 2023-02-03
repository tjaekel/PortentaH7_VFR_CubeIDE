/**
 * tftp.h
 *
 *  Created on: May 22, 2018
 *  Author: Torsten Jaekel
 */

#ifndef __TFTP_H__
#define __TFTP_H__

#include "fatfs.h"
#include "lwip/tcpip.h"
#include "ethernetif.h"
#include "app_ethernet.h"

#include "lwip/apps/tftp_server.h"

extern const struct tftp_context TFTPctx;

#endif
