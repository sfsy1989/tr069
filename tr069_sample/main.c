#include"tr069.h"
#include <pthread.h>


void nativeHttpdTask() {
	system("fv_httpd -auth_realm F640 -ports 8080 -root /data/iptv/tr069 &");
}

void  start_tr069_task()
{
	tr069_task();
}


int main()
{
	pthread_t config_id,http_id,tr069_id;
	int ret;
	ret=pthread_create(&http_id,NULL,(void *) nativeHttpdTask,NULL);
	if(ret!=0){
		printf("Create nativeHttpdTask pthread error!n");
		exit (-1);
	}
	ret=pthread_create(&tr069_id,NULL,(void *) start_tr069_task,NULL);
	if(ret!=0){
		printf("Create tr069_task pthread error!\n");
		exit (-1);
	}

	pthread_join(http_id,NULL);
	pthread_join(tr069_id,NULL);
	while(1);
}
