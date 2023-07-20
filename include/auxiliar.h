#include <stdint.h>
#include "protocols.h"
#include "lib.h"
#include "string.h"
#include <netinet/in.h>

typedef struct route_table
{
	struct route_table_entry *vector;
	int size;
} router_table;

typedef struct arp_table
{
	struct arp_entry *vector;
	int size;
} arp_table;

struct package {
	char *buf;
	int interface;
	size_t len;
};

#define MAX_ADDR_NR 100000

arp_table *init_arp_table(char *arp_pathname);
void free_arp_table(arp_table *my_arp_table);
router_table *init_router_table(const char *pathname);
void free_router_table(router_table *rt_tbl);
void swap(struct route_table_entry *entry1, struct route_table_entry *entry2);
int compare(struct route_table_entry pivot, struct route_table_entry current);
int partition(router_table *rt_tbl, int low, int high);
void quickSort(router_table *rt_tbl, int low, int high);
void print_ip_addr(uint32_t addr);
void print_mac_addr(uint8_t vector[6]);
void liniar_sorting(router_table *rt_tbl);
int liniar_search(router_table *rt_tbl, uint32_t ddar);
int verify_checksum(char *buf, struct iphdr *ip_hdr);
struct package* create_package(int interface, size_t len, char *buf);
arp_table *init_arp_table_without_file();
char *create_arp_request(struct route_table_entry *rt_tbl_entry, struct iphdr* ip_hdr);
char* create_icmp_header(char *buf, int interface, uint8_t type); 