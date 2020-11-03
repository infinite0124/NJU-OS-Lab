#include "lib.h"
#include "types.h"
#define N 5
sem_t forks[5];

void philosopher(int i){
	while(1){
		printf("Philosopher %d: think\n", i);// think
		sleep(128);
		if(i%2==0){
			sem_wait(&forks[i]); // get left fork
			sem_wait(&forks[(i+1)%N]); // get right fork
		} else {
			sem_wait(&forks[(i+1)%N]); // get right fork
			sem_wait(&forks[i]); // get left fork
		}
		printf("Philosopher %d: eat\n", i); // eat
		sleep(128);
		sem_post(&forks[i]); // put down left fork
		sem_post(&forks[(i+1)%N]); // put down right fork
	}
}
int main(void) {
	// TODO in lab4-ing
	printf("philosopher\n");
	for(int i=0;i<5;i++)
		sem_init(&forks[i],1);
	int order[5]={1,3,0,2,4};
	int i=0;
	while(i<5)
	{
		if(fork()==0)
		{
			philosopher(order[i]);
		}
		else
		{
			i++;
		}
		
	}
	exit();
	return 0;
}
