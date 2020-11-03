#include "lib.h"
#include "types.h"

void reader(int id,sem_t *CountMutex,sem_t *WriteMutex)
{
	while(1)
	{
		int Rcount;
		sem_wait(CountMutex);
		sleep(128);
		read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		//printf("1count:%d\n",Rcount);
		if (Rcount == 0)
		{
			sem_wait(WriteMutex);
		}
		Rcount++;
		write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		
		//printf("2count:%d\n",Rcount);
		sem_post(CountMutex);
		sleep(128);
	
		read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		printf("Reader %d: read, total %d reader\n", id, Rcount);
		//sleep(128);
		//read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		//printf("Reader %d: read, total %d reader, read finish\n", id, Rcount);

		
		sem_wait(CountMutex);
		sleep(128);
		read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		//printf("3count:%d\n",Rcount);
		Rcount--;
		write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
		printf("Reader %d: finish reading\n", id);
		//printf("4count:%d\n",Rcount);
		if (Rcount == 0)
		{
			sem_post(WriteMutex);
		}
		sem_post(CountMutex);
		sleep(128);
	}
	
}

void writer(int id,sem_t *write_mutex)
{
	while(1)
	{
		sem_wait(write_mutex);
		printf("Writer %d: write\n", id);
		sleep(128);
		sem_post(write_mutex);
		sleep(128);
	}
	
}

int main(void) {
	// TODO in lab4-ing
	printf("reader_writer\n");
	sem_t write_mutex,count_mutex;
	sem_init(&write_mutex,1);
	sem_init(&count_mutex,1);
	int rcount=0;
	write(SH_MEM, (uint8_t *)&rcount, 4, 0);
	int i=0;
	while(i<3)
	{
		if(fork()==0)
			writer(i,&write_mutex);
		else
		{
			i++;
		}
		
	}
	i=0;
	while(i<3)
	{
		if(fork()==0)
			reader(i,&count_mutex,&write_mutex);
		else
		{
			i++;
		}
		
	}
	exit();
	return 0;
}
