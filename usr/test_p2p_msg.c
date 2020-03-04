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

int main(void)
{
	int my_nid = get_local_nid();
	int my_pid = getpid();

	printf("[LOCAL NID]: %d, [LOCAL PID]: %d\n", my_nid, my_pid);

	//SENDER
	if (my_nid == TEST_SRC_NID) {
		printf("HI I'm Sender NID: %d, PID: %d\n", my_nid, my_pid);

		char* msg = ">>>>>Aloha from sender nid: %d, pid: %d, you hear me?\n";

		// remote_send_reply();
	}

	// RECEIVER
	else {
		printf("HI I'm Receiver NID: %d, PID: %d\n", my_nid, my_pid);
		// remote_recv();
	}
	
}
