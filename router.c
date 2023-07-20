#include "queue.h"
#include "lib.h"
#include "auxiliar.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#define MAX_ADDR_NR 100000
struct route_table_entry *bin_rtr_tbl(uint32_t daddr, router_table *rt_tbl, int left, int right)
{
	if (right >= left)
	{
		int mid = left + (right - left) / 2;
		if ((daddr & rt_tbl->vector[mid].mask) == rt_tbl->vector[mid].prefix)
		{
			return &rt_tbl->vector[mid];
		}
		if ((daddr & rt_tbl->vector[mid].mask) > rt_tbl->vector[mid].prefix)
		{
			return bin_rtr_tbl(daddr, rt_tbl, mid + 1, right);
		}
		if ((daddr & rt_tbl->vector[mid].mask) < rt_tbl->vector[mid].prefix)
		{
			return bin_rtr_tbl(daddr, rt_tbl, left, mid - 1);
		}
	}
	return NULL;
}

void modify_package_for_router(struct ether_header **eth_hdr,
							   struct arp_header **arp_hdr, int interface, uint32_t router_ip)
{
	// modified eth_hdr header;
	uint8_t interface_mac[6];
	get_interface_mac(interface, interface_mac);
	memcpy((*eth_hdr)->ether_dhost, (*eth_hdr)->ether_shost, 6);
	memcpy((*eth_hdr)->ether_shost, interface_mac, 6);
	(*eth_hdr)->ether_type = ntohs(0x0806);

	// modified arp_hdr
	(*arp_hdr)->tpa = (*arp_hdr)->spa;
	memcpy((*arp_hdr)->tha, (*arp_hdr)->sha, 6);
	memcpy((*arp_hdr)->sha, interface_mac, 6);
	(*arp_hdr)->spa = router_ip;
	(*arp_hdr)->op = ntohs(2);
}

uint32_t get_as_uint32_ip(int interface)
{
	struct in_addr ip_of_interface;
	inet_aton(get_interface_ip(interface), &ip_of_interface);
	return ip_of_interface.s_addr;
}



int check_arp_table(arp_table *arp_tbl, struct route_table_entry *rt_tbl_entry, struct ether_header **eth_hdr)
{
	int gasit_in_memoria_cache = 0;
	for (int i = 0; i < arp_tbl->size; i++)
	{
		if (rt_tbl_entry->next_hop == arp_tbl->vector[i].ip)
		{
			gasit_in_memoria_cache = 1;
			memcpy((*eth_hdr)->ether_dhost, arp_tbl->vector[i].mac, 6);
		}
	}
	return gasit_in_memoria_cache;
}
int main(int argc, char *argv[])
{
	char buf[MAX_PACKET_LEN];

	// Do not modify this line
	init(argc - 2, argv + 2);

	arp_table *arp_tbl = init_arp_table_without_file();
	router_table *rt_tbl = init_router_table(argv[1]);
	queue packets_queue = queue_create();
	quickSort(rt_tbl, 0, rt_tbl->size - 1);

	while (1)
	{

		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

		struct ether_header *eth_hdr = (struct ether_header *)buf;
		if (ntohs(eth_hdr->ether_type) == 0x0800)
		{
			printf("ipv4 waii\n");
			struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header));

			if (get_as_uint32_ip(interface) == ip_hdr->daddr)
			{
				// raspunde la mesaje de tip ICMP;
				char *buffer = create_icmp_header(buf, interface, 0);
				send_to_link(interface, buffer, len + sizeof(struct icmphdr)); 
				printf("ba ce drq\n");
				continue;
			}

			// check_sum nu prea e clara e treaba, ideea e ca il verifica, daca e corupt pachetul ii spune papa
			if (verify_checksum(buf, ip_hdr) == 0)
			{
				printf("aruncam pachetul\n");
				continue;
				// aruncam pachetul da-l incolo
			}

			if (ip_hdr->ttl == 0 || ip_hdr->ttl == 1)
			{
				// creaza pachetul ICPM cu time-exceeded
				char *buffer = create_icmp_header(buf, interface, 11);
				send_to_link(interface, buffer, len + sizeof(struct icmphdr)); 
				continue;
			}

			ip_hdr->ttl = ip_hdr->ttl - 1;

			struct route_table_entry *rt_tbl_entry;
			int index = liniar_search(rt_tbl, ip_hdr->daddr);
			if (index != -1) {
				rt_tbl_entry = &rt_tbl->vector[index];
				print_ip_addr(rt_tbl_entry->next_hop);
				print_ip_addr(rt_tbl_entry->prefix);
				print_ip_addr(rt_tbl_entry->interface);
			} else {
				// creaza pachetul ICPM cu destination unreacheable
				char *buffer = create_icmp_header(buf, interface, 3);
				send_to_link(interface, buffer, len + sizeof(struct icmphdr)); 
				continue;
			}

			// cauta adresa urmatorului hop, daca nu o gaseste da drq pachetul respectiv

			ip_hdr->check = htons(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));
			// pana in punctul asta pachetul e facut
			struct package *pack = create_package(rt_tbl_entry->interface, len, buf);
			queue_enq(packets_queue, pack);

			//cautam in tabela
			int right_index = -1;
			for(int i = 0 ; i < arp_tbl->size; i++) {
				if (ip_hdr->daddr == arp_tbl->vector[i].ip) {
					right_index = i;
					break;
				}
			}

			if (right_index != -1) {
				memcpy(eth_hdr->ether_dhost, arp_tbl->vector[right_index].mac, 6);
				get_interface_mac(rt_tbl_entry->interface, eth_hdr->ether_shost);
				send_to_link(rt_tbl_entry->interface, buf, len);
				continue;
			}

			//send ARP-request;
			char *buffer = create_arp_request(rt_tbl_entry, ip_hdr);
			send_to_link(rt_tbl_entry->interface, buffer,
				 sizeof(struct ether_header) + sizeof(struct arp_header));
		}
		if (ntohs(eth_hdr->ether_type) == 0x0806)
		{
			struct arp_header *arp_hdr = (struct arp_header *)(buf +
															   sizeof(struct ether_header));
			if (ntohs(arp_hdr->op) == 1)
			{
				uint32_t router_ip_addr = get_as_uint32_ip(interface);
				if (router_ip_addr == arp_hdr->tpa)
				{
					modify_package_for_router(&eth_hdr, &arp_hdr,
											  interface, router_ip_addr);
					send_to_link(interface, buf, len);
				}
				else
				{
					struct route_table_entry *rt_tbl_entry;
					 rt_tbl_entry = &rt_tbl->vector[liniar_search(rt_tbl, arp_hdr->tpa)];
					get_interface_mac(rt_tbl_entry->interface, eth_hdr->ether_shost);
					send_to_link(rt_tbl_entry->interface, buf, len);
				}
			}
			if (ntohs(arp_hdr->op) == 2)
			{
				uint32_t router_ip_addr = get_as_uint32_ip(interface);
				if (router_ip_addr == arp_hdr->tpa) {
					if (!queue_empty(packets_queue)) {
						struct package *pack = queue_deq(packets_queue);
						struct ether_header *pack_eth = (struct ether_header*)pack->buf;
						get_interface_mac(pack->interface, pack_eth->ether_shost);
						memcpy(pack_eth->ether_dhost, arp_hdr->sha, 6);

					arp_tbl->vector[arp_tbl->size].ip = arp_hdr->spa;
					memcpy(arp_tbl->vector[arp_tbl->size].mac, arp_hdr->sha, 6);
					arp_tbl->size = arp_tbl->size + 1; 
					send_to_link(pack->interface, pack->buf, len);
					}
				} else {
					struct route_table_entry *rt_tbl_entry;
					  rt_tbl_entry = &rt_tbl->vector[liniar_search(rt_tbl, arp_hdr->tpa)];
					get_interface_mac(rt_tbl_entry->interface, eth_hdr->ether_shost);
					send_to_link(rt_tbl_entry->interface, buf, len);
				}
			}
		}
	}
	free_arp_table(arp_tbl);
	free_router_table(rt_tbl);
	free(packets_queue);
} 