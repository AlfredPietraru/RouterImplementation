Repository for the first homework of the Communication Networks class. In this homework 
the students will implement the dataplane of a router.

First of all I had multiple problems with the checker, I kept receiving permission denied
for no apparent reason, even though I typped "sudo ./checker.sh" when I was using the 
checker. Later on I found that, it was checker.py that did not have execute permission,
and that was the problem. I could not figure it out for a lot of time, and I lost almost 
4 or 5 days debugging my code even thoguh wiresark did not pointed out any error to me.

Another problem I faced was that I wass only receiving ARP messages for some reason. When 
I asked other people about it, they could not give me any help, so I had to do the "arp 
request" and "arp reply first". After those were done, the first packet was an ARP and the 
rest started to be ICMP type, and from that moment on it became pretty simple to solve the 
homework. 

When I first started solving, I tried to separete  the packets received on two categories:
those who had at struct ether_header *eth_hdr, on eth_hdr->type 0x0806 (ARP) or 0x0800 
IPV4, and according to each type different things had to be done. 

    When it was ARP, I was checking if the package destination is the router, if it was an 
ARP request, I would give an ARP reply with the router's MAC address, in order to 
establish communication between the host and the router. If it was an ARP reply, I would 
look in the package_queue and get the IPV4 package that was on hold, and redirect it the 
right direction since I received the destination MAC address.

    When it was IPCM which also has an IPV4 header, I would check if the router is
destination, if so, I would give an echo reply. If the router only has to redirect the
package, I would verify the checksum, it was really confusing, because I did 
not knew exactly where it end the process of calculating the checksum, and I got the 
current result based on trail and error, until I saw in wireshark that the checksum is 
correct. Than I decrement the ttl, easy points, and calculate the next route. The 
algotithm is ineficient to be fair. It is an O(n) liniar search, I did not know exactly 
how to implement the trie. I initially tried with a binary search, I ordered the entries 
in the static router table, in decending order according with their prefix, and if two 
prefixes are equal, also in decending order by comparing the masks. But it does not work 
properly so I let it be. After finding the next hop, I recalculate the checksum, and the 
real fun begins. I have to figure out the mac address of the next_hop. I refused to use 
the "arp_table.txt" file because I wanted the ARP points. So first of all I create my own

typedef struct arp_table
{
	struct arp_entry *vector;
	int size;
} arp_table; 

I allocate memory for the vector, and I am using it to store the mac address of already 
known hosts. I first try to iterate through every element of the arp_table->vector. If I
do not find the right pair (IPV4 address, MAC address) I send an ARP request, and I add 
the package into a queue, and wait for the response, while I am redirecting other 
packages. If I can find the pair in the arp_table, I just send the package to the right 
MAC address. 