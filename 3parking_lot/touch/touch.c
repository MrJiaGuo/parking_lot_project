#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <time.h>
#include <termios.h>//串口有关的头文件
#include <poll.h>
#include <pthread.h>//线程有关的头文件
#include <linux/videodev2.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include "../rfid/rfid.h"
#include "../database/database.h"
#include "touch.h"

//触摸屏触控事件
struct input_event g_ts_event;

//映射内存
unsigned int *mem_p;

int fd_lcd,fd_ts;//文件描述符

//加载lcd驱动
int lcd_open()
{
	fd_lcd= open("/dev/fb0",O_RDWR);
    if(fd_lcd==-1){
        perror("open lcd");
        return -1;
    }
	
	mem_p = mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,fd_lcd,0);

    if(mem_p==MAP_FAILED){
        perror("mmap");
        return -1;
    }
	
}

//触摸事件驱动
int ts_open()
{
	fd_ts = open("/dev/input/event0",O_RDONLY);
	if(fd_ts == -1)
	{
		perror(" Open ts");
		return -1;
	}
}


//获取触摸屏坐标
int	ts_xy_get(int *x,int *y)
{
	static int ts_get_xy_count=0;
	
	int count;
	
	while(1)
	{
		/* 调用read函数,获取触摸屏输入事件报告 */	
		count = read(fd_ts,&g_ts_event,sizeof(struct input_event));
		
		/* 检查当前读取的事件报告是否读取完整 */
		if(count != sizeof(struct input_event))
		{
			perror("read error");
			return -1;
		}	

		/* 检查当前响应事件是否坐标值事件 */
		if(EV_ABS == g_ts_event.type)
		{
			/* x坐标 */
			if(g_ts_event.code == ABS_X)
			{
				ts_get_xy_count ++;
				
				*x = g_ts_event.value;
				
			}
			
			/* y坐标 */
 			if(g_ts_event.code == ABS_Y)
			{
				ts_get_xy_count ++;
				
				*y = g_ts_event.value;	
							
			}	
			
			if(ts_get_xy_count == 2)
			{
				ts_get_xy_count = 0;
				break;
			}
		}
	}
	return 0;
}


//关闭文件
int ts_close()
{
	close(fd_lcd);
	close(fd_ts);
}

