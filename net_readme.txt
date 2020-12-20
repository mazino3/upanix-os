#Show status of network bridge br0, whether it is UP or DOWN. You can see find it's MAC address
sudo ip link show br0

#Show status of network tap0, whether it is UP or DOWN. You can also see it's MAC address
sudo ip link show tap0

#send ARP broadcast packet to br0 with source address as 0.0.0.0 asking who as IP 10.10.10.10
#Upanix will receive this ARP packet
sudo arping -i br0 -S 0.0.0.0 10.10.10.10

#send ARP broadcast packet to tap0 with source address as 0.0.0.0 asking who as IP 10.10.10.10
#Upanix will receive this ARP packet
sudo arping -i tap0 -S 0.0.0.0 10.10.10.10

#send IPV4 packet to MAC address 52:54:00:12:34:56 - this is the MAC address of Upanix E1000 NIC device
#but because this is on eth0, Upanix will not receive this
sudo arping -i eth0 -S 0.0.0.0 52:54:00:12:34:56

#send IPV4 packet to MAC address 52:54:00:12:34:56 - this is the MAC address of Upanix E1000 NIC device
#because this is on tap0, Upanix will receive this
sudo arping -i tap0 -S 0.0.0.0 52:54:00:12:34:56

#send IPV4 packet to MAC address 52:54:00:12:34:56 - this is the MAC address of Upanix E1000 NIC device
#because this is on br0, Upanix will receive this
sudo arping -i br0 -S 0.0.0.0 52:54:00:12:34:56

#If Upanix receives ARP and IPV4 packets from above commands, then the Source MAC address of the packet will be of either br0 or tap0 resp.

sudo nmap --script broadcast-dhcp-discover
sudo nmap --script broadcast-dhcp-discover -e eth0
sudo nmap --script broadcast-dhcp-discover -e br0
sudo nmap --script broadcast-dhcp-discover -e tap0
sudo nmap --script broadcast-dhcp-discover -e tap0

iptables -I FORWARD -i br0 -j ACCEPT
iptables -I FORWARD -o br0 -j ACCEPT


auto lo
iface lo inet loopback

auto  eth0
iface eth0 inet manual

auto bridge0
iface bridge0 inet static
address   <your public ip>
netmask   <netmask value>
gateway   <your gateway>
bridge_ports eth0
bridge_stp off       # disable Spanning Tree Protocol
bridge_waitport 0    # no delay unless port available
bridge_fd 0          # no forwarding delay

dhclient -v br0

#command to make services running on WSL2 visible to other computers on the windows host subnet/network by means of port forwarding - this is the command to be run on windows powershell and
connectaddress is the dynamic address that wsl2 eth nic would have got - which changes everytime you start wsl2
netsh interface portproxy add v4tov4 listenport=3000 listenaddress=0.0.0.0 connectport=3000 connectaddress=172.30.16.3

after adding br0 and setting ip, netmask and broadcast same as eth0, erase ip for eth0 and then add below to route table
sudo ip route add default via 172.17.16.1 dev br0
