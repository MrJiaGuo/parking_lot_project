#ifndef __DATABASE_H
#define __DATABSE_H

int cal_db(void * arg , int ncolumn , char **col_val ,  char ** col_name );

char * get_time();
int count_pay(int cardid,char * out_time);
int database_clear();
int database_init();
int database_close();
int show_info();
int car_park(int cardid);
int car_leave(int cardid);

#endif