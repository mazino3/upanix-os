 sudo brctl addbr br0
 sudo ip tuntap add tap0 mode tap
 sudo brctl addif br0 tap0
 sudo brctl addif br0 eth0
 sudo ip link set dev br0 up
 sudo ip link set dev tap0 up
