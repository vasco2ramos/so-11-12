/* 
 * test-create.c
 *
 * Simple test of thread create
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <sthread.h>


void *thread_start(void *);


int success = 0;


int main(int argc, char **argv)
{
  sthread_t thr;
  void *ret;
  /* int i;*/

	printf("Testing sthread_join, impl: %s\n",
		   (sthread_get_impl() == STHREAD_PTHREAD_IMPL) ? "pthread" : "user");
	
  int parameter = 300;
  	
	sthread_init(&parameter);
    
	if ((thr = sthread_create(thread_start, (void*)1, 1, 0)) == NULL) {
		exit(-1);
	}
    
	sthread_join(thr, &ret);
	printf("back in main\n");
	return 0;
}


void *thread_start(void *arg)
{
  int i;
	printf("In thread_start, arg = %d\n", (int)arg);
	  for(i = 0; i < 300000000; i++);
	printf("In thread_start, ending\n");
	return 0;
}
