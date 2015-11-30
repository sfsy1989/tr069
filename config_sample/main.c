#include"cfg.h"
#include <pthread.h>


void start_config_task()
{
	config_task();	
}


int main()
{
	pthread_t config_id;
	int ret;
	ret=pthread_create(&config_id,NULL,(void *) start_config_task,NULL);
	if(ret!=0){
		printf ("Create config_task pthread error!n");
		exit (-1);
	}
	pthread_join(config_id,NULL);
	while(1);
}
