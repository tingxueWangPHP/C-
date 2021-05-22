#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_NUM 3

typedef struct tpool_work{
   void* (*work_routine)(void*); //function to be called
   void* args;                   //arguments 
   struct tpool_work* next;
}tpool_work_t;
 
typedef struct tpool{
   size_t               shutdown;       //is tpool shutdown or not, 1 ---> yes; 0 ---> no
   size_t               maxnum_thread; // maximum of threads
   pthread_t            *thread_id;     // a array of threads
   tpool_work_t*        tpool_head;     // tpool_work queue
   pthread_cond_t       queue_ready;    // condition varaible
   pthread_mutex_t      queue_lock;     // queue lock
}tpool_t;

//int create_tpool(tpool_t** pool,size_t max_thread_num);

tpool_t * create_tpool();

void destroy_tpool(tpool_t* pool);

int add_task_2_tpool(tpool_t* pool,void* (*routine)(void*),void* args);

void *work_routine(void *arg) {
	//pthread_detach(pthread_self());
	tpool_t *tpool_p = (tpool_t *)arg;
	tpool_work_t *work_p  = NULL;
	while (1) {
		pthread_mutex_lock(&tpool_p->queue_lock);
		while (tpool_p->tpool_head == NULL && tpool_p->shutdown == 0) {
			pthread_cond_wait(&tpool_p->queue_ready, &tpool_p->queue_lock);
		}

		if (tpool_p->shutdown == 1) {
			pthread_mutex_unlock(&tpool_p->queue_lock);
			return ;
		}
	
		work_p = tpool_p->tpool_head;
		tpool_p->tpool_head = tpool_p->tpool_head->next;

		pthread_mutex_unlock(&tpool_p->queue_lock);
	
		work_p->work_routine(work_p->args);
		
		free(work_p);

		pthread_yield();
	}
	
	return NULL;
	
}

void *task(void *arg) {
	printf("pthread_id is %ld\n", pthread_self());
	//puts("123");
	return NULL;
}

int main() {
	tpool_t *p = create_tpool();
	int i;
	for (i=0;i<100;i++) {
		add_task_2_tpool(p, task, NULL);
	}
	sleep(1);
	destroy_tpool(p);
	return 0;
}

tpool_t * create_tpool() {
	tpool_t *p = (tpool_t *)malloc(sizeof(tpool_t));
	if (p == NULL) {
		printf("malloc error is %s\n", strerror(errno));
		exit(1);
	}
	
	p->shutdown = 0;
	p->maxnum_thread = MAX_NUM;
	p->thread_id = (pthread_t *)malloc(MAX_NUM*sizeof(pthread_t));
	p->tpool_head = NULL;
	
	pthread_cond_init(&p->queue_ready, NULL);
	pthread_mutex_init(&p->queue_lock, NULL);

	int i;
	for (i=0;i<MAX_NUM;i++) {
		pthread_create(p->thread_id+i, NULL, work_routine, (void *)p);
	}

	return p;
}

int add_task_2_tpool(tpool_t* tpool_p,void* (*routine)(void*),void* args) {
	tpool_work_t *p = (tpool_work_t *)malloc(sizeof(tpool_work_t));
	if (p == NULL) {
		printf("malloc error is %s\n", strerror(errno));
                exit(1);		
	}

	p->work_routine = routine;
	p->args = args;
	p->next = NULL;

	tpool_work_t *temp_work;

	pthread_mutex_lock(&tpool_p->queue_lock);
	if (tpool_p->tpool_head == NULL) {
		tpool_p->tpool_head = p;
	} else {
		temp_work = tpool_p->tpool_head;
        	while (temp_work->next != NULL) {
                	temp_work = temp_work->next;
        	}
        	temp_work->next = p;
	}
	pthread_cond_signal(&tpool_p->queue_ready);
	pthread_mutex_unlock(&tpool_p->queue_lock);
	
	return 0;	
}


void destroy_tpool(tpool_t* tpool_p) {
	if (tpool_p->shutdown == 1) {
		return ;
	}

	tpool_work_t *temp;

	tpool_p->shutdown = 1;
	pthread_cond_broadcast(&tpool_p->queue_ready);
	
	int i;
	for (i=0;i<tpool_p->maxnum_thread;i++) {
		pthread_join(tpool_p->thread_id[i], NULL);
	}

	
	
	while (tpool_p->tpool_head) {
		temp = tpool_p->tpool_head;
		tpool_p->tpool_head = tpool_p->tpool_head->next;
		free(temp);
	}

	

	free(tpool_p->thread_id);
	
	pthread_cond_destroy(&tpool_p->queue_ready);
	pthread_mutex_destroy(&tpool_p->queue_lock);
	
	free(tpool_p);
	
}







