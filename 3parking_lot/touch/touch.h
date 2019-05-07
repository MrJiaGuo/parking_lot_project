#ifndef __TOUCH_H
#define __TOUCH_H

int lcd_open();
int ts_open();
int ts_close();
int	ts_xy_get(int *x,int *y);
#endif