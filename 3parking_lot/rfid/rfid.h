#ifndef __RFID_H
#define __RFID_H

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#define UARTPATH "/dev/ttySAC1"
//卡id
extern unsigned int cardid;
//初始化串口
int uart_init(int);
//打开串口
int uart_open();

//发送A命令
int sendA(int);
//发送B命令
int sendB(int);
//检验和
char getbcc(char *,int);


#endif