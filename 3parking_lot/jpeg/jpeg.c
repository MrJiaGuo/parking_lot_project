#include <stdio.h>
#include "../jpeglib.h"
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#define JPEG_QUALITY 100 //图片质量

//处理jpg图片
extern unsigned int *mem_p;
int jpg_flag;


int savejpg(char *pdata, char *jpg_file, int width, int height)
{ 
	//分别为RGB数据，要保存的jpg文件名，图片长宽
	int depth = 3;
	JSAMPROW row_pointer[1];//指向一行图像数据的指针
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
    FILE *outfile;

	cinfo.err = jpeg_std_error(&jerr);//要首先初始化错误信息
	//* Now we can initialize the JPEG compression object.
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(jpg_file, "wb")) == NULL)
	{
		fprintf(stderr, "can't open %s\n", jpg_file);
		return -1;
	}
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = width;             //* image width and height, in pixels
	cinfo.image_height = height;
	cinfo.input_components = depth;    //* # of color components per pixel
	cinfo.in_color_space = JCS_RGB;     //* colorspace of input image
	jpeg_set_defaults(&cinfo);

	jpeg_set_quality(&cinfo, JPEG_QUALITY, TRUE ); //* limit to baseline-JPEG values
	jpeg_start_compress(&cinfo, TRUE);

	int row_stride = width * 3;
	while (cinfo.next_scanline < cinfo.image_height)
	{
			row_pointer[0] = (JSAMPROW)(pdata + cinfo.next_scanline * row_stride);//一行一行数据的传，jpeg为大端数据格式
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);//这几个函数都是固定流程
	fclose(outfile);

	return 0;
}

int showjpg(int x,int y,char *jpgpath)
{
	int i;
	int j;

	//定义解压缩结构体变量和错误处理变量并初始化
	struct jpeg_decompress_struct mydecom;
	struct jpeg_error_mgr myerr;
	mydecom.err=jpeg_std_error(&myerr);
	jpeg_create_decompress(&mydecom);
	
	//打开你要显示的jpg图片
	FILE *jpgp=fopen(jpgpath,"r+");
	if(jpgp==NULL)
	{
		perror("打开jpg失败!\n");
		return -1;
	}

	//获取数据源-->你要显示那张jpg
	jpeg_stdio_src(&mydecom,jpgp); //jpeg_mem_src()
	
	//读取jpg的头信息
	jpeg_read_header(&mydecom,1);
	
	//开始解压缩
	jpeg_start_decompress(&mydecom);
	//printf("目前显示的jpg宽是:%d  高是:%d\n",mydecom.image_width,mydecom.image_height);
	//判断x，y是否超标
	if(x>(800-mydecom.image_width)&&y>(480-mydecom.image_height))
	{
		printf("对不起，坐标超标了!\n");
		return -1;
	}

	//先读取一行RGB数据
	char *buf=malloc(mydecom.image_width*3);
	int otherbuf[mydecom.image_width];
	for(i=0; i<mydecom.image_height; i++)
	{
		jpeg_read_scanlines(&mydecom,(JSAMPARRAY)&buf,1);
		//将3字节的RGB转换4字节的ARGB   buf[0][1][2]  [3][4][5]
		for(j=0; j<mydecom.image_width;j++)
		{
			otherbuf[j]=0x00<<24|buf[3*j]<<16|buf[3*j+1]<<8|buf[3*j+2];
		}
		memcpy(mem_p+800*(y+i)+x,otherbuf,mydecom.image_width*4);
	}
	
	//收尾
	jpeg_finish_decompress(&mydecom);
    jpeg_destroy_decompress(&mydecom);
	
	free(buf);
	fclose(jpgp);
	return 0;
	
}


