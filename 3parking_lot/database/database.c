#include <stdio.h>
#include <linux/input.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <time.h>
#include "../rfid/rfid.h"
#include "sqlite3.h"
#include "database.h"
#include "../camera/camera.h"
#include "../jpeg/jpeg.h"

extern unsigned int cardid;
extern int mode;
extern int camera_t;
extern int flag_camera;
//保存rfid卡号
int save_id;
//数据库
sqlite3 * db ; 
//停车位置
char car_buf[4]={0};
//记录停车位置和rfid号
char car_memory[4][15]={0};

//标志位
int found = 0;
int cnt = 0;
//保存入场时间
char save_time[50];
//离开时间
char leave_time[50];
//进场时间
char in_time[50];

extern int camerafd;

//static int cnt = 0;

//获取系统时间
char * get_time()
{
	time_t mytime;
	time(&mytime);  
	if(mytime==-1)
	{
		perror("get_time failed:");
		exit(-1);
	}
	
	printf("当前系统时间是:%s\n",ctime(&mytime));
	struct tm *ptime=gmtime(&mytime);
	sprintf(save_time,"%d-%d-%d %d:%d:%d",ptime->tm_year+1900,ptime->tm_mon+1,ptime->tm_mday,ptime->tm_hour,ptime->tm_min,ptime->tm_sec);
	
	strcpy(leave_time,save_time);
	//printf("time:%s\n",save_time);
	//printf("time:%s\n",leave_time);

}


//车入库判断，位置不满则在相应位置显示车子
int car_park(int cardid)
{

	int i;
	int flag=0;
	for(i=0;i<4;i++)
	{
		if(car_buf[i]==0)
		{
			flag = 1;
			car_buf[i]=1;
			sprintf(car_memory[i],"%u:%d",cardid,i);
			//printf("car:%s\n",car_memory[i]);
			if(i==0)
				showjpg(55,315,"./car.jpg");
			else if(i==1)
				showjpg(214,315,"./car.jpg");
			else if(i==2)
				showjpg(420,315,"./car.jpg");
			else
				showjpg(615,315,"./car.jpg");
			break;
		}
	}
	
	if(flag==0)
	{
		printf("车库已经停满，暂时没有空位置\n");
		system("madplay ./full.mp3");
		return 1;
	}
	return 0;
}

//车离开判断
int car_leave(int cardid)
{
	int i;
	int j;
	//记录卡号
	char car_id[15];
	//暂存信息
	char temp[15];
	//把rfid转换成字符
	char p[15];
	for(i=0;i<4;i++)
	{
		bzero(car_id,15);
		bzero(temp,15);
		bzero(p,15);
		strcpy(temp,car_memory[i]);
		if(strlen(car_memory[i])!=0)
		{
			//printf("strlen:%d\n",strlen(car_memory[i]));
			strcpy(car_id , strtok(temp,":"));
			j=atoi(strtok(NULL,":"));
			//printf("id:%s  i:%d\n",car_id,i);
		}
		//把rfid转换成字符
		sprintf(p,"%u",cardid);
		
		if(strcmp(car_id,p)==0)
		{
			//位置清零
			car_buf[j]=0;
			bzero(car_memory[j],15);
			
			if(j==0)
				showjpg(55,315,"./nocar.jpg");
			else if(j==1)
				showjpg(214,315,"./nocar.jpg");
			else if(j==2)
				showjpg(420,315,"./nocar.jpg");
			else
				showjpg(615,315,"./nocar.jpg");
			break;
		}
	}
	
	//printf("chuku ok\n");
}

//回调函数计算离开出库时间
int cal_db(void * arg , int ncolumn , char **col_val ,  char ** col_name )
{	

	//第五次显示时间
	int i ;
	found = 1;

	//printf("cardid:%x\n" ,col_val[0]);
	
	strcpy(in_time,col_val[4]);
	printf("current_time = %s\n\n" ,col_val[4]);

	return 0;
}

//回调函数，显示车库当前信息  列                 列值              列名  （有多少行回掉多少次？）
int show_db(void * arg , int ncolumn , char **col_val ,  char ** col_name )
{	
	int i;	

	cnt++;
	if(cnt==1)
	{

		for(i=0 ; i<ncolumn; i++ )
		{
			printf("%-15s" , col_name[i])	;
		}
		printf("\n");
		for(i=0 ; i<ncolumn; i++ )
		{
			printf("%-10s     " , col_val[i])	;
		}
		printf("\n");
	}
	else
	{
		for(i=0 ; i<ncolumn; i++ )
		{
			printf("%-10s     " , col_val[i] )	;
		}
		printf("\n");
	}
	return 0;

}

//调用show_db函数
int show_info()
{
	
	int res;
	res = sqlite3_exec ( db,"select *from  carinfo;", show_db , NULL,  NULL);
	if(res!=SQLITE_OK)
	{
		perror("show_info  failed:");
		return 0;
	}
	cnt = 0;
}

//计算停车费用函数，按分钟收费
int count_pay(int cardid,char * out_time)
{
	//收费变量
	float pay=0;
	//进库出库时间变量
	char year_in[5],year_leav[5];
	char mon_in[5],mon_leav[5];
	int day_in,day_leav;
	int hour_in,hour_leav;
	int min_in,min_leav;
	int sec_in,sec_leav;
	//分割进库时间
	strcpy(year_in,strtok(in_time,"- :"));
	strcpy(mon_in,strtok(NULL,"- :"));
	day_in = atoi(strtok(NULL,"- :"));
	hour_in = atoi(strtok(NULL,"- :"));
	min_in = atoi(strtok(NULL,"- :"));
	sec_in = atoi(strtok(NULL,"- :"));
	
	//printf("in_time:%d %d %d %d\n",day_in,hour_in,min_in,sec_in);
	//分割出库时间
	strcpy(year_leav,strtok(out_time,"- :"));
	strcpy(mon_leav,strtok(NULL,"- :"));
	day_leav = atoi(strtok(NULL,"- :"));
	hour_leav = atoi(strtok(NULL,"- :"));
	min_leav = atoi(strtok(NULL,"- :"));
	sec_leav = atoi(strtok(NULL,"- :"));
	
	//printf("out_time:%d %d %d %d\n\n",day_leav,hour_leav,min_leav,sec_leav);
	
	//一分钟2毛钱计费
	if(min_leav>=min_in)
		pay = ((hour_leav-hour_in)*60+(min_leav-min_in))*0.2;
	else if(min_leav<min_in&&(hour_leav-hour_in>1))
		pay = ((hour_leav-hour_in)*60+(60-min_in+min_leav))*0.2;
	else
		pay = (60-min_in+min_leav)*0.2;
	printf("should pay : %f￥\n\n",pay);
}

//初始化数据库
int database_clear()
{
	int ret;
	//打开数据库文件
	ret = sqlite3_open("./cardata", &db);
	if(ret)
	{
		printf("open DB failed , ret = %d \n", ret);
		return 0;
	}
	
	
	//创建表
	ret =  sqlite3_exec ( db, "create  table if not exists carinfo(cardid int ,car_num nchar,car_master nchar,master_tel nchar,car_intime nchar);" , NULL, NULL,  NULL);
	if(ret!=SQLITE_OK)
	{
		printf("create DB failed , ret = %d \n", ret);
		return 0;
	}
	//清空记录数据库记录
	ret = sqlite3_exec ( db, "delete from carinfo;", NULL, NULL,  NULL);
	if(ret!=SQLITE_OK)
	{
		printf("delete DB faie", ret);
		return 0;
	}
	
}

//进库出库数据库管理函数
int database_init()
{
	
	
	int ret ;
	
	//车主基本信息（主人，车牌，进场时间，电话）
	char car_master[20];
	char car_num[20];
	char car_intime[50];
	char master_tel[15];
	//打开串口
	uart_open();
	
	//如果摄像头没有打开返回主函数
	if(mode!=1)
	{
		printf("请先打开摄像头！\n");
		system("madplay ./camera.mp3");
		usleep(1000);
		return 0;
	}
	
	//记录刷卡的id
	char rfid_buf[15]={0}; 
	sprintf(rfid_buf,"select *from  carinfo where cardid=%u;",cardid);
	//查询是否存在该卡
	ret = sqlite3_exec ( db,rfid_buf, cal_db , NULL,  NULL);
	if(ret!=SQLITE_OK)
	{
		perror("select 1 failed:");
		return 0;
	}
	
	//找不到该车卡，插入操作
	if(found==0)
	{
		//保存卡号以来拍照
		save_id = cardid;
		//输入相关信息
		printf("please enter car info:\n");
		printf("请输入车牌号：");
		scanf("%s",car_num);
		printf("请输入车主姓名：");
		scanf("%s",car_master);
		printf("请输入车主电话：");
		scanf("%s",master_tel);
		
		//获取进库时间
		get_time();
		
		//拼接信息
		char buf[1024];
		bzero(buf,1024);
		sprintf(buf,"insert  into carinfo values(%u,\"%s\",\"%s\",\"%s\",\"%s\")",cardid,car_num,car_master,master_tel,save_time);
		
		
		//车入库
		ret = car_park(cardid);
		if(ret == 1)
		{
			printf("not add carinfo!\n");
			return 0;
		}
		
		//插入操作
		ret =  sqlite3_exec ( db, buf , NULL, NULL,  NULL);
		if(ret)
		{
			printf("insert DB failed again, ret = %d \n", ret);
			return 0;
		}
		//停止拍照
		mode = 2;
		camera_t=1;
		system("madplay ./park.mp3");
		
		
		
	}
	//存在则计算离开时间，计算收费
	else
	{
		char leav_buf[50];
		bzero(leav_buf,sizeof(leav_buf));
		sprintf(leav_buf,"delete from carinfo where cardid =%u;",cardid);
		//计算时间
		get_time();
		count_pay(cardid,leave_time);
		
		//删除记录
		printf("delete carid:%u\n",cardid);
		//车位置清零
		car_leave(cardid);
		//显示进场时拍的照片,先暂停拍摄显示1s后恢复
		char pic_buf[10];
		bzero(pic_buf,sizeof(pic_buf));
		sprintf(pic_buf,"%u.jpg",cardid);
		//暂停拍照
		flag_camera = 1;
		mode=2;
		camera_t=1;
		sleep(1);
		showjpg(480,0,pic_buf);	
		sleep(2);
		//恢复拍照
		mode=1;
		camera_t=0;
		

		//车出库删除后删除数据信息
		sqlite3_exec ( db, leav_buf , NULL, NULL,  NULL);
		if(ret!=SQLITE_OK)
		{
			printf("delete DB faie", ret);
			return 0;
		}
		
		found = 0;
		system("madplay ./leave.mp3");
	}
	


}


//数据库关闭函数
int database_close()
{
	int ret;
	// 释放
	ret = sqlite3_close(db);
	if(ret)
	{
		printf("close DB failed , ret = %d \n", ret);
		return 0;
	}
}
