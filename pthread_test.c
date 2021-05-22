#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>


int flag = 0;
pthread_cond_t waitlock = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


void *test1(void *arg) {
	char str[] = "12345";
	int n = strlen(str);
	int i;
	for (i = 0; i < n; i++) {
		pthread_mutex_lock(&lock);
		while (flag == 1) {
			pthread_cond_wait(&waitlock, &lock);
		}
		printf("%c\n", str[i]);
		flag = 1;
		pthread_mutex_unlock(&lock);
		pthread_cond_signal(&waitlock);
	}
}


void *test2(void *arg) {
	char str[] = "abcde";
        int n = strlen(str);
        int i;
        for (i = 0; i < n; i++) {
		sleep(1);
		pthread_mutex_lock(&lock);
		while (flag == 0) {
			pthread_cond_wait(&waitlock, &lock);
		}
                printf("%c\n", str[i]);
		flag = 0;
		pthread_mutex_unlock(&lock);
		pthread_cond_signal(&waitlock);
        }
}


int main(int argc, const char* argv[])
{
	pthread_t t1, t2;
	pthread_create(&t1, NULL, test1, NULL);
	pthread_create(&t2, NULL, test2, NULL);
	
	pthread_exit(NULL);
	
	return 0;
}
