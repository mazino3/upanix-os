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
