#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include "includeme.h"

#define P2P_MSG_BUFFER_SIZE 100

#define TEST_SRC_NID 0
#define TEST_SRC_PID 3
#define TEST_DST_NID 1
#define TEST_DST_PID 3

#define SUCCESS_MSG_TRY	5

int main(void)
{
	int my_nid = get_local_nid();
	int my_pid = getpid();

	printf("[SENDER]: [LOCAL NID]: %d, [LOCAL PID]: %d\n", my_nid, my_pid);

	// Take a short nap in case the receiver has yet up? Or rely on the failing mechanism?

	//SENDER
	if (my_nid == TEST_SRC_NID) {
		printf("[SENDER]: HI I'm Sender NID: %d, PID: %d\n", my_nid, my_pid);

		char* msg = ">>>>>Aloha from sender, you hear me?\n";

		int success_deliver_count = 0;

		while (success_deliver_count < SUCCESS_MSG_TRY) {
			//remote_send
			// success += 1
		}


		printf("[SENDER]: Finish sending! 886\n");
	}

	// RECEIVER
	else {
		printf("[RECEIVER]: HI I'm Receiver NID: %d, PID: %d\n", my_nid, my_pid);

		int success_receive_count = 0;

		while (success_receive_count < SUCCESS_MSG_TRY) {
			// remote_recv();
			// print out the received msg
			// success += 1
		}

		printf("[RECEIVER]: Finish receiving! 886\n");

	}
	
}
