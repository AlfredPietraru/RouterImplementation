## Implement the dataplane of a router.

## How to execute the project:
In order to simulate a virtual network we are going to use Mininet.
I created this project on Ubuntu 22.04 machine, so the next few commands will be 
for bash.
To install Mininet you need to use :
```
sudo apt update
sudo apt install mininet openvswitch-testcontroller tshark python3-click python3-scapy xterm
sudo pip3 install mininet
```

There is a Makefile in the root folder of the project, for compiling the project:
```
make
``` 

For launching each router individually:
```
make run_router0    
make run_router1    
```
For launching the topology, which consists of 2 routers, and 4 hosts, you need to execute from the root folder:
```
sudo python3 checker/topo.py
```    
There is a set of standard tests, in order to check their results:
```
./checker/checker.sh
```
    
    
## The actual implementation

When I first started solving, I tried to separate the packets received into two categories: those that had a struct ether_header *eth_hdr, with eth_hdr->type 0x0806 (ARP) or 0x0800 (IPv4). According to each type, different actions had to be taken.

For ARP packets, I checked if the package destination was the router. If it was an ARP request, I would give an ARP reply with the router's MAC address to establish communication between the host and the router. If it was an ARP reply, I would look in the package_queue and get the IPv4 package that was on hold, and redirect it in the right direction, now that I had the destination MAC address..

For IPv4 packets, I would check if the router was the destination. If so, I would give an echo reply. If the router had to redirect the package, I would verify the checksum. The checksum calculation process was confusing, but after some trial and error, I managed to get the correct result, verified with Wireshark. Then I decremented the TTL and calculated the next route. The algorithm was inefficient, using an O(n) linear search, as I did not know exactly how to implement the trie. I attempted a binary search by ordering the entries in the static router table in descending order based on their prefix. If two prefixes were equal, I ordered them based on their masks. However, this approach did not work correctly, so I stuck with the linear search. After finding the next hop, I recalculated the checksum.

When it was ICPM which also has an IPV4 header, I would check if the router is
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
```
typedef struct arp_table
{
	struct arp_entry *vector;
	int size;
} arp_table;
``` 

I allocate memory for the vector, and I am using it to store the mac address of already 
known hosts. I first try to iterate through every element of the arp_table->vector. If I
do not find the right pair (IPV4 address, MAC address) I send an ARP request, and I add 
the package into a queue, and wait for the response, while I am redirecting other 
packages. If I can find the pair in the arp_table, I just send the package to the right 
MAC address. 


## Problems along the way

First of all, I had multiple problems with the checker. I kept receiving "permission denied" for no apparent reason, even though I typed "sudo ./checker.sh" when using the checker. Later on, I found out that it was checker.py that did not have execute permission, and that was the problem. It took me a lot of time to figure this out, and I lost almost 4 or 5 days debugging my code, even though Wireshark did not point out any errors to me.

Another problem I faced was that I was only receiving ARP messages for some reason. When I asked other people about it, they couldn't help me, so I had to send "ARP request" and "ARP reply" first. After those were done, the first packet was an ARP packet, and the rest started to be ICMP packets. From that moment on, it became pretty simple to solve the homework.
