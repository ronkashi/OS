sudo insmod message_slot.ko

sudo mknod /dev/simple_message_slot c 245 0
# sudo mknod /dev/simple_message_slot_1 c 245 0
# sudo mknod /dev/simple_message_slot_2 c 245 0

sudo chmod 0777 /dev/simple_message_slot
# sudo chmod 0777 /dev/simple_message_slot_1
# sudo chmod 0777 /dev/simple_message_slot_2

# ./message_sender_plus_arg 2 kerrrreeenn /dev/simple_message_slot_1
# ./message_sender_plus_arg 3 mess_to_2 /dev/simple_message_slot_2

# ./message_reader_plus_arg 2 /dev/simple_message_slot_1
# ./message_reader_plus_arg 3 /dev/simple_message_slot_2


./message_sender 2 hhhhhhhh
./message_reader 2