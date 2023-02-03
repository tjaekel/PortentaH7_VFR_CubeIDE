/**
 * app_ethernet.c
 *
 *  Created on: Jul 10, 2022
 *      Author: Torsten Jaekel
 */

#include "lwip/opt.h"
#include "main.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif
#include "app_ethernet.h"
#include "ethernetif.h"

#include "httpserver_netconn.h"
#include "syserr.h"

unsigned long gIPAddr = 0;
char GHostName[10] = {""};	//max. 16 characters, we use 5 + 4 + NUL

#if LWIP_DHCP
#define MAX_DHCP_TRIES  4
__IO uint8_t DHCP_state = DHCP_OFF;
#endif

/**
  * @brief  Notify the User about the network interface config status
  * @param  netif: the network interface
  * @retval None
  */
void ethernet_link_status_updated(struct netif *netif) 
{
  if (netif_is_up(netif))
 {
#if LWIP_DHCP
    /* Update DHCP state machine */
    DHCP_state = DHCP_START;
#else
    BSP_LED_On(LED2);
    BSP_LED_Off(LED_RED);
#endif /* LWIP_DHCP */
  }
  else
  {  
#if LWIP_DHCP
    /* Update DHCP state machine */
    DHCP_state = DHCP_LINK_DOWN;
    SYS_SetError(SYS_ERR_NETWORK);
#else    
    BSP_LED_Off(LED2);
    BSP_LED_On(LED_RED);
#endif /* LWIP_DHCP */
  } 
}

#if LWIP_DHCP
/**
  * @brief  DHCP Process
  * @param  argument: network interface
  * @retval None
  */
void DHCP_Thread(void *argument)
{
  struct netif *netif = (struct netif *) argument;
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
  struct dhcp *dhcp;
  
  for (;;)
  {
    switch (DHCP_state)
    {
      case DHCP_START:
      {
        ip_addr_set_zero_ip4(&netif->ip_addr);
        ip_addr_set_zero_ip4(&netif->netmask);
        ip_addr_set_zero_ip4(&netif->gw);    
        DHCP_state = DHCP_WAIT_ADDRESS;
        
        dhcp_start(netif);
      }
      break;    
      case DHCP_WAIT_ADDRESS:
      {                
        if (dhcp_supplied_address(netif)) 
        {
          DHCP_state = DHCP_ADDRESS_ASSIGNED;	
         
          /* address via DHCP */
          gIPAddr = netif->ip_addr.addr;

          HAL_GPIO_WritePin(GPIOK, GPIO_PIN_7, GPIO_PIN_RESET);	//blue LED on
        }
        else
        {
          osThreadYield();
          dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
    
          /* DHCP timeout */
          if (dhcp->tries >= (MAX_DHCP_TRIES - 1))
          {
            DHCP_state = DHCP_TIMEOUT;
            
            /* Stop DHCP */
            dhcp_stop(netif);
            
            /* Static address used */
            IP_ADDR4(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
            IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
            IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
            netif_set_addr(netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));

            /* address via STATIC - HDCP has failed */
            gIPAddr = (unsigned long)ipaddr.addr;

            SYS_SetError(SYS_ERR_DHCPTO);
          }
        }
    }
    break;
    case DHCP_LINK_DOWN:
    {
      /* Stop DHCP */
      dhcp_stop(netif);
      DHCP_state = DHCP_OFF;

      gIPAddr = 0;

      SYS_SetError(SYS_ERR_ETHDOWN);
    }
    break;
    case DHCP_OFF:
    case DHCP_ADDRESS_ASSIGNED:
    case DHCP_TIMEOUT:
    {
    	/* kill the DHCP thread */
    	osThreadTerminate(osThreadGetId());
    }
    break;
    default: break;
    }
    
    /* wait 500 ms */
    osDelay(500);
  }
}
#endif  /* LWIP_DHCP */

unsigned long ETH_GetIPAddr(void)
{
	return gIPAddr;
}
