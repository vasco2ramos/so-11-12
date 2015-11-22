/*
 * test-time-slice.c
 *
 * Simple time slice test. There are no yield calls here, so everything 
 * depends on time slices working correctly. There are two threads 
 * ping-ponging between each other - correct output is ping 1, pong 1, 
 * ping 2, pong 2, etc.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sthread.h>


static int ping = 0;
static int t1_complete = 0;
static int t2_complete = 0;

void *thread_1(void *);

void *thread_2(void *);


int main(int argc, char **argv)
{
/*  int j; */

   int count = argc > 1 ? atoi(argv[1]) : 100;
   printf("Testing time slice and mutexes, impl: %s\n",
      (sthread_get_impl() == STHREAD_PTHREAD_IMPL) ? "pthread" : "user");

  int parameter = 300;
  	
	sthread_init(&parameter);

   if (sthread_create(thread_1, (void*)count, 4,0) == NULL) {
      printf("sthread_create failed\n");
      exit(1);
   }

   if (sthread_create(thread_2, (void*)count, 4,0) == NULL) {
      printf("sthread_create failed\n");
      exit(1);
   }
   printf("created two threads\n");

   printf("if this is the last line of output, time slices aren't working!\n");

   while(!t1_complete || !t2_complete);

   printf("PASSED\n");
   return 0;
}


void *thread_1(void *arg)
{
   int i=0;
   while(i<(int)arg) {
     //printf("[1]\n");
      if(ping == 0) {
         ping = 1;
         i++;
         printf("[1] ping %d\n", i);
      } 
   }
   t1_complete = 1;
   return 0;
}


void *thread_2(void *arg)
{
   int i=0;
   while(i<(int)arg) {
     //printf("[2]\n");
      if(ping == 1) {
         ping = 0;
         i++;
         printf("[2] pong %d\n", i);
      }
   }
   t2_complete = 1;
   return 0;
}
