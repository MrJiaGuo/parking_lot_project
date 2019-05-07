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
#include "camera.h"
#include "../jpeg/jpeg.h"

extern int mode;
extern int camera_t;
extern unsigned int *mem_p;
extern int save_id;
extern int fd;

int camerafd;

unsigned char* rgb;

int flag_camera = 0;

struct usrbuf
{
	void *start;
	int length;
};

int yuyv2rgb(int y, int u, int v)
{
     unsigned int pixel24 = 0;
     unsigned char *pixel = (unsigned char *)&pixel24;
     int r, g, b;
     static int  ruv, guv, buv;

	 // 色度
     ruv = 1596*(v-128);
	 guv = 391*(u-128) + 813*(v-128);
     buv = 2018*(u-128);
     
	// RGB
     r = (1164*(y-16) + ruv) / 1000;
     g = (1164*(y-16) - guv) / 1000;
     b = (1164*(y-16) + buv) / 1000;

     if(r > 255) r = 255;
     if(g > 255) g = 255;
     if(b > 255) b = 255;
     if(r < 0) r = 0;
     if(g < 0) g = 0;
     if(b < 0) b = 0;

     pixel[0] = r;
     pixel[1] = g;
     pixel[2] = b;

     return pixel24;
}


int yuyv2rgb0(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height)
{
     unsigned int in, out;
     int y0, u, y1, v;
     unsigned int pixel24;
     unsigned char *pixel = (unsigned char *)&pixel24;
     unsigned int size = width*height*2;

     for(in = 0, out = 0; in < size; in += 4, out += 6)
     {
	// YUYV
          y0 = yuv[in+0];
          u  = yuv[in+1];
          y1 = yuv[in+2];
          v  = yuv[in+3];

          pixel24 = yuyv2rgb(y0, u, v); // RGB
          rgb[out+0] = pixel[0];    
          rgb[out+1] = pixel[1];
          rgb[out+2] = pixel[2];

          pixel24 = yuyv2rgb(y1, u, v);// RGB
          rgb[out+3] = pixel[0];
          rgb[out+4] = pixel[1];
          rgb[out+5] = pixel[2];

     }
     return 0;
}



int camera_doing()
{
	
	int ret;
	int i;
	int x,y;

	
	//打开摄像头
	camerafd=open("/dev/video7",O_RDWR);
	if(camerafd==-1)
	{
		perror("open camera failed!\n");
		return -1;
	}
	
	//设置采集格式
	struct v4l2_format format;
	memset(&format, 0, sizeof format);
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = 320;
	format.fmt.pix.height = 240;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	format.fmt.pix.field = V4L2_FIELD_NONE;
	ret=ioctl(camerafd,VIDIOC_S_FMT, &format);
	if(ret==-1)
	{
		perror("设置采集格式失败!\n");
		return -1;
	}
	//申请缓存
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if(ioctl(camerafd,VIDIOC_REQBUFS, &req)== -1)
	{
		perror("申请缓存失败!\n");
		return -1;
	}
	
    //分配你刚才申请的缓冲块 --- 队列中设置了4个缓冲区 pbuf[i],
	struct usrbuf *pbuf=calloc(4,sizeof(struct usrbuf));
    for(i = 0; i < 4; i++) 
    {
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (ioctl(camerafd,VIDIOC_QUERYBUF,&buf) == -1)
		{
			perror("分配缓存失败!\n");
			return -1;
		}
		// 四个缓冲区是直接映射camerafd数据的
		pbuf[i].start=mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
			   camerafd, buf.m.offset);
		pbuf[i].length=buf.length;
   	 }

	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while(1)
	{
		
		if(mode==1)
		{
			
			//入队VIDIOC_QBUF
			for (i = 0; i < 4; i++) 
			{
				struct v4l2_buffer buf;
				memset(&buf, 0, sizeof buf);
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				buf.index = i;
				if (ioctl(camerafd,VIDIOC_QBUF,&buf) == -1)
				{
					perror("入队失败!\n");
					return -1;
				}
			}
			
			
			//开始采集VIDIOC_STREAMON
			
			if(ioctl(camerafd, VIDIOC_STREAMON, &type) == -1)
			{
				perror("开始采集失败!\n");
				return -1;
			}
			
			//rgb = malloc(640*480*3);
			rgb = malloc(320*240*3);
			struct timeval timeout;
			timeout.tv_sec = 3;
			timeout.tv_usec = 0;


			while(!camera_t)
			{
				fd_set fds;
				FD_ZERO(&fds);
				FD_SET(camerafd, &fds);
				struct v4l2_buffer buf;
				int r = select(camerafd + 1, &fds, 0, 0, &timeout); // 只要这个摄像头文件有动静就会结束阻塞 
				for(i=0; i<4; i++)
				{
					
					memset(&buf, 0, sizeof buf);
					buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
					buf.memory = V4L2_MEMORY_MMAP;
					buf.index=i;
					if (ioctl(camerafd, VIDIOC_DQBUF, &buf) == -1)// 读不出数据出错了 VIDIOC_DQBUF
					{
						perror("出队失败!\n");
						return -1;
					}		
					if(ioctl(camerafd, VIDIOC_QBUF, &buf) == -1)//  写不进数据
					{
						perror("入队失败!\n");
						return -1;
					}
					
					// 正常数据
					// 把V4L2输出的YUYV格式数据转换成RGB
					yuyv2rgb0(pbuf[i].start,rgb,320,240);
					
					// 把RGB888输出到屏幕上ARGB8888
					for(y=0; y<240; y++)
						for(x=0; x<320; x++)
							*(mem_p + 480 +y*800 + x ) = rgb[(y * 320 + x) * 3 + 0]<<16 | rgb[(y * 320 + x) * 3 + 1]<<8 | rgb[(y * 320 + x) * 3 + 2];
				}
			}
		
		}
		
		else if(mode==2&&camera_t==1)
		{
			if(ioctl(camerafd, VIDIOC_STREAMOFF, &type) == -1)
			{
				perror("停止采集失败!\n");
				return -1;
			}
			
			camera_t=0;
			mode = 0;
			//车入库拍照
			if(flag_camera==0)
			{
				//保存文件
				char pic_buf[10];
				bzero(pic_buf,sizeof(pic_buf));
				sprintf(pic_buf,"%u.jpg",save_id);
				savejpg(rgb,pic_buf, 320,240);
				//重新开始拍照
				sleep(1);
				mode = 1;
				camera_t=0;
				free(rgb);
			}
			//按下关闭摄像头时为1，赋值为0能够让拍照进行
			flag_camera=0;
		}
	}


	
	for(i=0; i<4; i++)
		munmap(pbuf[i].start , sizeof(struct v4l2_buffer) );
	free(pbuf);	
	close (camerafd);

	munmap (mem_p, 800*480*4);

}
