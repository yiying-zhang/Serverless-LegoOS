#include "includeme.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define NR_THREADS 1
#define TOTAL_PAYLOAD_SIZE 1024
#define SINGLE_PAYLOAD_SIZE TOTAL_PAYLOAD_SIZE/NR_THREADS
#define SUCCESS_MSG_TRY 3

#define TEST_SRC_NID 0
#define TEST_SRC_PID 18
#define TEST_DST_NID 1
#define TEST_DST_PID 18

struct thread_data {
    int  thread_id;
    char * msg;
    void * retbuf;
};

// // print struct ID 100 times
// void *printHundredIDs(void *threadarg) {
//     struct thread_data *my_data;   

//     my_data = (struct thread_data *) threadarg;
//     int i;

//     for (i = 0; i < 100; i++) {
//         printf("Thread ID : %d ", my_data->thread_id);
//     }

//     pthread_exit(NULL);
// }

static pthread_spinlock_t trial_result_lock;
// static pthread_barrier_t send_finish_barrier;
static int nr_active_thread;
static struct timeval ts, te;


static void *thread_func(void *arg)
{
    int tid = gettid();
    printf("Thread [%d] running\n", tid);

    // struct thread_data *my_data = (struct thread_data *) arg;

    // int is_leader = 0;
    // pthread_spin_lock(&trial_result_lock);
    // nr_active_thread += 1;
    // if (nr_active_thread == 1) { is_leader = 1; }
    // pthread_spin_unlock(&trial_result_lock);

    // printf("Thread [%d] waiting before barrier\n", tid);
    // // pthread_barrier_wait(&send_finish_barrier);
    // printf("Thread [%d] pass the barrier\n", tid);

    // if (is_leader) { gettimeofday(&ts, NULL); }

    // printf("Thread [%d] before remote send\n", tid);
    // remote_send_reply(TEST_DST_NID, TEST_DST_PID, my_data->msg, SINGLE_PAYLOAD_SIZE, my_data->retbuf, SINGLE_PAYLOAD_SIZE);
    // printf("Thread [%d] back from remote send\n", tid);

    // pthread_spin_lock(&trial_result_lock);
    // nr_active_thread -= 1;
    // if (nr_active_thread == 0) { gettimeofday(&te, NULL); }
    // pthread_spin_unlock(&trial_result_lock);
}

int spawn_thread_and_send(struct timeval * time_span, pthread_t * tid, struct thread_data * td) {

    // pthread_barrier_init(&send_finish_barrier, NULL, NR_THREADS);
    pthread_spin_init(&trial_result_lock, PTHREAD_PROCESS_PRIVATE);

    printf("Finished barrier setup and spin init\n");

    int thread_init_failed = 0;
    for (int i = 0; i < NR_THREADS; i++) {
        int ret = pthread_create(&tid[i], NULL, thread_func, (void *)&td[i]);
        if (ret) {
            die("fail to create new thread");
            thread_init_failed = 1;
            break;
        }
    }
    if (thread_init_failed) { return 0; }

    printf("Thread creation all done\n");
    for (int i = 0; i < NR_THREADS; i++) {
        pthread_join(tid[i], NULL);
    }

    printf("Thread all joined\n");

    timeval_sub(time_span, &te, &ts);

    return 1;
}

// Create 100 threads and print out struct ID 100 times in each thread 
int main() {

    int my_nid = get_local_nid();
    int my_pid = getpid();

    printf("[LOCAL NID]: %d, [LOCAL PID]: %d\n", my_nid, my_pid);
    printf("\t[NR_THREADS]: %d\n\t[TOTAL_PAYLOAD_SIZE] %d\n\t[SINGLE_PAYLOAD_SIZE] %d\n",
        NR_THREADS, TOTAL_PAYLOAD_SIZE, SINGLE_PAYLOAD_SIZE);

    if (my_nid == TEST_SRC_NID) {
        printf("[SENDER]: HI I'm Sender NID: %d, PID: %d\n", my_nid, my_pid);

        struct thread_data td[NR_THREADS];

        /* Thread data initialization stage */
        int msg_len = SINGLE_PAYLOAD_SIZE;
        for (int i = 0; i < NR_THREADS; i++) {
            td[i].thread_id = i;
            td[i].msg = malloc(SINGLE_PAYLOAD_SIZE);

            if ( td[i].msg == NULL) {
                printf("msg init failed!\n");
                break;
            }
        }

        int retlen = SINGLE_PAYLOAD_SIZE;
        for (int i = 0; i < NR_THREADS; i++) {
            td[i].thread_id = i;
            td[i].retbuf = malloc(SINGLE_PAYLOAD_SIZE);

            if ( td[i].retbuf == NULL) {
                printf("retbuf init failed!\n");
                break;
            }
        }

        int success_deliver_count = 0;
        struct timeval final_result;
        timeval_sub(&final_result, &final_result, &final_result);

        pthread_t tid[NR_THREADS];

        while (success_deliver_count < SUCCESS_MSG_TRY) {

            nr_active_thread = 0;

            char num_buffer[20];
            sprintf(num_buffer, "%d", success_deliver_count); 

            for (int i = 0; i < NR_THREADS; i++) {
                td[i].msg = malloc(SINGLE_PAYLOAD_SIZE);
                memset(td[i].msg, 0, msg_len);
                strcat(td[i].msg, num_buffer);
            }

            struct timeval single_exp_time;

            printf("[SENDER BEFORE SEND:%d]: %s\n", success_deliver_count, td[0].msg);
            int exp_ret = spawn_thread_and_send(&single_exp_time, tid, td);
            if (exp_ret == 0) {
                printf("Experiment Failed: Trail [%d]\n", success_deliver_count);
                continue;
            }

            timeval_add_res(&final_result, &final_result, &single_exp_time);
            success_deliver_count += 1;
            printf("[SENDER DONE SEND ITE:%d]: %s\n", success_deliver_count);

            fprintf(stderr, "(nr=%d)(tot_payload=%d)\tTotal Time [%ld.%ld (s)]\tAverage Time [%ld (ns)]\n",
                success_deliver_count, TOTAL_PAYLOAD_SIZE, final_result.tv_sec, final_result.tv_usec/1000,
                (1000000000*final_result.tv_sec + 1000*final_result.tv_usec)/success_deliver_count);
        }

        printf("[SENDER]: Finish sending! 886\n");
    }
    else {

        printf("[RECEIVER]: HI I'm Receiver NID: %d, PID: %d\n", my_nid, my_pid);

        int success_receive_count = 0;
        char recv_msg[SINGLE_PAYLOAD_SIZE];

        while (success_receive_count < SUCCESS_MSG_TRY * NR_THREADS) {
            memset(recv_msg, 0, SINGLE_PAYLOAD_SIZE);

            remote_recv(recv_msg, SINGLE_PAYLOAD_SIZE);
            printf("[RECEIVER ITE:%d]: %s\n", success_receive_count, recv_msg);

            success_receive_count += 1;
        }

        printf("[RECEIVER]: Finish receiving! 886\n");
    }


}