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
    struct in_addr my_in_addr;
    unsigned char hwaddr[ETHER_ADDR_LEN];
    void *buf = NULL; //1 byte at a time uint8_t
    unsigned int len = 0; //Later use sizeof to finde how many bytes
    const char* iface = "eth1"; //eth1 doesnt exist // prob with eth header
    char* sender = ""; //"" = myself

    struct sr_arphdr arphdr;

    


    /* REQUIRES */
    assert(sr);

    /* Add initialization code here! TODO*/
    sr_add_interface(sr, iface);
    sr_set_ether_addr(sr ,);
    sr_set_ether_ip(sr, uint32_t);


    //ioctl_sock = socket(AF_INET, SOCK_PACKET, htons(ETH_P_RARP));
    //ioctl_sock = socket(SOL_SOCKET, SOCK_RAW, ETHERTYPE_REVARP);
    ioctl_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(ioctl_sock < 0){
        fprintf(stderr, "sr_init: Unable to create socket\n");
    }

    get_hw_addr(hwaddr, sender);
    get_ip_addr(&my_in_addr, sender);

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
    
    for(int i = 0; i < ETHER_ADDR_LEN; i++){
        arphdr.ar_sha[i] = hwaddr[i];
    }

    
    for(int i = 0; i < ETHER_ADDR_LEN; i++){
        arphdr.ar_tha[i] = 0xff; //Broadcast MAC addresses ff:ff:ff:ff:ff:ff
    }
    
    memcpy(&arphdr.ar_sip, &my_in_addr, IP_ADDR_LEN);
    memcpy(&arphdr.ar_tip, &my_in_addr, IP_ADDR_LEN);
    //Debug("sr_int: memcpyed IP address: %s\n", inet_ntoa((struct in_addr)arphdr.ar_sip));
     

    //Find out which interface to send them
    iface = "eth1";
    //Set buffer
    buf = &arphdr;
    len = sizeof(arphdr);
    Debug("sr_init: len = %u\n", len);
    
    sr_print_if_list(sr);

    //Send packet
    if(sr_send_packet(sr, buf, len, iface)){
        fprintf(sdterr, "Failed to send initial ARP request\n");
    } //sr_vns_comm.c
    Debug("Successfully completed sr_init\n");

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
    
    if(strcmp(str, "") == 0){
        Debug("Using own IP address\n");
        struct ifconf ifc;
        char buffer[1024];
        
        
        ifc.ifc_len = sizeof(buffer);
        ifc.ifc_buf = buffer;
        if(ioctl(ioctl_sock, SIOCGIFCONF, &ifc) == -1){
            fprintf(stderr, "get_ip_addr: socket error\n");
        }
        
        struct ifreq* it = ifc.ifc_req;
        const struct ifreq* const end = it + (ifc.ifc_len /sizeof(struct ifreq));
        
        for(; it != end; ++it){
            strcpy(ifr.ifr_name, it->ifr_name);
            ifr.ifr_addr.sa_family = it->ifr_addr.sa_family;
            //ifr.ifr_addr.sa_family = AF_INET;
            if(ioctl(ioctl_sock, SIOCGIFFLAGS, &ifr) == 0){
                if(!(ifr.ifr_flags & IFF_LOOPBACK)){//don't count loopback
                    if(ioctl(ioctl_sock, SIOCGIFADDR, &ifr) == 0){
                        //Success
                        break;
                    }else{
                        die("Failed to get IP address for the interface");
                    }
                }
            }else{
                fprintf(stderr, "get_ip_addr: Error inside loop with flags\n");
            }
        }
    }else{
        strcpy(ifr.ifr_name, str);
        ifr.ifr_addr.sa_family = AF_INET;
        if (ioctl(ioctl_sock, SIOCGIFADDR, &ifr))
            die("Failed to get IP address for the interface");
    }


    memcpy(&sin, &ifr.ifr_addr, sizeof(struct sockaddr_in));
    in_addr->s_addr = sin.sin_addr.s_addr;
    Debug("IP address: %s\n", inet_ntoa(*in_addr));
}



void get_hw_addr(u_char* buf,char* str)
{
  
    struct ifreq ifr;
    
    if(strcmp(str, "") == 0){
        Debug("Using own hardware address\n");
        struct ifconf ifc;
        char buffer[1024];
        
        
        ifc.ifc_len = sizeof(buffer);
        ifc.ifc_buf = buffer;
        if(ioctl(ioctl_sock, SIOCGIFCONF, &ifc) == -1){
            fprintf(stderr, "get_hw_addr: socket error\n");
        }
        
        struct ifreq* it = ifc.ifc_req;
        const struct ifreq* const end = it + (ifc.ifc_len /sizeof(struct ifreq));
        
        for(; it != end; ++it){
            strcpy(ifr.ifr_name, it->ifr_name);
            if(ioctl(ioctl_sock, SIOCGIFFLAGS, &ifr) == 0){
                if(!(ifr.ifr_flags & IFF_LOOPBACK)){//don't count loopback
                    if(ioctl(ioctl_sock, SIOCGIFHWADDR, &ifr) == 0){
                        //Success
                        break;
                    }else{
                        die("Failed to get own MAC address for the interface");
                    }
                }
            }else{
                fprintf(stderr, "get_hw_addr: Error inside loop with flags\n");
            }
        }
    }else{
        strcpy(ifr.ifr_name, str);
        if (ioctl(ioctl_sock, SIOCGIFHWADDR, &ifr) < 0)
            die("Failed to get MAC address for the interface");
    }
    

    memcpy(buf, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
    Debug("MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n", *(buf), *(buf+1), *(buf+2), *(buf+3),*(buf+4), *(buf+5));
}

void die(const char* str)
{
    fprintf(stderr,"Error: %s\n",str);
    exit(1);
}
