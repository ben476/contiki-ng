#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>

#include "sys/log.h"
#include "uip-ds6.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 0
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define SEND_INTERVAL ((int)(CLOCK_SECOND))

static struct simple_udp_connection udp_conn;
static uint32_t rx_count = 0;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr,
                uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr,
                uint16_t receiver_port,
                const uint8_t *data,
                uint16_t datalen)
{

  LOG_INFO("Received response '%.*s' from ", datalen, (char *)data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
  rx_count++;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[6000];
  uip_ipaddr_t dest_ipaddr;
  static uint32_t tx_count;
  static uint32_t missed_tx_count;
  static uint32_t tx_bytes;

  PROCESS_BEGIN();

  // LOG_INFO("CLOCK_SECOND = %ld\n", CLOCK_SECOND);

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, 0);
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if (NETSTACK_ROUTING.node_is_reachable() &&
        NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr))
    {

      /* Print statistics every 10th TX */
      /* Print statistics every 10th TX */
      // if (tx_count % 10 == 0)
      // {
      // LOG_INFO("Tx/Rx/MissedTx/Bytes: %" PRIu32 "/%" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
      //          tx_count, rx_count, missed_tx_count, tx_bytes);
      // }

      /* Send to DAG root */
      LOG_INFO("Sending request %" PRIu32 " from ", tx_count);
      uip_ds6_addr_t *addr = uip_ds6_get_global(ADDR_PREFERRED);
      if (addr != NULL)
      {
        LOG_INFO_6ADDR(&addr->ipaddr);
      }
      LOG_INFO_("\n");
      // LOG_INFO_("\n");
      // random 50/50
      // if (random_rand() % 2 == 0)
      // {
      snprintf(str, sizeof(str), "%" PRIu32, tx_count);
      // }
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      tx_count++;
      tx_bytes += strlen(str);
    }
    else
    {
      // LOG_INFO("Not reachable yet\n");
      if (tx_count > 0)
      {
        missed_tx_count++;
      }
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, (random_rand() % (1000)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
