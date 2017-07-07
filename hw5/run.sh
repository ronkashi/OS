sudo insmod message_slot.ko

sudo mknod /dev/simple_message_slot c 245 0

sudo chmod 0777 /dev/simple_message_slot

./message_sender 2 ido