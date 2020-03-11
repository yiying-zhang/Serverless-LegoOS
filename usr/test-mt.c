#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *printMsg(void *num) {
    int i = 0;
    int *num_ptr = (int *)num;
    for (i = 0; i < 10; i++) {
        printf("Thread number is %d", *num_ptr);
    }
}

void main() {
    pthread_t tid1, tid2; 
    int a = 1;
    int b = 2;
    pthread_create(&tid1, NULL, printMsg, &a);
    pthread_create(&tid1, NULL, printMsg, &b);
    pthread_join(tid1, NULL); 
    pthread_join(tid2, NULL); 
}