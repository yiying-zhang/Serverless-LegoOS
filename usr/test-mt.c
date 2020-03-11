#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void printMsg(int num) {
    int i = 0;
    for (i = 0; i < 10; i++) {
        printf("Thread number is %d", num);
    }
}

void main() {
    pthread_t tid1, tid2; 
    pthread_create(&tid1, NULL, thread1, 1);
    pthread_create(&tid1, NULL, thread1, 2);
    pthread_join(tid1, NULL); 
    pthread_join(tid2, NULL); 
}