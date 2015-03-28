/**********************************************************************
 * file:  sr_router.c
 * date:  Mon Feb 18 12:50:42 PST 2002
 * Contact: casado@stanford.edu
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing. 11
 * 90904102
 **********************************************************************/

#include <stdio.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <sys/ioctl.h>

//#define  NEWSOCKET() socket(AF_INET, SOCK_PACKET, htons(ETH_P_RARP))


#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"

int ioctl_sock;

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    uint8_t *buf = NULL; //1 byte at a time
    unsigned int len = 0; //Later use sizeof to finde how many bytes
    const char* iface = "";

    struct sr_arphdr arphdr;

    ioctl_sock = socket(AF_INET, SOCK_PACKET, htons(ETH_P_RARP));


    /* REQUIRES */
    assert(sr);

    /* Add initialization code here! TODO*/
    //Send out ARP request to all servers on LAN to build table
    //Create ARP request packet
    //Fill out info. In sr_protocol.h
    //Types
    arphdr.ar_hrd = htons(ARPHDR_ETHER);
    arphdr.ar_pro = htons(ETHERTYPE_IP);
    //Address lengths
    arphdr.ar_hln = ETHER_ADDR_LEN; //6
    arphdr.ar_pln = IP_ADDR_LEN; //4
    //Type
    arphdr.ar_op = htons(ARP_REQUEST);
    //Sender/Target IP/Hardware addresses
    // arphdr.ar_sha

    // arphdr.ar_sip

    // arphdr.ar_tha

    // arphdr.ar_tip

    //Find out which interface to send them
    iface = "eth1";
    
    //Send packets
    sr_send_packet(sr, buf, len, iface); //sr_vns_comm.c

} /* -- sr_init -- */



/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */, //Packet buffer
        unsigned int len, //Packet length
        char* interface/* lent */) //Recieving interface
{
    /* REQUIRES */
    assert(sr);
    assert(packet);
    assert(interface);

    printf("*** -> Received packet of length %d on interface %s\n",len, interface);

    //Not complete?TODO

}/* end sr_ForwardPacket */


/*---------------------------------------------------------------------
 * Method:
 *
 *---------------------------------------------------------------------*/

/*
you don't need to be concerned about the interaction between VNL service and soft-host,
 or how packets flow in the physical topology. You can just focus
on the virtual topology and your own router. Your job is to make the router fully
 functional by implementing packet processing and forwarding within the skeleton
code. More specifically, you'll need to implement ARP and basic IP forwarding.
*/

void get_ip_addr(struct in_addr* in_addr,char* str)
{
    struct ifreq ifr;
    struct sockaddr_in sin;

    strcpy(ifr.ifr_name, str);
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(ioctl_sock, SIOCGIFADDR, &ifr))
        die("Failed to get IP address for the interface");


    memcpy(&sin, &ifr.ifr_addr, sizeof(struct sockaddr_in));
    in_addr->s_addr = sin.sin_addr.s_addr;
    Debug("IP address: %s\n", inet_ntoa(*in_addr));
}

void get_hw_addr(u_char* buf,char* str)
{
    int i;
    struct ifreq ifr;
    char c,val = 0;

    strcpy(ifr.ifr_name, str);
    if (ioctl(ioctl_sock, SIOCGIFHWADDR, &ifr))
        die("Failed to get MAC address for the interface");

    memcpy(buf, ifr.ifr_hwaddr.sa_data, 8);
    Debug("MAC address: %0.2X:%0.2X:%0.2X:%0.2X:%0.2X:%0.2X\n", *(buf), *(buf+1), *(buf+2), *(buf+3),*(buf+4), *(buf+5));
}

void die(const char* str)
{
    fprintf(stderr,"Error: %s\n",str);
    exit(1);
}
