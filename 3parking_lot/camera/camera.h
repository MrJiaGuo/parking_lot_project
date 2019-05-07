#ifndef __CAMERA_H
#define __CAMERA_H


int camera_doing();

int yuyv2rgb0(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height);

int yuyv2rgb(int y, int u, int v);

#endif