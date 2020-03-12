#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include "includeme.h"

#define P2P_MSG_BUFFER_SIZE 10000
#define SUCCESS_MSG_TRY	1000

int main(void)
{
	const int TEST_SRC_NID = 0;
	const int TEST_SRC_PID = 14;
	const int TEST_DST_NID = 1;
	const int TEST_DST_PID = 14;

	int my_nid = get_local_nid();
	int my_pid = getpid();

	printf("[LOCAL NID]: %d, [LOCAL PID]: %d\n", my_nid, my_pid);


	//SENDER
	if (my_nid == TEST_SRC_NID) {
		// Take a short nap in case the receiver has yet up? Or rely on the failing mechanism?
		// sleep(10);

		printf("[SENDER]: HI I'm Sender NID: %d, PID: %d\n", my_nid, my_pid);

		char* base_str = ">>>>> Aloha from sender, you hear me? Iteration: ";
		char digit[3] = "0\n\0";

		int msg_len = P2P_MSG_BUFFER_SIZE;
		char msg[P2P_MSG_BUFFER_SIZE];

		int success_deliver_count = 0;

		int retlen = P2P_MSG_BUFFER_SIZE;
		void * retbuf = malloc(P2P_MSG_BUFFER_SIZE * sizeof(char));

		while (success_deliver_count < SUCCESS_MSG_TRY) {

			memset(msg, 0, P2P_MSG_BUFFER_SIZE);
			memset(retbuf, 0, P2P_MSG_BUFFER_SIZE);

			digit[0] = (char)(success_deliver_count + '0');

			strcat(msg, base_str);
			strcat(msg, digit);

			printf("[SENDER BEFORE SEND:%d]: %s\n", success_deliver_count, msg);
			remote_send_reply(TEST_DST_NID, TEST_DST_PID, msg, msg_len, retbuf, retlen);
			printf("[SENDER DONE SEND ITE:%d]: %s\n", success_deliver_count, retbuf);
			
			success_deliver_count += 1;

			sleep(1);
		}


		printf("[SENDER]: Finish sending! 886\n");
	}

	// RECEIVER
	else {
		printf("[RECEIVER]: HI I'm Receiver NID: %d, PID: %d\n", my_nid, my_pid);

		int success_receive_count = 0;
		char recv_msg[P2P_MSG_BUFFER_SIZE];

		while (success_receive_count < SUCCESS_MSG_TRY) {
			memset(recv_msg, 0, P2P_MSG_BUFFER_SIZE);

			remote_recv(recv_msg, P2P_MSG_BUFFER_SIZE);
			printf("[RECEIVER ITE:%d]: %s\n", success_receive_count, recv_msg);

			success_receive_count += 1;
		}

		printf("[RECEIVER]: Finish receiving! 886\n");

	}
	
}
