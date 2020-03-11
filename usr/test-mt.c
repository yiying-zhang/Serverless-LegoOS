#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 100

struct thread_data
{
  int  thread_id;
  char *message;
};


void *PrintHello(void *threadarg)
{
   struct thread_data *my_data;   

   my_data = (struct thread_data *) threadarg;

   int i;
   for (i = 0; i < 100; i++) {
       printf("%d", my_data->thread_id);
   }

   //printf("Thread ID : %d", my_data->thread_id);

   //printf("Message : %s", my_data->message);

   pthread_exit(NULL);
}

int main ()
{
   pthread_t threads[NUM_THREADS];

   struct thread_data td[NUM_THREADS];

   int rc, i;


   for( i=0; i < NUM_THREADS; i++ )    
   {

      printf("main() : creating thread %d", i);

      td[i].thread_id = i;

      td[i].message = "This is message";

      rc = pthread_create(&threads[i], NULL, PrintHello, (void *)&td[i]);

      if (rc){

         printf("Error : unable to create thread, %d", rc);

         exit(-1);    
      }    
   }    
   pthread_exit(NULL);    
}