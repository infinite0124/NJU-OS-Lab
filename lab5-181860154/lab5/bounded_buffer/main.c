#include "lib.h"
#include "types.h"

void producer(int id,sem_t *empty,sem_t *mutex,sem_t *full)
{
	while(1)
	{
		sem_wait(empty);
		sem_wait(mutex);
		sleep(128);
		printf("Producer %d: produce\n", id);
		sem_post(mutex);
		sem_post(full);
		
		sleep(128);
	}
}

void consumer(sem_t *empty,sem_t *mutex,sem_t *full)
{
	while(1)
	{
		sem_wait(full);
		sem_wait(mutex);
		sleep(128);
		printf("Consumer : consume\n");
		sem_post(mutex);
		sem_post(empty);
		sleep(128);
	}
}
int main(void) {
	// TODO in lab4-ing
	printf("bounded_buffer\n");
	int k=5;
	sem_t mutex,full,empty;
	sem_init(&empty,k);
	sem_init(&full,0);
	sem_init(&mutex,1);
	int i=0;
	while(i<=4)
	{
		if(fork()==0)
		{
			if(i==0)
				consumer(&empty,&mutex,&full);
			else
				producer(i-1,&empty,&mutex,&full);
		}
		else
			i++;	
	}
	
	exit();
	return 0;
}
