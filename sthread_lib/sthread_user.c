/* Simplethreads Instructional Thread Package
 * 
 * sthread_user.c - Implements the sthread API using user-level threads.
 *
 *    You need to implement the routines in this file.
 *
 * Change Log:
 * 2002-04-15        rick
 *   - Initial version.
 * 2005-10-12        jccc
 *   - Added semaphores, deleted conditional variables
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <sthread.h>
#include <sthread_user.h>
#include <sthread_ctx.h>
#include <sthread_time_slice.h>
#include <queue.h>
#include <generic_queue.h>



static queue_t *exe_thr_list;         /* lista de threads executaveis */
static queue_t *dead_thr_list;        /* lista de threads "mortas" */
static queue_t *sleep_thr_list;
static queue_t *join_thr_list;
static queue_t *zombie_thr_list;
static gqueue_t *mutex_list;
static gqueue_t *monitor_list;
static struct _sthread *active_thr;   /* thread activa */
static int tid_gen;                   /* gerador de tid's */


#define CLOCK_TICK 10000
static long Clock;


/*********************************************************************/
/* Part 1: Creating and Scheduling Threads                           */
/*********************************************************************/


void sthread_user_free(struct _sthread *thread);
void mutex_dump();
void monitor_dump();

void sthread_aux_start(void){
  splx(LOW);
  active_thr->start_routine_ptr(active_thr->args);
  sthread_user_exit((void*)0);
}

void sthread_user_dispatcher(void);

void sthread_user_init(void *parameters) {

  exe_thr_list = create_queue(); 
  dead_thr_list = create_queue(); 
  sleep_thr_list = create_queue(); 
  join_thr_list = create_queue(); 
  zombie_thr_list = create_queue(); 
  mutex_list = create_gqueue(); 
  monitor_list = create_gqueue(); 
  tid_gen = 1;
  Clock = 1;
  int i;
  
  
  struct _sthread *main_thread = malloc(sizeof(struct _sthread));
  main_thread->start_routine_ptr = NULL;
  main_thread->args = NULL;
  main_thread->saved_ctx = sthread_new_blank_ctx();
  main_thread->wake_time = 0;
  main_thread->join_tid = 0;
  main_thread->join_ret = NULL;
  main_thread->tid = tid_gen++;
  main_thread->prio = 4;
  main_thread->nice = 19;
  for(i=0;i<N_STATES;i++) main_thread->statistics[i]=0;
  active_thr = main_thread;

  sthread_time_slices_init(sthread_user_dispatcher, CLOCK_TICK);
}

sthread_t sthread_user_create(sthread_start_func_t start_routine, void *arg, int priority, int nice) 
//sthread_t sthread_user_create(sthread_start_func_t start_routine, void *arg)
{
  struct _sthread *new_thread = (struct _sthread*)malloc(sizeof(struct _sthread));
  sthread_ctx_start_func_t func = sthread_aux_start;
  new_thread->args = arg;
  new_thread->start_routine_ptr = start_routine;
  new_thread->wake_time = 0;
  new_thread->join_tid = 0;
  new_thread->join_ret = NULL;
  new_thread->saved_ctx = sthread_new_ctx(func);
  new_thread->prio = priority;
  new_thread->nice = nice;
  int i;
  for(i=0;i<N_STATES;i++) new_thread->statistics[i]=0;
  
  splx(HIGH);
  new_thread->tid = tid_gen++;
  queue_insert(exe_thr_list, new_thread); 
  splx(LOW);
  return new_thread;
}

void sthread_user_exit(void *ret) {
   struct _sthread *old_thr;

  splx(HIGH);
   
   int is_zombie = 1;

   // unblock threads waiting in the join list
   queue_t *tmp_queue = create_queue();       
   while (!queue_is_empty(join_thr_list)) {
      struct _sthread *thread = queue_remove(join_thr_list);
     
      //printf("Test join list: join_tid=%d, active->tid=%d\n", thread->join_tid, active_thr->tid);

      if (thread->join_tid == active_thr->tid) {
         thread->join_ret = ret;
         queue_insert(exe_thr_list,thread); 
         is_zombie = 0;
      } else {
         queue_insert(tmp_queue,thread);
      }
   }
   delete_queue(join_thr_list);
   join_thr_list = tmp_queue;
 
   if (is_zombie) {
      queue_insert(zombie_thr_list, active_thr);
   } else {
      queue_insert(dead_thr_list, active_thr);
   }
   

  if(queue_is_empty(exe_thr_list)){  /* pode acontecer se a unica thread em execucao fizer */ // Removed in BFS
    free(exe_thr_list);              /* sthread_exit(0). Este codigo garante que o programa sai bem. */
    delete_queue(dead_thr_list);
    sthread_user_free(active_thr);
    printf("Exec queue is empty!\n");
    exit(0);
  }

  
   // remove from exec list
   old_thr = active_thr;
   active_thr = queue_remove(exe_thr_list);
   sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);

   splx(LOW);
}



void sthread_user_dispatcher(void)
{
   Clock++;
   queue_t *tmp_queue = create_queue(); 

   while (!queue_is_empty(sleep_thr_list)) {
      struct _sthread *thread = queue_remove(sleep_thr_list);
      
      if (thread->wake_time == Clock) {
         thread->wake_time = 0;
         queue_insert(exe_thr_list,thread); 
      } else {
         queue_insert(tmp_queue,thread);
      }
   }
   delete_queue(sleep_thr_list);
   sleep_thr_list = tmp_queue;

   sthread_user_yield();
}


void sthread_user_yield(void)
{
  splx(HIGH);
  struct _sthread *old_thr;
  old_thr = active_thr;
  queue_insert(exe_thr_list, old_thr); 
  active_thr = queue_remove(exe_thr_list); 
  sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
  splx(LOW);
}





void sthread_user_free(struct _sthread *thread)
{
  sthread_free_ctx(thread->saved_ctx);
  free(thread);
}


/*********************************************************************/
/* Part 2: Join and Sleep Primitives                                 */
/*********************************************************************/

int sthread_user_join(sthread_t thread, void **value_ptr)
{
   /* suspends execution of the calling thread until the target thread
      terminates, unless the target thread has already terminated.
      On return from a successful pthread_join() call with a non-NULL 
      value_ptr argument, the value passed to pthread_exit() by the 
      terminating thread is made available in the location referenced 
      by value_ptr. When a pthread_join() returns successfully, the 
      target thread has been terminated. The results of multiple 
      simultaneous calls to pthread_join() specifying the same target 
      thread are undefined. If the thread calling pthread_join() is 
      canceled, then the target thread will not be detached. 

      If successful, the pthread_join() function returns zero. 
      Otherwise, an error number is returned to indicate the error. */

   
   splx(HIGH);
   // checks if the thread to wait is zombie
   int found = 0;
   queue_t *tmp_queue = create_queue(); 
   while (!queue_is_empty(zombie_thr_list)) {
      struct _sthread *zthread = queue_remove(zombie_thr_list);
      if (thread->tid == zthread->tid) {
         *value_ptr = thread->join_ret;
         queue_insert(dead_thr_list,zthread);
         found = 1;
      } else {
         queue_insert(tmp_queue,thread);
      }
   }
   delete_queue(zombie_thr_list);
   zombie_thr_list = tmp_queue;
  
   if (found) {
       splx(LOW);
       return 0;
   }

   
   // search active queue
   if (active_thr->tid == thread->tid) {
      found = 1;
   }
   
   queue_element_t *qe = NULL;

   // search exe
   qe = exe_thr_list->first;
   while (!found && qe != NULL) {
      if (qe->thread->tid == thread->tid) {
         found = 1;
      }
      qe = qe->next;
   }

   // search sleep
   qe = sleep_thr_list->first;
   while (!found && qe != NULL) {
      if (qe->thread->tid == thread->tid) {
         found = 1;
      }
      qe = qe->next;
   }

   // search join
   qe = join_thr_list->first;
   while (!found && qe != NULL) {
      if (qe->thread->tid == thread->tid) {
         found = 1;
      }
      qe = qe->next;
   }

   // if found blocks until thread ends
   if (!found) {
      splx(LOW);
      return -1;
   } else {
      active_thr->join_tid = thread->tid;
      
      struct _sthread *old_thr = active_thr;
      queue_insert(join_thr_list, old_thr);
      active_thr = queue_remove(exe_thr_list); 
      sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
  
      *value_ptr = thread->join_ret;
   }
   
   splx(LOW);
   return 0;
}


/* minimum sleep time of 1 clocktick.
  1 clock tick, value 10 000 = 10 ms */

int sthread_user_sleep(int time)
{
   splx(HIGH);
   
   long num_ticks = time / CLOCK_TICK;
   if (num_ticks == 0) {
      splx(LOW);
      
      return 0;
   }
   
   active_thr->wake_time = Clock + num_ticks;

   queue_insert(sleep_thr_list,active_thr); 

   if(queue_is_empty(exe_thr_list)){  /* pode acontecer se a unica thread em execucao fizer */
       free(exe_thr_list);              /* sleep(x). Este codigo garante que o programa sai bem. */
       delete_queue(dead_thr_list);
       sthread_user_free(active_thr);
       printf("Exec queue is empty! too much sleep...\n");
       exit(0);
   }
   sthread_t old_thr = active_thr;
   active_thr = queue_remove(exe_thr_list); 
   sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
   
   splx(LOW);
   return 0;
}

int sthread_user_nice(int nice){
	printf("Nice function not implemented!\n");
  return 0;
}

void sthread_thread_dump(sthread_t thread){
  printf("TID:%02d\tPRIO:%05d\tNICE:%05d\tWAKE:%05ld\n",
    thread->tid,
    thread->prio,
    thread->nice,
    thread->wake_time);
  sthread_update_stats(thread, thread->state);
  printf("\tVDL :%05ld\tT_SL:%05d\tRunT:%05ld\n",
    thread->vdl,
    thread->time_slice,
    thread->statistics[ST_ACTIVE]);
  printf("\tWaiT:%05ld\tSlpT:%05ld\tBlkT:%05ld\n",
    thread->statistics[ST_WAITING],
    thread->statistics[ST_SLEEPING],
    thread->statistics[ST_BLOCKED]
    );
   
}

void sthread_update_stats(sthread_t thread, int newstate){
}

int sthread_user_dump(){
    printf("<<Active Thread>>\n");
    sthread_thread_dump(active_thr);
    queue_dump(exe_thr_list,"\n<<Executable Queue>>"); 
    queue_dump(sleep_thr_list,"\n<<Sleep Queue>>");
    mutex_dump();
    monitor_dump();
    queue_dump(dead_thr_list,"\n<<Dead Queue>>");
    queue_dump(zombie_thr_list,"\n<<Zombie Queue>>");
    queue_dump(join_thr_list,"\n<<Join Queue>>");
    return 0;
}

long sthread_user_get_clock(){
  return Clock;
}


/* --------------------------------------------------------------------------*
 * Synchronization Primitives                                                *
 * ------------------------------------------------------------------------- */

/*
 * Mutex implementation
 */

struct _sthread_mutex
{
  lock_t l;
  struct _sthread *thr;
  queue_t* queue;
};

void mutex_dump(){
  char buf[128];
  
  void *temp = gqueue_first(mutex_list);
  while(temp!=NULL){
    if(((sthread_mutex_t)temp)->thr != NULL)
      sprintf(buf,"\n<<ON MUTEX LOCKED BY TID:%d>>",((sthread_mutex_t)temp)->thr->tid);
    else
      sprintf(buf,"\n<<ON MUTEX LOCKED BY NOONE!!!>>");
    queue_dump(((sthread_mutex_t)temp)->queue,buf);
    temp = gqueue_find_next(mutex_list,temp);
  }

}

sthread_mutex_t sthread_user_mutex_init()
{
  sthread_mutex_t lock;

  if(!(lock = malloc(sizeof(struct _sthread_mutex)))){
    printf("Error in creating mutex\n");
    return 0;
  }

  /* mutex initialization */
  lock->l=0;
  lock->thr = NULL;
  lock->queue = create_queue(); 
  
  gqueue_insert(mutex_list, (void *)lock);
  return lock;
}

void sthread_user_mutex_free(sthread_mutex_t lock)
{
  gqueue_find_remove(mutex_list, (void *)lock);
  delete_queue(lock->queue);
  free(lock);
}

void sthread_user_mutex_lock(sthread_mutex_t lock)
{
  while(atomic_test_and_set(&(lock->l))) {}

  if(lock->thr == NULL){
    lock->thr = active_thr;

    atomic_clear(&(lock->l));
  } else {
    queue_insert(lock->queue, active_thr);
    
    atomic_clear(&(lock->l));

    splx(HIGH);
    struct _sthread *old_thr;
    old_thr = active_thr;
    active_thr = queue_remove(exe_thr_list);  
    sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);

    splx(LOW);
  }
}

void sthread_user_mutex_unlock(sthread_mutex_t lock)
{
  if(lock->thr!=active_thr){
    printf("unlock without lock! ctid:%d ltid:%d\n",active_thr->tid,lock->thr->tid);
    return;
  }

  while(atomic_test_and_set(&(lock->l))) {}

  if(queue_is_empty(lock->queue)){
    lock->thr = NULL;
  } else {
    lock->thr = queue_remove(lock->queue);
    queue_insert(exe_thr_list, lock->thr); 

    sthread_user_yield(); // Added for preemption
  }

  atomic_clear(&(lock->l));
}

/*------------
Semaphore Implementation 
*/

struct _sthread_sem {
    unsigned int count;
    sthread_mutex_t mutex;
};

sthread_sem_t sthread_user_sem_init(unsigned int initial_count) {
	sthread_sem_t sem;
	
	if(!(sem = malloc(sizeof(struct _sthread_sem)))){
		printf("Error creating semaphore\n");
		return 0;
	}
	sem->count = initial_count;
	sem->mutex = sthread_user_mutex_init();
	return sem;
}

void sthread_user_sem_wait(sthread_sem_t s) {
	while(atomic_test_and_set(&(s->mutex->l))) {}
    
	if(s->count>0){
		s->count--;
		atomic_clear(&(s->mutex->l));
	} else {
		queue_insert(s->mutex->queue, active_thr);
		
		atomic_clear(&(s->mutex->l));
		
		splx(HIGH);
		struct _sthread *old_thr;
		old_thr = active_thr;
		// active_thr = queue_remove(exe_thr_list);
		active_thr = bfs_get_next();                // Added in BFS
		sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
		
		splx(LOW);
	}
    //return s->count;
}

void sthread_user_sem_post(sthread_sem_t s) {
    
	while(atomic_test_and_set(&(s->mutex->l))) {}
    
	if(queue_is_empty(s->mutex->queue)){
		s->count++;
	} else {
		s->mutex->thr = queue_remove(s->mutex->queue);
		// queue_insert(exe_thr_list, ->thr);
		bfs_insert_thread(s->mutex->thr);  // Added in BFS
	}
    
	atomic_clear(&(s->mutex->l));
	//    return s->count;
}

void sthread_user_sem_destroy (sthread_sem_t s){
    sthread_user_mutex_free(s->mutex);
    free(s);
    
}


/*
 * Readers/Writer implementation
 */

struct _sthread_rwlock {
    int nleitores; 
    int /*boolean_t*/ em_escrita;
    int leitores_espera;
    int escritores_espera;
    sthread_mutex_t m;
    sthread_sem_t leitores;
    sthread_sem_t escritores;
};

void sthread_user_rwlock_destroy(sthread_rwlock_t rwlock){

    sthread_user_mutex_free(rwlock->m);
    sthread_user_sem_destroy(rwlock->leitores);
    sthread_user_sem_destroy(rwlock->escritores);
    free(rwlock);

}

sthread_rwlock_t sthread_user_rwlock_init(){

    sthread_rwlock_t rwlock;

  if(!(rwlock = malloc(sizeof(struct _sthread_rwlock)))){
    printf("Error in creating rwlock\n");
    return 0;
  }

    rwlock->leitores = sthread_user_sem_init(0) ;
    rwlock->escritores = sthread_user_sem_init(0);
    rwlock->m = sthread_user_mutex_init();
    rwlock->nleitores=0;
    rwlock->em_escrita=0; /*FALSE;*/
    rwlock->leitores_espera=0;
    rwlock->escritores_espera=0;
  
    return rwlock;

}

void sthread_user_rwlock_rdlock(sthread_rwlock_t rwlock){

    sthread_user_mutex_lock(rwlock->m);
    if (rwlock->em_escrita || rwlock->escritores_espera > 0) {
	rwlock->leitores_espera++;
	sthread_user_mutex_unlock(rwlock->m);
	sthread_user_sem_wait(rwlock->leitores);
	sthread_user_mutex_lock(rwlock->m);
    }
    else{
	rwlock->nleitores++;
    }
    sthread_user_mutex_unlock(rwlock->m);

}

void sthread_user_rwlock_wrlock(sthread_rwlock_t rwlock){

    sthread_user_mutex_lock(rwlock->m);
    if (rwlock->em_escrita || rwlock->nleitores > 0) {
	rwlock->escritores_espera++;
	sthread_user_mutex_unlock(rwlock->m);
	sthread_user_sem_wait(rwlock->escritores);
	sthread_user_mutex_lock(rwlock->m);
    }
    else{
	rwlock->em_escrita = 1; /*TRUE;*/
    }
    sthread_user_mutex_unlock(rwlock->m);   
   
}


void sthread_user_rwlock_unlock(sthread_rwlock_t rwlock){
    int i;
    sthread_user_mutex_lock(rwlock->m);
    
    if (/*TRUE*/ 1== rwlock->em_escrita) { /* writer unlock*/
	
	rwlock->em_escrita = 0; /*FALSE; */
	if (rwlock->leitores_espera > 0){
	    for (i=0; i< rwlock->leitores_espera; i++) {
		sthread_user_sem_post(rwlock->leitores);
		rwlock->nleitores++;
	    }
	    rwlock->leitores_espera -= i;
        }else{
           if (rwlock->escritores_espera > 0) {
	     sthread_user_sem_post(rwlock->escritores);
	     rwlock->em_escrita=1; /*TRUE;*/
	     rwlock->escritores_espera--;
	    }
	}
    }else{ /* reader unlock*/
	
	rwlock->nleitores--;
	if (rwlock->nleitores == 0 && rwlock->escritores_espera > 0){
	    sthread_user_sem_post(rwlock->escritores);
	    rwlock->em_escrita=1; /*TRUE;*/
	    rwlock->escritores_espera--;
	}
    }
    sthread_user_mutex_unlock(rwlock->m);    
}

/*
 * Monitor implementation
 */

struct _sthread_mon {
 	sthread_mutex_t mutex;
	queue_t* queue;
};

void monitor_dump(){
  
  void *temp = gqueue_first(monitor_list);
  while(temp!=NULL){
    queue_dump(((sthread_mon_t)temp)->queue,"<<ON MONITOR>>");
    temp = gqueue_find_next(mutex_list,temp);
  }

}


sthread_mon_t sthread_user_monitor_init()
{
  sthread_mon_t mon;
  if(!(mon = malloc(sizeof(struct _sthread_mon)))){
    printf("Error creating monitor\n");
    return 0;
  }

  mon->mutex = sthread_user_mutex_init();
  mon->queue = create_queue(); 
  gqueue_insert(monitor_list, (void *)mon);
  return mon;
}

void sthread_user_monitor_free(sthread_mon_t mon)
{
  sthread_user_mutex_free(mon->mutex);
  gqueue_find_remove(monitor_list, (void *)mon);
  delete_queue(mon->queue);
  free(mon);
}

void sthread_user_monitor_enter(sthread_mon_t mon)
{
  sthread_user_mutex_lock(mon->mutex);
}

void sthread_user_monitor_exit(sthread_mon_t mon)
{
  sthread_user_mutex_unlock(mon->mutex);
}

void sthread_user_monitor_wait(sthread_mon_t mon)
{
  struct _sthread *temp;

  if(mon->mutex->thr != active_thr){
    printf("monitor wait called outside monitor\n");
    return;
  }

  /* inserts thread in queue of blocked threads */
  temp = active_thr;
  queue_insert(mon->queue, temp);

  /* exits mutual exclusion region */
  sthread_user_mutex_unlock(mon->mutex);

  splx(HIGH);
  struct _sthread *old_thr;
  old_thr = active_thr;
  active_thr = queue_remove(exe_thr_list); 
  sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
  splx(LOW);
}

void sthread_user_monitor_signal(sthread_mon_t mon)
{
  struct _sthread *temp;

  if(mon->mutex->thr != active_thr){
    printf("monitor signal called outside monitor\n");
    return;
  }

  while(atomic_test_and_set(&(mon->mutex->l))) {}
  if(!queue_is_empty(mon->queue)){
    /* changes blocking queue for thread */
    temp = queue_remove(mon->queue);
    queue_insert(mon->mutex->queue, temp);
  } else
    printf("queue is empty\n");
  atomic_clear(&(mon->mutex->l));
}

void sthread_user_monitor_signalall(sthread_mon_t mon)
{
  struct _sthread *temp;

  if(mon->mutex->thr != active_thr){
    printf("monitor signalall called outside monitor\n");
    return;
  }

  while(atomic_test_and_set(&(mon->mutex->l))) {}
  while(!queue_is_empty(mon->queue)){
    /* changes blocking queue for thread */
    temp = queue_remove(mon->queue);
    queue_insert(mon->mutex->queue, temp);
  }
  atomic_clear(&(mon->mutex->l));
}


/* The following functions are dummies to 
 * highlight the fact that pthreads do not
 * include monitors.
 */

sthread_mon_t sthread_dummy_monitor_init()
{
   printf("WARNING: pthreads do not include monitors!\n");
   return NULL;
}


void sthread_dummy_monitor_free(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_enter(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_exit(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_wait(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_signal(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}

void sthread_dummy_monitor_signalall(sthread_mon_t mon)
{
   printf("WARNING: pthreads do not include monitors!\n");
}
