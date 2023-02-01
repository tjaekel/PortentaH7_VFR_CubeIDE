/**
 * httpserver_netconn.c
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "MEM_Pool.h"
#include "cmd_dec.h"
#include "VCP_UART.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "lwip/apps/netbiosns.h"

#include "httpserver_netconn.h"
#include <cmsis_os.h>

#include "debug_sys.h"
#include "SPI.h"
#include "I2C3_user.h"
#include "CHIP_cmd.h"

#define TCP_PORT	80			/* for our HTTPD */

#define WEBSERVER_THREAD_PRIO    ( osPriorityNormal ) //( osPriorityAboveNormal ) or ( osPriorityBelowNormal )

u32_t nPageHits = 0;

static int http_processCommand(struct netconn *conn, char *buf);

/* buffer for HTTPD output */
volatile size_t httpdOut_Idx = 0;
uint8_t *httpdOut = NULL;
uint8_t *httpdIn = NULL;
struct netconn *httpdOutConn = NULL;

int HTTPD_OutInit(void)
{
	/* ATT: we cannot make use of D3 - ETH DMA has an issue there ! */
	httpdOut = (uint8_t *)MEM_PoolAlloc(HTTPD_BUF_SIZE_OUT);
	httpdIn  = (uint8_t *)MEM_PoolAlloc(HTTPD_BUF_SIZE_IN);				/* header plus 64K + 16 */
	/* never freed if HTTPD was launched once */

	if (( ! httpdOut) || ( ! httpdIn))
	{
		SYS_SetError(SYS_ERR_SYSCFG);
		return 0;	//FAIL
	}
	else
		return 1;	//OK
}

void __attribute__((section(".itcmram"))) HTTPD_OutFlush(struct netconn *conn)
{
	httpdOut_Idx = 0;
	httpdOut[httpdOut_Idx] = '\0';
	httpdOutConn = conn;				//remember and set the HTTPD connection active now
}

uint8_t * __attribute__((section(".itcmram"))) HTTPD_GetOut(int *len)
{
	*len = httpdOut_Idx;
	return httpdOut;
}

void __attribute__((section(".itcmram"))) HTTPD_PrintOut(unsigned char *buf, int len)
{
	int l;
	if (httpdOut)
	{
		if (httpdOut_Idx < HTTPD_BUF_SIZE_OUT)
		{
			l = HTTPD_BUF_SIZE_OUT - httpdOut_Idx;
			if (len <= l)
				l = len;

			memcpy(&httpdOut[httpdOut_Idx], buf, l);

			httpdOut_Idx += l;
		}
	}
}

static void __attribute__((section(".itcmram"))) http_serve_generatePage(struct netconn *conn)
{
	struct fs_file file;
	char *xBuf;
	int len;

	fs_open(&file, "/STM32H7xx.html");
	/* TEXTAREA: we had to combine the parts now */
	netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
	fs_close(&file);

	/* send text from CMD result as TEXTAREA content  */
	xBuf = (char *)HTTPD_GetOut(&len);
	if (len)
		netconn_write(conn, xBuf, len, NETCONN_NOCOPY);

	fs_open(&file, "/STM32H7xx_b.html");
	netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
	fs_close(&file);
}

#if 0
static void http_serverErrorPage(struct netconn *conn)
{
#if 1
	struct fs_file file;

	fs_open(&file, "/404.html");
	netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len -1, NETCONN_NOCOPY);
	fs_close(&file);
#endif
}
#endif

static __attribute__((section(".itcmram"))) void http_server_sendResponse(struct netconn *conn, uint8_t *s, int l)
{
#define TCP_RESPONE_LEN			(32*1024 + 4)		//32768		//1460

	uint8_t *sx = s;
    while (l)
    {
   	 ////err_t e;
   	 if (l > TCP_RESPONE_LEN)
   	 {
   		 /*e =*/ netconn_write(conn, s, TCP_RESPONE_LEN, NETCONN_NOCOPY);
   		 s += TCP_RESPONE_LEN;
   		 l -= TCP_RESPONE_LEN;
   		 ////print_log(UART_OUT, "\r\nXX3a: %d | err: %x\r\n", 1460, e);
   		 ////osThreadYield();
   		 ////osDelay(1);
   	 }
   	 else
   	 {
   		 /*e =*/ netconn_write(conn, s, l, NETCONN_NOCOPY);
   		 ////print_log(UART_OUT, "\r\nXX3b: %d | err: %x\r\n", l, e);
   		 ////osThreadYield();
   		 ////osDelay(10);
   		 l = 0;
   	 }
    }

    if (GDebug & DBG_NETWORK)
    	hex_dump(sx, 12, 1, UART_OUT);
}

static __attribute__((section(".itcmram"))) void http_server_binMode_serve(struct netconn *conn, uint8_t *b, int len)
{
	if (len < 4)				//len is total length, with 4-byte header
		return;
	if (len > (HTTPD_BUF_SIZE_IN -4))
		len = HTTPD_BUF_SIZE_IN - 4;		//the max. payload length

	//do the command - b[0] is command, b[1..3] is binary length - little endian
	switch (b[0])
	{
	case 0x01:
		{
	    	uint8_t *s;
	        int l;

	        HTTPD_OutFlush(conn);

	        b[len] = '\0';                        			//ASCII NUL for CMD__DEC_execute(), terminated string
	        UART_Send((unsigned char *)"\000\000\000\000", 4, HTTPD_OUT);		//queue the binary response header - empty
	        if (len > 4)                                  	//only if anything available
	        {
	        	if (GDebug & DBG_LOG_BOTH)                  //log the command sent itself
	            {
	            	UART_Send((unsigned char *)"$ ", 2, UART_OUT);          //special prompt displayed
	                UART_Send(&b[4], len - 4, UART_OUT);     				//log the command
	                UART_Send((unsigned char *)"\r\n", 2, UART_OUT);
	            }
	            CMD_DEC_execute((char*)&b[4], HTTPD_OUT);   //execute the command
	        }
	        //update the response length
	        s = HTTPD_GetOut(&l);
	        //keep the same code in response
	        *(s + 0) = 0x01;								//same respond char command
	        *(s + 1) = (l - 4) >> 0;
	        *(s + 2) = (l - 4) >> 8;
	        *(s + 3) = (l - 4) >> 16;                       //Little Endian

	        http_server_sendResponse(conn, s, l);			//send response, with binary header
	    }
		break;
	case 0x02:
	    {
	    	//binary SPI
	    	SPI_TransmitReceive(&b[4], &b[4], len - 4);		//SPI transaction after CMD and LEN
	    	http_server_sendResponse(conn, b, len);         //send back the same updated buffer, we assume all OK
	    }
	    break;
	case 0x03:                                      		//back-to-back queued SPI transactions
	   	{
	        //every SPI transaction starts with a 2byte LEN field, after the total LEN of network packet
	        //iterate over all SPI transactions until entire LEN is reached, the LEN fields must be correct
	        unsigned short spiLen;
	        int idx = 4;                                	//idx to our length field for single SPI, after expBinLen as total
	        int expBinLen;									//keep len for sending response

	        expBinLen = len - 4;
	        while (expBinLen > 0)                       	//loop over all SPI transactions
	        {
	        	/* ATT: this can fail! NSS is too early, before SCLK is stable (SPI Mode)! */
	        	if ((GSysCfg.SPIcfg & CFG_SPI_CFG_EARLY) >> 8)
	        		HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_RESET);			//early CSB - OPTIONAL

	        	spiLen  = b[idx + 0] << 0;              	//Little Endian
	            spiLen |= b[idx + 1] << 8;

	            //do the SPI transaction
	            //if (spiLen)								//we skip to save time
	            {
	            	SPI_TransmitReceive(&b[idx + 2], &b[idx + 2], spiLen);  	//it will update same buffer
	            }
	            expBinLen -= spiLen + 2;
	            idx += 2 + spiLen;                      	//move to next idx field
	        }
	        http_server_sendResponse(conn, b, len);         //send back the same updated buffer, we assume all OK
	    }
	   	break;
	case 0x04 :
		{
			/* I2C write 0x04 : slaveAddr, bytes (1st is register address) */
			uint16_t i2cLen = 0;
			if (len >= (4 + 1))			//at least slaveAddr provided?
			{
				uint16_t slaveAddr = (uint16_t)b[4];
				if (I2CUser_Write(slaveAddr, &b[5], len - 4 - 1))
					i2cLen = 0;				//ERROR
				else
					i2cLen = len - 4;		//OK, the same length as provided
			}
			b[1] = (uint8_t)(i2cLen >> 0);	//Little Endian
			b[2] = (uint8_t)(i2cLen >> 8);
			b[3] = 0;
			//update the len for values written - the command code is still in b[0] and all in b[]
			http_server_sendResponse(conn, b, (int)i2cLen + 4);
		}
		break;
	case 0x05 :
		{
			/* I2C memread 0x05: slaveAddr, regAddr, numbytes */
			uint16_t i2cLen = 0;
			if (len >= (4 + 3))			//all needed parameters provided?
			{
				uint16_t slaveAddr = (uint16_t)b[4];
				uint16_t regAddr   = (uint16_t)b[5];
				i2cLen    		   = (uint16_t)b[6];

				if (I2CUser_MemReadEx(slaveAddr, regAddr, &b[4], i2cLen))
					i2cLen = 0;				//ERROR
			}
			//update the len for values read - the command code is still in b[0]
			b[1] = (uint8_t)(i2cLen >> 0);	//Little Endian
			b[2] = (uint8_t)(i2cLen >> 8);
			b[3] = 0;

			http_server_sendResponse(conn, b, (int)i2cLen + 4);
		}
		break;

	case 0x06 :			//SPI RREG: length in bytes, reuse b and align for SPI transaction
	{
		if (GSysCfg.ChipNo == 1)
		{
			//set the command: align the buffer b in a way that MISO values are payload
			//payload in b is: 32bit address + 16bit number for num of registers to read
			int num, xNum;
			uint32_t addr;
			uint16_t *spiTx;

			//if 32bit aligned - we could read directly
			addr  = (b[4] <<  0);				//addr LITTLE ENDIAN, 32bit
			addr |= (b[5] <<  8);
			addr |= (b[6] << 16);
			addr |= (b[7] << 24);

			num  = (b[8]  <<  0);				//LITTLE ENDIAN, 32bit, in num words to read
			num |= (b[9]  <<  8);
			num |= (b[10] << 16);
			num |= (b[11] << 24);

			if (num > ((HTTPD_BUF_SIZE_IN - 4) / 2))
				num = (HTTPD_BUF_SIZE_IN - 4) / 2;			//max. number words possible based on buffer size

			spiTx = (uint16_t *)&b[2];			//must be 16bit aligned!, but not word aligned for SPI!
			//command:
			*spiTx    = SPI_REG_ADDR_SHIFT(SPI_REG_ADDR_MASK(addr));
			*spiTx++ |= SPI_RREG_CMD;

			xNum = num;
			while (num--)
			{
				*spiTx++ = GSysCfg.SPInoop;
			}
			SPI_TransmitReceive(&b[2], &b[2], xNum*2 + 2);

			//update the length: b[0] is still command, payload length in bytes
			b[1] = (xNum*2) & 0xFF;
			b[2] = ((xNum*2) >> 8);
			b[3] = ((xNum*2) >> 16);

			http_server_sendResponse(conn, b, (int)(xNum*2) + 4);
		}
	}
	break;

	case 0x07 :			//SPI WREG: length in bytes, take b words for SPI transaction, place SPI command on length field
	{
		if (GSysCfg.ChipNo == 1)
		{
			uint32_t addr;
			uint16_t *spiTx;

			//if 32bit aligned - we could read directly
			addr  = (b[4] <<  0);				//addr LITTLE ENDIAN, 32bit
			addr |= (b[5] <<  8);
			addr |= (b[6] << 16);
			addr |= (b[7] << 24);

			spiTx = (uint16_t *)&b[6];			//must be 16bit aligned!, but not word aligned for SPI!
			//command:
			*spiTx  = SPI_REG_ADDR_SHIFT(SPI_REG_ADDR_MASK(addr));
			*spiTx |= SPI_WREG_CMD;

			SPI_TransmitReceive(&b[6], &b[6], len - 4 - 4 + 2);		//HDR and addr bytes. 2 bytes SPI cmd

			//update the length: b[0] is still command, payload length in bytes - nothing on write
			b[1] = 0;
			b[2] = 0;
			b[3] = 0;

			http_server_sendResponse(conn, b, 4);
		}
	}
	break;

	case 0x08 :			//SPI RBLK
	{
		if (GSysCfg.ChipNo == 1)
		{
			int num, xNum;
			uint32_t addr;
			uint16_t *spiTx;

			//if 32bit aligned - we could read directly
			addr  = (b[4] <<  0);				//addr LITTLE ENDIAN, 32bit
			addr |= (b[5] <<  8);
			addr |= (b[6] << 16);
			addr |= (b[7] << 24);

			num  = (b[8]  <<  0);				//LITTLE ENDIAN, 32bit, in num words to read
			num |= (b[9]  <<  8);
			num |= (b[10] << 16);
			num |= (b[11] << 24);

			if (num > ((HTTPD_BUF_SIZE_IN - 4) / 2))
				num = (HTTPD_BUF_SIZE_IN - 4) / 2;		//max. number words possible based on buffer size

			spiTx = (uint16_t *)&b[2];					//must be 16bit aligned!, but not word aligned for SPI!
			//command:
			*spiTx    = SPI_BLK_ADDR_SHIFT(SPI_BLK_ADDR_MASK(addr));
			*spiTx++ |= SPI_RBLK_CMD;

			xNum = num;
			while (num--)
			{
				*spiTx++ = GSysCfg.SPInoop;
			}
			SPI_TransmitReceive(&b[2], &b[2], xNum*2 + 2);

			if (GDebug & DBG_NET_BYTE)
			{
				print_log(UART_OUT, "\r\n*D: SPI Rx:\r\n");
				hex_dump(&b[4], xNum*2, 1, UART_OUT);
			}
			if (GDebug & DBG_NET_12BIT)
			{
				print_log(UART_OUT, "\r\n*D: SPI Rx:\r\n");
				hex_dump(&b[4], xNum*2, 3, UART_OUT);
			}

			//update the length: b[0] is still command, payload length in bytes
			b[1] = (xNum*2) & 0xFF;
			b[2] = ((xNum*2) >>  8);
			b[3] = ((xNum*2) >> 16);

			http_server_sendResponse(conn, b, (int)(xNum*2) + 4);
		}
	}
	break;

	case 0x09 :			//SPI WBLK
	{
		if (GSysCfg.ChipNo == 1)
		{
			uint32_t addr;
			uint16_t *spiTx;

			//if 32bit aligned - we could read directly
			addr  = (b[4] <<  0);				//addr LITTLE ENDIAN, 32bit
			addr |= (b[5] <<  8);
			addr |= (b[6] << 16);
			addr |= (b[7] << 24);

			spiTx = (uint16_t *)&b[6];			//must be 16bit aligned!, but not word aligned for SPI!
			//command:
			*spiTx  = SPI_BLK_ADDR_SHIFT(SPI_BLK_ADDR_MASK(addr));
			*spiTx |= SPI_WBLK_CMD;

			if (GDebug & DBG_NET_BYTE)
			{
				print_log(UART_OUT, "\r\n*D: SPI Tx:\r\n");
				hex_dump(&b[8], len - 4 - 4, 1, UART_OUT);
			}
			if (GDebug & DBG_NET_12BIT)
			{
				print_log(UART_OUT, "\r\n*D: SPI Tx:\r\n");
				hex_dump(&b[8], len - 4 - 4, 3, UART_OUT);
			}

			SPI_TransmitReceive(&b[6], &b[6], len - 4 - 4 + 2);		//HDR and addr bytes, 2 bytes SPI cmd

			//update the length: b[0] is still command, payload length in bytes - nothing on write
			b[1] = 0;
			b[2] = 0;
			b[3] = 0;

			http_server_sendResponse(conn, b, 4);
		}
	}
	break;

	case 0x0A :			//NOOP, with number of noops
	{
		if (GSysCfg.ChipNo == 1)
		{
			uint32_t num, xNum;
			uint16_t *spiTx;

			//if 32bit aligned - we could read directly
			num  = (b[4] <<  0);				//num LITTLE ENDIAN, 32bit
			num |= (b[5] <<  8);
			num |= (b[6] << 16);
			num |= (b[7] << 24);

			if (num > ((len - 4) / 2))
				num = (len - 4) / 2;			//max. number words possible based on buffer size

			spiTx = (uint16_t *)&b[2];			//must be 16bit aligned!, but not word aligned for SPI!
			//command:
			*spiTx  = 0;
			*spiTx++ |= SPI_NOOP_CMD;

			xNum = num;
			while (num--)
			{
				*spiTx++ = GSysCfg.SPInoop;
			}
			SPI_TransmitReceive(&b[2], &b[2], xNum*2 + 2);

			//update the length: b[0] is still command, payload length in bytes - nothing on write
			b[1] = 0;
			b[2] = 0;
			b[3] = 0;

			http_server_sendResponse(conn, b, 4);
		}
	}
	break;

	default :												//send the same back, unmodified, measure ETH throughput
			http_server_sendResponse(conn, b, len);
	}
}

/**
  * @brief serve tcp connection  
  * @param conn: pointer on connection structure 
  * @retval None
  */
static __attribute__((section(".itcmram"))) int http_server_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  err_t recv_err;
  char* buf;
  uint8_t *s;
  u16_t buflen;
  int idx;
  int binMode;
  int expBinLen;
  
  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  idx = 0;
  binMode = 0;
  do {
	  recv_err = netconn_recv(conn, &inbuf);
	  if (recv_err == ERR_OK)
	  {
	    	  netbuf_data(inbuf, (void**)&buf, &buflen);
	    	  memcpy(&httpdIn[idx], buf, buflen);
	    	  netbuf_delete(inbuf);

	    	  if (GDebug & DBG_NETWORK)
	    	  {
	    		  print_log(UART_OUT, "\r\n*D: net packet %d | %d\r\n", idx, buflen);
	    		  hex_dump(httpdIn, 12, 1, UART_OUT);
	    	  }

	    	  if ((idx + buflen) <= HTTPD_BUF_SIZE_IN)
	    		  idx += buflen;
	    	  else
	    	  {
	    		  //error - we overflow the input buffer
	    		  netconn_close(conn);
	    		  SYS_SetError(SYS_ERR_OVERFLOW);
	    		  if (GDebug & DBG_NETWORK)
	    			  print_log(UART_OUT, "\r\n*E: network buffer overflow\r\n");
	    		  return 1;
	    	  }

	    	  if (idx == buflen)				//for the first time, on first packet
	    	  {
	    		  if (httpdIn[0] < ' ')
	    		  {
	    			  binMode = 1;
	    			  //get the binary length: LITTLE ENDIAN, 3 bytes
	    			  expBinLen  = httpdIn[1] <<  0;
	    			  expBinLen |= httpdIn[2] <<  8;
	    			  expBinLen |= httpdIn[3] << 16;
	    			  ////print_log(UART_OUT, "XX: bin mode: %d\r\n", expBinLen);
	    		  }
	    	  }

	    	  if ( ! binMode)
	    	  {
	    		  httpdIn[idx] = '\0';						//terminate the string
	    		  s = (uint8_t *)strstr((const char*)buf, "\r\n\r\n");
	    		  if (s)
	    		  {
	    			  *s = '\0';
	    			  break;			//ASCII commands - continue afterwards
	    		  }
	    	  }
	    	  else
	    	  {
	    		  if (idx >= (expBinLen + 4))
	    		  {
	    			  //now we have a complete binary command packet - process it
	    			  http_server_binMode_serve(conn, httpdIn, idx);		//idx is total length of packet

	    			  //do not close connection
	    			  return 0;			//do not close
	    		  }
	    	  }

	      osThreadYield();
	  }
	  else
	  {
		  netconn_close(conn);
		  ////print_log(UART_OUT, "\r\n*E: network receiver error, connection closed\r\n");
		  return 1;
	  }
  } while (1);

      /* Is this an HTTP GET command? (only check the first 5 chars, since
      there are other formats for GET, and we're keeping it very simple )*/
      if ((buflen >=5) && (strncmp((char *)httpdIn, "GET /", 5) == 0))
      {
    	HTTPD_OutFlush(conn);

    	if (strncmp((const char*)&httpdIn[5], "favicon.ico", 11) == 0)
    	{
    	    netconn_write(conn, "HTTP/1.1 404 not supported", 26, NETCONN_NOCOPY);
    	    goto CLOSE_HTTPD;
    	}

        if ((strncmp((char *)httpdIn, "GET /STM32H7xx.html", 19) == 0) || (strncmp((char *)httpdIn, "GET / ", 6) == 0))
        {
        	if (strncmp((char *)httpdIn, "GET /STM32H7xx.html?CMD=", 24) == 0)
        	{
        	    http_processCommand(conn, (char *)&httpdIn[24]);
        	}

        	/* Load STM32H7xx page */
        	http_serve_generatePage(conn);
        }
        else if ((strncmp((char *)httpdIn, "GET /softreset.html", 17) == 0) || (strncmp((char *)httpdIn, "GET / ", 6) == 0))
        {
        	extern ECMD_DEC_Status CMD_fwreset(TCMD_DEC_Results *res, EResultOut out);

        	/* TODO: send a SPI soft reset command to the chip */
        	//we send a fwreset to MCU
        	CMD_fwreset(NULL, SILENT);

        	/* actually, we do not reach this code - MCU resets itself */
        	http_serve_generatePage(conn);
         }
         else if((strncmp((char *)httpdIn, "GET /hardreset.html", 15) == 0) || (strncmp((char *)httpdIn, "GET / ", 6) == 0))
         {
#if 0
            /* toggle both reset signals */
#endif

        	http_serve_generatePage(conn);
        }
        else 
        {
#if 0
        	/* Load Error page */
        	http_serverErrorPage(conn);
        	/* we handle now all other requests as URL from a Web Browser */
#else
        	/* assume GET/any_ASCII_command - Web Browser w/o a full page */
            //ASCII command, now execute command
             int l;

             HTTPD_OutFlush(conn);				//might be done already
             //ATT: this format is very important for web browser!
             UART_printString((unsigned char *)"HTTP/1.1 200 OK\r\nContent-type:text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html><body><pre>\r\n", HTTPD_OUT);
             l = strlen((const char*)&httpdIn[5]);
             if (l)
                 http_processCommand(conn, (char *)&httpdIn[5]);

             //send response - we put all together in HTTPD_OUT buffer
             //ATT: every single netconn_write is a new TCP handshake
             UART_printString((unsigned char *)"</pre></body></html>\r\n\r\n\003", HTTPD_OUT);
             s = HTTPD_GetOut(&l);

             ////print_log(UART_OUT, "\r\nXX2: %d\r\n", l);
             http_server_sendResponse(conn, s, l);
#endif
        }
      }      

CLOSE_HTTPD:
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  return 1;			//close connection
}


/**
  * @brief  http server thread 
  * @param arg: pointer on argument(not used here) 
  * @retval None
  */
static __attribute__((section(".itcmram"))) void http_server_netconn_thread(void *arg)
{ 
  (void)arg;

  struct netconn *conn, *newconn;
  err_t err, accept_err;
  
  /* start NetBIOS */
  netbiosns_init();
  netbiosns_set_name("P7MCU01");

  /* Create a new TCP connection handle */
  conn = netconn_new(NETCONN_TCP);
  
  if (conn!= NULL)
  {
    /* Bind to port 80 (HTTP) with default IP address */
    err = netconn_bind(conn, NULL, TCP_PORT);
    
    if (err == ERR_OK)
    {
      /* Put the connection into LISTEN state */
      netconn_listen(conn);
  
      while(1) 
      {
    	int r;

        /* accept any incoming connection */
        accept_err = netconn_accept(conn, &newconn);
        if (accept_err == ERR_OK)
        {
        	do {
        		/* serve connection */
        		r = http_server_serve(newconn);

        		/* delete connection */
        		if (r)
        		{
        			netconn_delete(newconn);
        			if (GDebug & DBG_NETWORK)
        				print_log(UART_OUT, "\r\n*I: connection closed\r\n");
        		}
        	} while (r == 0);
        }
        else
        {
        	SYS_SetError(SYS_ERR_NETWORK);
        	if (GDebug & DBG_NETWORK)
        		print_log(UART_OUT, "\r\n*E: connection error: %d\r\n", accept_err);
        }
      }
    }
  }
}

/**
  * @brief  Initialize the HTTP server (start its thread) 
  * @param  none
  * @retval None
  */
void http_server_netconn_init()
{
  sys_thread_new("HTTP", http_server_netconn_thread, NULL, DEFAULT_THREAD_STACKSIZE, WEBSERVER_THREAD_PRIO);
}

static int http_processCommand(struct netconn *conn, char *buf)
{
	char *xBuf;

	xBuf = buf;
	//process the command now, start is in xBuf, ATT: the spaces are + signs, ? is %3F, ; is %3B etc.

	/* remark: we use the HTTP input request buffer */
	/* get rid of " HTTP" */
	xBuf = strstr(buf, " HTTP");
	if (xBuf)
		*xBuf = '\0';

	convertURL(buf, buf, strlen(buf));

	/* now we should have in buf our command string for the command interpreter */
	////HTTPD_OutFlush(conn);		//already done
	if (GDebug & DBG_LOG_BOTH)                  //log the command sent itself
	    print_log(UART_OUT, "$ %s\r\n", buf);

	CMD_DEC_execute(buf, HTTPD_OUT);

	return 1;
}
