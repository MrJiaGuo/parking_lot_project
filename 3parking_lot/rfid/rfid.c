#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include "rfid.h"
#include "../database/database.h"
#include "../camera/camera.h"
#include "../touch/touch.h"
//rfid号
unsigned int  cardid ;
//串口fd
int fd;

static struct timeval timeout;
//宏定义串口
#define DEV_PATH   "/dev/ttySAC1"

int uart_init(int fd)
{
	  
	//声明设置串口的结构体
	struct termios termios_new;
	//先清空该结构体
	bzero( &termios_new, sizeof(termios_new));
	//	cfmakeraw()设置终端属性，就是设置termios结构中的各个参数。
	cfmakeraw(&termios_new);
	//设置波特率
	//termios_new.c_cflag=(B9600);
	cfsetispeed(&termios_new, B9600);
	cfsetospeed(&termios_new, B9600);
	//CLOCAL和CREAD分别用于本地连接和接受使能，因此，首先要通过位掩码的方式激活这两个选项。    
	termios_new.c_cflag |= CLOCAL | CREAD;
	//通过掩码设置数据位为8位
	termios_new.c_cflag &= ~CSIZE;
	termios_new.c_cflag |= CS8; 
	//设置无奇偶校验
	termios_new.c_cflag &= ~PARENB;
	//一位停止位
	termios_new.c_cflag &= ~CSTOPB;
	tcflush(fd,TCIFLUSH);
	// 可设置接收字符和等待时间，无特殊要求可以将其设置为0
	termios_new.c_cc[VTIME] = 10;
	termios_new.c_cc[VMIN] = 1;
	// 用于清空输入/输出缓冲区
	tcflush (fd, TCIFLUSH);
	//完成配置后，可以使用以下函数激活串口设置
	if(tcsetattr(fd,TCSANOW,&termios_new) )
		printf("Setting the serial1 failed!\n");
	
	
	
}

//检验和
char getbcc(char *buf,int n)
{
	int i;
	unsigned char bcc=0;
	for(i = 0; i < n; i++)
	{
		bcc ^= *(buf+i);
	}
	return (~bcc);
}

//发送A命令
int sendA(int fd)
{
	unsigned char WBuf[128], RBuf[128];
	int  ret, i;
	fd_set rdfd;
	
	memset(WBuf, 0, 128);  // 清空
	memset(RBuf,1,128);    // 全1
	WBuf[0] = 0x07;	//帧长= 7 Byte
	WBuf[1] = 0x02;	//包号= 0 , 命令类型= 0x01
	WBuf[2] = 0x41;	//命令= 'A'
	WBuf[3] = 0x01;	//信息长度= 1
	WBuf[4] = 0x52;	//请求模式:  ALL=0x52
	WBuf[5] = getbcc(WBuf, WBuf[0]-2);		//校验和
	WBuf[6] = 0x03; 	//结束标志


	FD_ZERO(&rdfd);
	FD_SET(fd,&rdfd);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	
	write(fd, WBuf, 7);
	//  printf("sendA\n");
	ret = select(fd + 1,&rdfd, NULL,NULL,&timeout);// select什么情况返回-1 ？返回0？返回其它值？
	switch(ret)
	{
		case -1:  // select 出错返回-1
			perror("select error\n");
			break;
		case  0:  //select 在指定时间到了还没有收到对方发来的数据，返回0
		//	printf("Request timed out.\n");
			break;
		default: // 正常， 数据可读
			ret = read(fd, RBuf, 8); //S50卡只返回8个字节
			if (ret < 0) // 读失败
			{
		//		printf("ret = %d, 0x%x\n", ret, errno);
				break;
			}
			if (RBuf[2] == 0x00)	 	//应答帧状态部分为0 则请求成功  读卡器说我很好
			{
				// 卡的类型
				//printf("GOT a S50 card!! \n");
				return 0;
			}
			break;
	}
	return -1;
	
	
}
//发送B命令
int sendB(int fd)
{
	unsigned char WBuf[128], RBuf[128];
	int ret, i;
	fd_set rdfd;;
	memset(WBuf, 0, 128);
	memset(RBuf,0,128);
	WBuf[0] = 0x08;	//帧长= 8 Byte
	WBuf[1] = 0x02;	//包号= 0 , 命令类型= 0x01
	WBuf[2] = 0x42;	//命令= 'B'
	WBuf[3] = 0x02;	//信息长度= 2
	WBuf[4] = 0x93;	//防碰撞0x93 --一级防碰撞
	WBuf[5] = 0x00;	//位计数0
	WBuf[6] = getbcc(WBuf, WBuf[0]-2);		//校验和
	WBuf[7] = 0x03; 	//结束标志
	
	FD_ZERO(&rdfd);
	FD_SET(fd,&rdfd);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	
	write(fd, WBuf, 8);
	ret = select(fd + 1,&rdfd, NULL,NULL,&timeout);
	switch(ret)
	{
		case -1:
			perror("select error\n");
			break;
		case  0:
		//	perror("Timeout:");
			break;
		default:
			ret = read(fd, RBuf, 10);
			if (ret < 0)
			{
		//		printf("ret = %d, 0x%x\n", ret, errno);
				break;
			}
			
		//	for(i=0; i<10 ;i++)
		//		printf("[%d] %x \n", i,RBuf[i]);
			
			if (RBuf[2] == 0x00) //应答帧状态部分为0 则获取ID 成功
			{
				// 卡号
				cardid = (RBuf[7]<<24) | (RBuf[6]<<16) | (RBuf[5]<<8) | RBuf[4];//保存得到的卡ID
				//printf("GOT cardid = %x !! \n", cardid);
				return 0;
			}
	}
	return -1;
	
}


int uart_open()
{
	int ret, i;

	fd = open(DEV_PATH, O_RDWR | O_NOCTTY | O_NONBLOCK);//以非阻塞
	if (fd < 0)
	{
		fprintf(stderr, "Open Gec210_ttySAC1 fail!\n");
		return -1;
	}
	/*初始化串口*/
	uart_init(fd);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	printf("wait find card!\n");
	while(1) //多次请求
	{
		
		//发送寻卡命令'A'给串口设备文件，判断其应答
		if ( sendA(fd)==-1 ) // 寻卡失败
		{
			// printf("The request failed!\n");
			tcflush (fd, TCIFLUSH);
			usleep(200000);
			continue; // 没有卡回应，说明不需要读卡ID，则可以继续去发送A命令寻卡
		}

			
		// 'A'寻卡命令成功，会发送‘B'防冲撞命令得到卡ID
		if( sendB(fd)==-1 )
		{
			printf("Couldn't get card-id!\n");
			tcflush (fd, TCIFLUSH);
			continue;
		}
		else
			printf("card ID = %x\n", cardid);
		
		if( cardid== 0)
			continue ;
		else
		{
			close(fd);
			return 0;
		}
	}
	
}
