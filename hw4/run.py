import os
import random
import time

thread_num = 20
for _ in range(10):
	for i in range(thread_num):
		if os.fork() == 0:
			os.execv("./pcc_client",["./pcc_client",str(random.choice(xrange(100)))])

	for i in range(thread_num):
		os.wait
	time.sleep(1)