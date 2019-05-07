#include <stdio.h>
#include <linux/input.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>
#include <time.h>
#include <linux/input.h>
#include "rfid/rfid.h"
#include "sqlite3.h"
#include "database/database.h"
#include "camera/camera.h"
#include "touch/touch.h"
#include "jpeg/jpeg.h"

extern int flag_camera;

//触控屏线程
pthread_t tid;

//摄像头线程
pthread_t tid1;
//c触摸屏坐标值
int ts_x,ts_y;
//摄像头模式选择
int mode,camera_t;

//触摸屏控制函数
void *ctl_camera(void *arg)
{
	while(1)
	{
		//等待触控
		ts_xy_get(&ts_x,&ts_y);
		//printf("x:%d   y:%d\n",ts_x,ts_y);
		//开启摄像头模式
		if(ts_x>10&&ts_x<170&&ts_y>20&&ts_y<80)
		{
			mode = 1;
			camera_t=0;
		}
		//关闭摄像头
		else if(ts_x>10&&ts_x<170&&ts_y>120&&ts_y<180)
		{
			mode = 2;
			camera_t=1;
			flag_camera = 1;
		}
		else if(ts_x>10&&ts_x<170&&ts_y>220&&ts_y<280)
		{
			show_info();
		}
		//printf("mode:%d   camera_t:%d\n",mode,camera_t);
			
	}
	
	pthread_exit(NULL);
}
//摄像头工作函数
void * camera_do(void *arg)
{
	while(1)
	{
		camera_doing();
	}
	
	pthread_exit(NULL);
}

int main()
{
	
	int res;
	//LCD驱动打开
	lcd_open();
	//触控屏驱动打开
	ts_open();
	//触控屏线程
	res = pthread_create(&tid,NULL,ctl_camera,NULL);
	if(res==-1)
	{
		perror("create ts failed!");
	}
	//摄像头线程
	res = pthread_create(&tid1,NULL,camera_do,NULL);
	if(res==-1)
	{
		perror("create camera failed!");
	}
	//设置线程分离
	pthread_detach(tid);
	pthread_detach(tid1);
	//显示主界面
	showjpg(0,0,"./parking.jpg");
	system("madplay ./welcome.mp3");
	sleep(2);
	database_clear();
	while(1)
	{
		database_init();
		sleep(1);
	}
	//关闭相关资源
	database_close();
	ts_close();
	return 0;
}