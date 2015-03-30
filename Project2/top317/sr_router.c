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

#define BROADCAST 0xFFFFFFFFFFFF

//int ioctl_sock;
int nameCount;

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{

    /* REQUIRES */
    assert(sr);

    /* Add initialization code here! TODO*/
    
    nameCount = 0;
    

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
    //Read eth hdr
    struct sr_arppkt* etherHdr = NULL;
    etherHdr = (struct sr_arppkt*)packet;
    struct sr_if* if_walker = 0;
    unsigned short  type = ntohs(etherHdr->ether_type);
    
    //Save source and dest
    Debug("Ethernet type is 0x%x", type);
    if(type == ETHERTYPE_IP){
        Debug(" (IPv4)\n");
    }else if(type == ETHERTYPE_ARP){
        Debug(" (ARP ");
        if(ntohs(etherHdr->ar_op) == ARP_REQUEST){
            Debug("Request)\n");
        }else if(ntohs(etherHdr->ar_op) == ARP_REPLY){
            Debug("Reply)\n");
        }else{
            Debug("?)\n");
        }
    }
    Debug("source MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
          etherHdr->ether_shost[0],
          etherHdr->ether_shost[1], etherHdr->ether_shost[2],
          etherHdr->ether_shost[3],etherHdr->ether_shost[4],
          etherHdr->ether_shost[5]);
    Debug("destination MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
          etherHdr->ether_dhost[0],
          etherHdr->ether_dhost[1], etherHdr->ether_dhost[2],
          etherHdr->ether_dhost[3],etherHdr->ether_dhost[4],
          etherHdr->ether_dhost[5]);

    /*-----------------------------------------------------------------------------------------------------*/
    //Check if need to add to routing table
    if(type == ETHERTYPE_IP){
        updateRoutingTable(sr, packet + sizeof(struct sr_ethernet_hdr), ETHERTYPE_IP, interface);
    }else if(type == ETHERTYPE_ARP){
        updateRoutingTable(sr, packet + sizeof(struct sr_ethernet_hdr), ETHERTYPE_ARP, interface);
    }
    /*-----------------------------------------------------------------------------------------------------*/
    if(type == ETHERTYPE_IP){
        struct ip* ipHdr = NULL;
        struct sr_rt* rt_walker;

        //MAC first
        for(if_walker = sr->if_list; if_walker; if_walker = if_walker->next){
            if(memcmp(etherHdr->ether_dhost, if_walker->addr, ETHER_ADDR_LEN) == 0){
                Debug("I already know this MAC address\n");
                break;
            }
        }
        if(if_walker == 0){
            Debug("MAC destination not in interface, need to send ARP request TODO!!\n");
            
        }
        //forward packet to all other interfaces
       
       //Flood to all other nodes
       //Continue until end of list
       //  Debug("Forwarding packet to all other interfaces\n");
       // for(if_walker = sr->if_list;if_walker;if_walker = if_walker->next){
       //     Debug("Searching through interfaces...\n");
       //     if(strcmp(if_walker->name, interface) != 0){
       //         //Change source to my own
       //         memcpy( etherHdr->ether_shost , if_walker->addr, ETHER_ADDR_LEN);
       //         Debug("Forwarding to interface %s\n", if_walker->name);
       //         if(sr_send_packet(sr, packet, len, if_walker->name)){
       //             fprintf(stderr, "Failed to forward IP packet\n");
       //         } //sr_vns_comm.c 
       //     }
       // }
        
        ipHdr = (struct ip*)(packet + sizeof(struct sr_ethernet_hdr));
        
        Debug("Source IP address: %s\n", inet_ntoa(*((struct in_addr*)(&ipHdr->ip_src))));
        Debug("Destination IP address: %s\n", inet_ntoa(*((struct in_addr*)(&ipHdr->ip_dst))));

        
        //Find out if dest is on same subnet
        //AND own IP and dest IP with subnet mask. If same then same subnet. Use if
        uint32_t destIP;
        uint32_t myIP;
        memcpy(&destIP, &ipHdr->ip_dst, IP_ADDR_LEN);
        
        for(if_walker = sr->if_list; if_walker; if_walker = if_walker->next){
            if(strcmp(interface, if_walker->name) == 0){
                break;
            }
        }
        if(if_walker == 0){
            fprintf(stderr, "Don't know my own IP Address for interface: %s!\n", interface);
            return;
        }
        memcpy(&myIP, &if_walker->ip, IP_ADDR_LEN);
        //Get mask
        for(rt_walker = sr->routing_table; rt_walker; rt_walker = rt_walker->next){
            if(strcmp(interface, rt_walker->interface) == 0){
                break;
            }
        }
        if(rt_walker == 0){
            fprintf(stderr, "Couldn't fine subnet mask for this interface: %s!\n", interface);
            return;
        }
        uint32_t mask = 0x00ffffff;//255.255.255.0
        //memcpy(&mask, &rt_walker->mask, IP_ADDR_LEN);
        Debug("Before mine AND dest IPs: %s", inet_ntoa(*((struct in_addr*)(&myIP))));
        Debug(" and %s\n", inet_ntoa(*((struct in_addr*)(&destIP))));

        Debug("With mask: %s\n", inet_ntoa(*((struct in_addr*)(&mask))));
        myIP = myIP & mask;
        destIP = destIP & mask;
        Debug("After AND: %s ", inet_ntoa(*((struct in_addr*)(&myIP))));
        Debug("and %s\n",  inet_ntoa(*((struct in_addr*)(&destIP))));

        /*-----------------------------------------------------------------------------------------------------*/
        //If dest ip in same subnet and dont know, then send arp request
        //If dest ip not on same LAN, then check routing table and send it
        //If no match then send to default gateway

        //Check mask
        if(memcmp(&myIP, &destIP, IP_ADDR_LEN) == 0){
            //What happens when with local LAN
            Debug("This needs to go to the local subnet\n");
            
            //do{
                for(if_walker = sr->if_list; if_walker; if_walker = if_walker->next){
                    if(memcmp(&ipHdr->ip_dst, &if_walker->ip, IP_ADDR_LEN) == 0){
                        Debug("IP addr (and MAC) found in interface\n");
                        break;
                    }
                }
                if(if_walker == 0){
                    Debug("Need to send ARP request to find HWAddr\n");
                    requestARP(sr, ipHdr->ip_dst);
                }
            //}while(if_walker != 0);
            Debug("About to send packet to LAN\n");
            //Edit packet
            
            for(rt_walker = sr->routing_table; rt_walker; rt_walker = rt_walker->next){
                Debug("Loop\n");
                if(memcmp(&rt_walker->dest, &if_walker->ip, IP_ADDR_LEN) == 0){
                    Debug("Break\n");
                    break;
                }
            }
            if(rt_walker == 0){
                fprintf(stderr, "Something wrong with the routing table\n");
            }
            Debug("1\n");
            //struct sr_if* baseIF = sr_get_interface(sr, rt_walker->interface);
            //memcpy(etherHdr->ether_dhost, if_walker->addr, ETHER_ADDR_LEN);
            //memcpy(etherHdr->ether_shost, baseIF->addr, ETHER_ADDR_LEN);
            // Debug("New dest HWaddr: ");
            // for(int i = 0; i < ETHER_ADDR_LEN; i++){
            //     Debug("%02x:", etherHdr->ether_dhost[i]);
            // }
            // Debug("\n");
            // Debug("New source HWaddr: ");
            // for(int i = 0; i < ETHER_ADDR_LEN; i++){
            //     Debug("%02x:", etherHdr->ether_shost[i]);
            // }
            // Debug("\n");
            //Send it
            if(sr_send_packet(sr, packet, len, rt_walker->interface)){
                fprintf(stderr, "Failed to forward IP packet\n");
            } //sr_vns_comm.c
            
            Debug("2\n");
        }else{
            //Dest is not in LAN, so no ARP
            Debug("This goes to different network\n");
            //Forward to default
            uint32_t zip = 0;
            for(rt_walker = sr->routing_table; rt_walker; rt_walker = rt_walker->next){
                if(memcmp(&rt_walker->dest, &zip, IP_ADDR_LEN) == 0){
                    Debug("Found the default router at %s\n", rt_walker->interface);
                    break;
                }
            }
            if(rt_walker == 0){
                fprintf(stderr, "Something went wrong finding default router\n");
            }else{
                //Continue until end of list
               for(if_walker = sr->if_list;if_walker;if_walker = if_walker->next){
                   Debug("Searching through interfaces...\n");
                   if(strcmp(if_walker->name, rt_walker->interface) == 0){
                        Debug("Found default router\n");
                        //CHange packet!

                        // for(rt_walker = sr->routing_table; rt_walker; rt_walker = rt_walker->next){
                        //     if(memcmp(&rt_walker->dest, &if_walker->ip, IP_ADDR_LEN) == 0){
                        //         break;
                        //     }
                        // }
                        // if(rt_walker == 0){
                        //     fprintf(stderr, "Something wrong with the routing table out\n");
                        // }
                        // struct sr_if* baseIF = sr_get_interface(sr, rt_walker->interface);
                        // memcpy(etherHdr->ether_dhost, if_walker->addr, ETHER_ADDR_LEN);
                        // memcpy(etherHdr->ether_shost, baseIF->addr, ETHER_ADDR_LEN);
                       //Change source to my own
                        memcpy( etherHdr->ether_shost , if_walker->addr, ETHER_ADDR_LEN);
                       //memcpy( etherHdr->ether_dhost, if_walker->)
                       Debug("Forwarding to interface, which should be defualt:%s\n", if_walker->name);
                       if(sr_send_packet(sr, packet, len, if_walker->name)){
                           fprintf(stderr, "Failed to forward IP packet\n");
                       } //sr_vns_comm.c
                       break;
                   }
                   
                }
                if(if_walker == 0){
                    Debug("Couldn't find default router\n");
                }
            }
        }
        
        
        
        
    /*-----------------------------------------------------------------------------------------------------*/
    }else if(type == ETHERTYPE_ARP){
        
        
        struct sr_arphdr* arpHdr = (struct sr_arphdr*)(packet + sizeof(struct sr_ethernet_hdr));
        
        Debug("Source of ARP's HWaddr: ");
        for(int i = 0; i < ETHER_ADDR_LEN; i++){
            Debug("%02x:", arpHdr->ar_sha[i]);
        }
        Debug("\n");
        Debug("Source IP address: %s\n", inet_ntoa(*((struct in_addr*)(&arpHdr->ar_sip))));
        
        
        Debug("Target of ARP's HWaddr: ");
        for(int i = 0; i < ETHER_ADDR_LEN; i++){
            Debug("%02x:", arpHdr->ar_tha[i]);
        }
        Debug("\n");
        Debug("Target IP address: %s\n", inet_ntoa(*((struct in_addr*)(&arpHdr->ar_tip))));
        
        //Compare target IP to myself
        
        for(if_walker = sr->if_list; if_walker; if_walker = if_walker->next){
            if(strcmp(if_walker->name, interface) == 0){
                Debug("My IP is %s\n", inet_ntoa(*((struct in_addr*)(&if_walker->ip))));
                if(memcmp(&if_walker->ip, &arpHdr->ar_tip, IP_ADDR_LEN) == 0){
                    Debug("This ARP request is for me!\n");
                    if(ntohs(etherHdr->ar_op) == ARP_REQUEST){
                        //Send ARP reply
                        replyARP(sr, packet, len, if_walker);
                    }else if(ntohs(etherHdr->ar_op) == ARP_REPLY){
                        //Add ARP reply to routingTable/Interface
                        updateARPCache(sr, packet);
                    }
                    
                    
                    break;
                }

            }
        }
        
    }else{
        Debug("Unknown packet type. Dropping...\n");
    }
    //Still need to update Routing table with each packet
    sr_print_routing_table(sr);
    sr_print_if_list(sr);
    Debug("Finished handling packet\n\n");
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

void updateRoutingTable(struct sr_instance* sr, uint8_t* packet, unsigned int ethertype, char* interface){
    uint32_t gateway = 0x00000000;
    uint32_t mask = 0xf8ffffff;//255.255.255.248
    struct in_addr senderIP;
    struct in_addr gw;
    struct in_addr msk;
    struct sr_rt* rt_walker;

    //Debug("This default gateway is: %s\n", inet_ntoa(*((struct in_addr*)(&gateway))));
    //Debug("This default mask is: %s\n", inet_ntoa(*((struct in_addr*)(&mask))));
    if(ethertype == ETHERTYPE_IP){
        struct ip* ip_pckt = (struct ip*)packet;
        memcpy(&senderIP, &ip_pckt->ip_src, IP_ADDR_LEN);
        memcpy(&gw, &gateway, IP_ADDR_LEN);
        memcpy(&msk, &mask, IP_ADDR_LEN);

        for(rt_walker = sr->routing_table; rt_walker; rt_walker = rt_walker->next){
            Debug("Comparing IP address: %s", inet_ntoa(*((struct in_addr*)(&senderIP))));
            Debug(" to RT IP address: %s\n", inet_ntoa(*((struct in_addr*)(&rt_walker->dest))));
            if(memcmp(&rt_walker->dest, &senderIP, IP_ADDR_LEN) == 0){
                break;
            }
        }
        if(rt_walker == 0){
            sr_add_rt_entry(sr, senderIP, gw, msk, interface);
            Debug("Updated routing table\n");
        }
        
        

    }else if(ethertype == ETHERTYPE_ARP){
        struct sr_arphdr* arp_pckt = (struct sr_arphdr*)packet;
        memcpy(&senderIP, &arp_pckt->ar_sip, IP_ADDR_LEN);
        memcpy(&gw, &gateway, IP_ADDR_LEN);
        memcpy(&msk, &mask, IP_ADDR_LEN);
        
        for(rt_walker = sr->routing_table; rt_walker; rt_walker = rt_walker->next){
            if(memcmp(&rt_walker->dest, &senderIP, IP_ADDR_LEN) == 0){
                break;
            }
        }
        if(rt_walker == 0){
            sr_add_rt_entry(sr, senderIP, gw, msk, interface);
            Debug("Updated routing table\n");
        }
    
    }
    
}

//Receives and ARP Reply packet to this router
//Update Interface
void updateARPCache(struct sr_instance* sr, uint8_t * packet)
{
    struct sr_arppkt* arppkt = (struct sr_arppkt*)packet;
    char newName[sr_IFACE_NAMELEN] = "arp";
    char num[12];
    Debug("Updating ARP Cache\n");
    sprintf(num, "%d", nameCount);
    strcat(newName, num);
    Debug("New interface called %s\n", newName);
    nameCount++;
    
    sr_add_interface(sr, newName);
    sr_set_ether_addr(sr, arppkt->ar_sha);//Which MAC address?
    sr_set_ether_ip(sr, arppkt->ar_sip);
    Debug("New interface list\n");
    sr_print_if_list(sr);
}

int decrementAndCheck(struct ip* ipHdr){
    //Check checksum
    uint16_t checksum;
    memcpy(&checksum, &ipHdr->ip_sum, 2);
    Debug("Checksum originally = %u\n", checksum);
    checksum = ipHdr->ip_sum;
    Debug("Checksum originally = %u\n", checksum);
    checksum = ip_checksum(ipHdr, ipHdr->ip_hl);
    Debug("Checksum = %u\n", checksum);
    ipHdr->ip_sum = 0;
    //memcpy(ipHdr->ip_sum, zero, 2);
    memset(&ipHdr->ip_sum, 0, 2);
    Debug("Checksum zeroed  %u\n", ipHdr->ip_sum);
    uint16_t hLen = ipHdr->ip_hl;//NUmber of 32 bit words
    Debug("Header length is %u\n", hLen);
    
    checksum = ip_checksum(ipHdr, hLen);
    Debug("Checksum = %u\n", checksum);
    checksum = ip_checksum(ipHdr, hLen);
    Debug("Checksum = %u\n", checksum);
    checksum = chksum(ipHdr, hLen);
    Debug("Checksum new = %u\n", checksum);
    
    //Decrement TTL
    Debug("Decremented TTL from %u", ipHdr->ip_ttl);
    ipHdr->ip_ttl--;
    Debug(" to %u\n", ipHdr->ip_ttl);
    //Drop packet if ttl == 0
    if(ipHdr->ip_ttl == 0){
        return -1;
    }
    //Update checksum
    return 0;
}

//Reply knowing that you are the target
void replyARP(struct sr_instance* sr,
              uint8_t * packet/* lent */, //Packet buffer
              unsigned int len, //Packet length
              struct sr_if* iface/* lent */) //Recieving interface
{
    struct sr_arppkt* arppkt = (struct sr_arppkt*)packet;
    
    //Change ether hdr
    memcpy(arppkt->ether_dhost, arppkt->ether_shost, ETHER_ADDR_LEN);
    memcpy(arppkt->ether_shost, iface->addr, ETHER_ADDR_LEN);
    //->ether_type is same  htons(ETHERTYPE_ARP)
    
    //Change arp hdr
    //1st 4 have same format and length
    arppkt->ar_op = htons(ARP_REPLY);
    //Target first
    memcpy(arppkt->ar_tha, arppkt->ar_sha, ETHER_ADDR_LEN);
    memcpy(&arppkt->ar_tip, &arppkt->ar_sip, ETHER_ADDR_LEN);
    //Then sender
    memcpy(arppkt->ar_sha, iface->addr, ETHER_ADDR_LEN);
    memcpy(&arppkt->ar_sip, &iface->ip, IP_ADDR_LEN);
    
    
    Debug("Source of ARP's HWaddr: ");
    for(int i = 0; i < ETHER_ADDR_LEN; i++){
        Debug("%02x:", arppkt->ar_sha[i]);
    }
    Debug("\n");
    Debug("Source IP address: %s\n", inet_ntoa(*((struct in_addr*)(&arppkt->ar_sip))));
    
    
    Debug("Target of ARP's HWaddr: ");
    for(int i = 0; i < ETHER_ADDR_LEN; i++){
        Debug("%02x:", arppkt->ar_tha[i]);
    }
    Debug("\n");
    Debug("Target IP address: %s\n", inet_ntoa(*((struct in_addr*)(&arppkt->ar_tip))));
    
    
    if(sr_send_packet(sr, packet, len, iface->name)){
        fprintf(stderr, "Failed to send ARP reply\n");
    } //sr_vns_comm.c
    Debug("Successfully sent ARP Reply\n");
}



//Sends arp request of given target IP to all non-gateways
void requestARP(struct sr_instance* sr, struct in_addr target_in_addr){
    
//    struct in_addr my_in_addr;
//    unsigned char hwaddr[ETHER_ADDR_LEN];
    void *buf = NULL; //1 byte at a time uint8_t
    unsigned int len = 0; //Later use sizeof to finde how many bytes
    struct sr_rt* rt_walker = NULL;
    struct sr_if* if_walker = NULL;
   
    struct sr_arppkt arppkt;
    
    
    /* REQUIRES */
    assert(sr);
    
    Debug("Creating ARP request\n");
    
    
    //Send out ARP request to all servers on LAN to build table
    //Create ARP request packet
    //Fill out info. In sr_protocol.h
    //Types
    arppkt.ether_type = htons(ETHERTYPE_ARP);
    arppkt.ar_hrd = htons(ARPHDR_ETHER);
    arppkt.ar_pro = htons(ETHERTYPE_IP);
    //Address lengths
    arppkt.ar_hln = ETHER_ADDR_LEN; //6
    arppkt.ar_pln = IP_ADDR_LEN; //4
    //Type
    arppkt.ar_op = htons(ARP_REQUEST);
    
    //Sender/Target IP/Hardware addresses
    
    Debug("Target MAC: ");
    for(int i = 0; i < ETHER_ADDR_LEN; i++){
        arppkt.ar_tha[i] = 0x00;
        arppkt.ether_dhost[i] = 0xff;//Broadcast MAC addresses
        Debug("%02x:", arppkt.ar_tha[i]);
    }
    Debug("\n");
    
    Debug("Destination MAC(Ethernet): ");
    for(int i = 0; i < ETHER_ADDR_LEN; i++){
        Debug("%02x:", arppkt.ether_dhost[i]);
    }
    Debug("\n");
    
    
    memcpy(&arppkt.ar_tip, &target_in_addr, IP_ADDR_LEN);
    Debug("requestARP: target IP address: %s\n", inet_ntoa(*((struct in_addr*)(&arppkt.ar_tip))));
    
    
    
    //Find out which interface to send them
    
    //Set buffer
    buf = &arppkt;
    len = sizeof(arppkt);
    Debug("requestARP: len = %u\n", len);
    
    
    //Send packet to all non-gateways
    //source ip and MAC addr changes each time
    uint32_t zero = 0;
    for(rt_walker = sr->routing_table; rt_walker; rt_walker = rt_walker->next){
        //Check all entries in routing table
        if(memcmp(&rt_walker->gw, &zero, IP_ADDR_LEN) == 0){
            Debug("Need to send ARP request through interface %s\n", rt_walker->interface);
            //Get HWAddr for given interface
            for(if_walker = sr->if_list; if_walker; if_walker = if_walker->next){
                if(strcmp(rt_walker->interface, if_walker->name) == 0){
                    //Set source/sender MAC
                    memcpy(arppkt.ar_sha, if_walker->addr, ETHER_ADDR_LEN);
                    memcpy(arppkt.ether_shost, if_walker->addr, ETHER_ADDR_LEN);
                    Debug("Source dest MAC: ");
                    for(int i = 0; i < ETHER_ADDR_LEN; i++){
                        Debug("%02x:", arppkt.ar_sha[i]);
                    }
                    Debug("\n");
                    //Set sender IP
                    memcpy(&arppkt.ar_sip, &if_walker->ip, IP_ADDR_LEN);
                    Debug("requestARP: sender IP address: %s\n", inet_ntoa(*((struct in_addr*)(&arppkt.ar_sip))));
                    
                    if(sr_send_packet(sr, buf, len, rt_walker->interface)){
                        fprintf(stderr, "Failed to send initial ARP request\n");
                    } //sr_vns_comm.c

                    break;
                }
            }
            if(if_walker == 0){
                fprintf(stderr, "requestARP: Error, routing table and interface not in sync\n"
                        "Not send ARP Request\n");
                return;
            }
            
            
        }
    }
    Debug("Successfully Sent ARP Request\n");
    
}




void die(const char* str)
{
    fprintf(stderr,"Error: %s\n",str);
    exit(1);
}

//Calculates checksum of ipv4 header
//Make sure checksum field is already zeroed out
//Length is header length
uint16_t ip_checksum(void* vdata,size_t length) {
    // Cast the data pointer to one that can be indexed.
    char* data=(char*)vdata;
    
    // Initialise the accumulator.
    uint64_t acc=0xffff;
    
    // Handle any partial block at the start of the data.
    unsigned int offset=((uintptr_t)data)&3;
    if (offset) {
        size_t count=4-offset;
        if (count>length) count=length;
        uint32_t word=0;
        memcpy(offset+(char*)&word,data,count);
        acc+=ntohl(word);
        data+=count;
        length-=count;
    }
    
    // Handle any complete 32-bit blocks.
    char* data_end=data+(length&~3);
    while (data!=data_end) {
        uint32_t word;
        memcpy(&word,data,4);
        acc+=ntohl(word);
        data+=4;
    }
    length&=3;
    
    // Handle any partial block at the end of the data.
    if (length) {
        uint32_t word=0;
        memcpy(&word,data,length);
        acc+=ntohl(word);
    }
    
    // Handle deferred carries.
    acc=(acc&0xffffffff)+(acc>>32);
    while (acc>>16) {
        acc=(acc&0xffff)+(acc>>16);
    }
    
    // If the data began at an odd byte address
    // then reverse the byte order to compensate.
    if (offset&1) {
        acc=((acc&0xff00)>>8)|((acc&0x00ff)<<8);
    }
    
    // Return the checksum in network byte order.
    return htons(~acc);
}

uint16_t chksum(void *vdata, size_t length){
    
    // Cast the data pointer to one that can be indexed.
    char* data=(char*)vdata;
    
    // Initialise the accumulator.
    uint32_t acc=0xffff;
    
    // Handle complete 16-bit blocks.
    for (size_t i=0;i+1<length;i+=2) {
        uint16_t word;
        memcpy(&word,data+i,2);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }
    
    // Handle any partial block at the end of the data.
    if (length&1) {
        uint16_t word=0;
        memcpy(&word,data+length-1,1);
        acc+=ntohs(word);
        if (acc>0xffff) {
            acc-=0xffff;
        }
    }
    
    // Return the checksum in network byte order.
    return htons(~acc);
}
