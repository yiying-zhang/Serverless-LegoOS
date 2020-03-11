#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include "includeme.h"

#define P2P_MSG_BUFFER_SIZE 100
#define SUCCESS_MSG_TRY	1000

typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef void (*fun_ptr)(void);

double mean(int* x, int n) {
	double tot = 0;
	for (int i = 0; i < n; i++) {
		tot += x[i];
	}
	return tot/n;
}

double median(int* x, int n) {
    int temp;
    int i, j;
    // the following two loops sort the array x in ascending order
    for(i=0; i<n-1; i++) {
        for(j=i+1; j<n; j++) {
            if(x[j] < x[i]) {
                // swap elements
                temp = x[i];
                x[i] = x[j];
                x[j] = temp;
            }
        }
    }

    if(n%2==0) {
        // if there is an even number of elements, return mean of the two elements in the middle
        return((x[n/2] + x[n/2 - 1]) / 2.0);
    } else {
        // else return the element in the middle
        return x[n/2];
    }
}

long benchmarkSendReply(int dest_nid, int dest_pid, void* msg, int msg_len, void* retbuf, int retlen) {
    uint32_t cycles_high0, cycles_low0, cycles_low1, cycles_high1;
    asm volatile (
    "CPUID\n\t"
        "RDTSC\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t": "=r" (cycles_high0), 
    "=r" (cycles_low0)::"rax", "%rbx", "%rcx", "%rdx"
    );

    // time function below
    remote_send_reply(dest_nid, dest_pid, msg, msg_len, retbuf, retlen);

    asm volatile (
        "RDTSCP\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t": "=r" (cycles_high1), 
    "=r" (cycles_low1)::"rax", "%rbx", "%rcx", "%rdx"
    );

    uint64_t start = ((uint64_t)cycles_high0 << 32) | cycles_low0;
    uint64_t end = ((uint64_t)cycles_high1 << 32) | cycles_low1;

    return end-start;	
}

int main(void)
{
	const int TEST_SRC_NID = 0;
	const int TEST_SRC_PID = 14;
	const int TEST_DST_NID = 1;
	const int TEST_DST_PID = 14;

	int my_nid = get_local_nid();
	int my_pid = getpid();

	printf("[LOCAL NID]: %d, [LOCAL PID]: %d\n", my_nid, my_pid);

	int cycleCounts[SUCCESS_MSG_TRY] = {0};

	//SENDER
	if (my_nid == TEST_SRC_NID) {
		// Take a short nap in case the receiver has yet up? Or rely on the failing mechanism?
		// sleep(10);

		fprintf(stderr, "[SENDER]: HI I'm Sender NID: %d, PID: %d\n", my_nid, my_pid);

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

			// fprintf(stderr, "[SENDER BEFORE SEND:%d]: %s\n", success_deliver_count, msg);

			
			long cycles = benchmarkSendReply(TEST_DST_NID, TEST_DST_PID, 
                    msg, msg_len, retbuf, retlen);

			// fprintf(stderr, "[SENDER DONE SEND ITE:%d]: %s\n", success_deliver_count, retbuf);
			fprintf(stderr, "[SENDER FINISHED IN %ld cycles ]\n", cycles);

			cycleCounts[success_deliver_count] = cycles;
			success_deliver_count += 1;
			sleep(.01);
		}

		fprintf(stderr, "Median: %f, Mean: %f over %d trials\n",
			median(cycleCounts, SUCCESS_MSG_TRY),
			mean(cycleCounts, SUCCESS_MSG_TRY),
			SUCCESS_MSG_TRY
		);
		fprintf(stderr, "~~~~~~~~~~~~~~~~~~~ New Message ~~~~~~~~~~~~~~~~~~~~~~~~\n");
		fprintf(stderr, "[SENDER]: Finish sending! 886\n");
	}

	// RECEIVER
	else {
		printf("[RECEIVER]: HI I'm Receiver NID: %d, PID: %d\n", my_nid, my_pid);

		int success_receive_count = 0;
		char recv_msg[P2P_MSG_BUFFER_SIZE];

		while (success_receive_count < SUCCESS_MSG_TRY) {
			memset(recv_msg, 0, P2P_MSG_BUFFER_SIZE);

			remote_recv(recv_msg, P2P_MSG_BUFFER_SIZE);

			success_receive_count += 1;
		}

		printf("[RECEIVER]: Finish receiving! 886\n");

	}
	
}
