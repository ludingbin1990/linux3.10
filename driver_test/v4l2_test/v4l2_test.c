/*
 * main.c
 *
 *  Created on: Apr 25, 2016
 *      Author: anzyelay
 */
#include <unistd.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
 
#include <asm/types.h>
#include <linux/videodev2.h>
 
// 2440 video 2 used for preview rgb image
#define CAMERA_DEVICE "/dev/video1"
#define FB_DEVICE_NAME "/dev/fb0"  
 #define RGB_BMP_STORE_LOCATION "/tmp/picture"
#define VIDEO_WIDTH 240
#define VIDEO_HEIGHT 320
#define BUFFER_COUNT 8

typedef struct VideoBuffer {
    void   *start;
    size_t  length;
} VideoBuffer;
 
VideoBuffer VideoCapturebuf[BUFFER_COUNT];

#define BI_BITFIELDS 0x3
#define VIDEO_RGB_BMP_TEST 1
#define VIDEO_LCD_SHOW_TEST 1
typedef char BYTE;
typedef short WORD;
typedef int DWORD;
typedef int LONG;
 
typedef struct tagBITMAPFILEHEADER{
	WORD bfType;//位图文件的类型，在Windows中，此字段的值总为‘BM’(1-2字节）
	DWORD bfSize;//位图文件的大小，以字节为单位（3-6字节，低位在前）
	WORD bfReserved1;//位图文件保留字，必须为0(7-8字节）
	WORD bfReserved2;//位图文件保留字，必须为0(9-10字节）
	DWORD bfOffBits;//位图数据的起始位置，以相对于位图（11-14字节，低位在前）
	//文件头的偏移量表示，以字节为单位
}__attribute__((packed)) BitMapFileHeader;	//BITMAPFILEHEADER;
 
typedef struct tagBITMAPINFOHEADER{
	DWORD biSize;//本结构所占用字节数（15-18字节）
	LONG biWidth;//位图的宽度，以像素为单位（19-22字节）
	LONG biHeight;//位图的高度，以像素为单位（23-26字节）
	WORD biPlanes;//目标设备的级别，必须为1(27-28字节）
	WORD biBitCount;//每个像素所需的位数，必须是1（双色），（29-30字节）
	//4(16色），8(256色）16(高彩色)或24（真彩色）之一
	DWORD biCompression;//位图压缩类型，必须是0（不压缩），（31-34字节）
	//  1(BI_RLE8压缩类型）或2(BI_RLE4压缩类型）之一
	DWORD biSizeImage;//位图的大小(其中包含了为了补齐行数是4的倍数而添加的空字节)，以字节为单位（35-38字节）
	LONG biXPelsPerMeter;//位图水平分辨率，像素数（39-42字节）
	LONG biYPelsPerMeter;//位图垂直分辨率，像素数（43-46字节)
	DWORD biClrUsed;//位图实际使用的颜色表中的颜色数（47-50字节）
	DWORD biClrImportant;//位图显示过程中重要的颜色数（51-54字节）
}__attribute__((packed)) BitMapInfoHeader;	//BITMAPINFOHEADER;
 
 
typedef struct tagRGBQUAD{
	BYTE rgbBlue;//蓝色的亮度（值范围为0-255)
	BYTE rgbGreen;//绿色的亮度（值范围为0-255)
	BYTE rgbRed;//红色的亮度（值范围为0-255)
	BYTE rgbReserved;//保留，必须为0
}__attribute__((packed)) RgbQuad;	//RGBQUAD;
 
 
 
 
 
int Rgb565ConvertBmp(char *buf,int width,int height, const char *filename)
{
	FILE* fp;
 	int i;
	BitMapFileHeader bmfHdr; //定义文件头
	BitMapInfoHeader bmiHdr; //定义信息头
	RgbQuad bmiClr[3]; //定义调色板
 
	bmiHdr.biSize = sizeof(BitMapInfoHeader);
	bmiHdr.biWidth = width;//指定图像的宽度，单位是像素
	bmiHdr.biHeight = height;//指定图像的高度，单位是像素
	bmiHdr.biPlanes = 1;//目标设备的级别，必须是1
	bmiHdr.biBitCount = 16;//表示用到颜色时用到的位数 16位表示高彩色图
	bmiHdr.biCompression = BI_BITFIELDS;//BI_RGB仅有RGB555格式
	bmiHdr.biSizeImage = (width * height * 2);//指定实际位图所占字节数
	bmiHdr.biXPelsPerMeter = 0;//水平分辨率，单位长度内的像素数
	bmiHdr.biYPelsPerMeter = 0;//垂直分辨率，单位长度内的像素数
	bmiHdr.biClrUsed = 0;//位图实际使用的彩色表中的颜色索引数（设为0的话，则说明使用所有调色板项）
	bmiHdr.biClrImportant = 0;//说明对图象显示有重要影响的颜色索引的数目，0表示所有颜色都重要
	//RGB565格式掩码
	bmiClr[0].rgbBlue = 0;
	bmiClr[0].rgbGreen = 0xF8;
	bmiClr[0].rgbRed = 0;
	bmiClr[0].rgbReserved = 0;
	bmiClr[1].rgbBlue = 0xE0;
	bmiClr[1].rgbGreen = 0x07;
	bmiClr[1].rgbRed = 0;
	bmiClr[1].rgbReserved = 0;
	bmiClr[2].rgbBlue = 0x1F;
	bmiClr[2].rgbGreen = 0;
	bmiClr[2].rgbRed = 0;
	bmiClr[2].rgbReserved = 0;
 
	bmfHdr.bfType = (WORD)0x4D42;//文件类型，0x4D42也就是字符'BM'
	bmfHdr.bfSize = (DWORD)(sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader) + sizeof(RgbQuad) * 3 + bmiHdr.biSizeImage);//文件大小
	bmfHdr.bfReserved1 = 0;//保留，必须为0
	bmfHdr.bfReserved2 = 0;//保留，必须为0
	bmfHdr.bfOffBits = (DWORD)(sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader)+ sizeof(RgbQuad) * 3);//实际图像数据偏移量
 
	if (!(fp = fopen(filename, "wb"))){
		return -1;
	} else {
		//printf("file %s open success\n",filename);
	}
 
	fwrite(&bmfHdr, 1, sizeof(BitMapFileHeader), fp); 
	fwrite(&bmiHdr, 1, sizeof(BitMapInfoHeader), fp); 
	fwrite(&bmiClr, 1, 3*sizeof(RgbQuad), fp);
 
	for(i=0; i<height; i++){
		fwrite(buf+(width*(height-i-1)*2), 2, width, fp);
	}
	//printf("Image size=%d, file size=%d, width=%d, height=%d\n", bmiHdr.biSizeImage, bmfHdr.bfSize, width, height);
	//printf("%s over\n", __FUNCTION__);
	fclose(fp);
	return 0;
}


typedef struct fb_dev   {  
	int fd;             /* 帧缓冲设备硬件描述符 */  
	void *pfb;          /* 指向帧缓冲映射到用户空间的首地址 */  
	int xres;           /* 一帧图像的宽度 */  
	int yres;           /* 一帧图像的高度 */  
	int size;           /* 一帧图像的大小 */  
	int bits_per_pixel; /* 每个像素的大小 */  
} fb_dev_t;  
  
int fb_open(fb_dev_t *fbd, char *fbn)  
{  
	struct fb_var_screeninfo vinfo;  
	if((fbd->fd = open(fbn, O_RDWR)) == -1)  {  
		printf("Error: Cannot open framebuffer device.\n");  
		_exit(EXIT_FAILURE);  
	}  
  
	/* 获取LCD 的可变参数 */  
	ioctl(fbd->fd, FBIOGET_VSCREENINFO, &vinfo);  
  
	fbd->xres = vinfo.xres;  
	fbd->yres = vinfo.yres;  
	fbd->bits_per_pixel = vinfo.bits_per_pixel;  
  
	/* 计算一帧图像的大小 */  
	fbd->size = fbd->xres * fbd->yres * fbd->bits_per_pixel / 8;  	  
	printf("%d * %d,%d bits_per_pixel,screensize = %d.\n",fbd->xres,fbd->yres,fbd->bits_per_pixel,fbd->size);  
  
	/* 将帧映射到内存 */  
	/* mmap的应用 */  
	/* mmap可以把文件内容映射到一段内存中，准确说是虚拟内存，通过对这段内存的读取和修改，实现对文件的读取和修改。 */  
	/* addr:指定映射的起始地址，通常为NULL，由系统指定 */  
	/* length:将文件的多大长度映射到内存 */  
	/* prot:映射区的保护方式，可以是可被执行(PROT_EXEC)，可被写入(PROT_WRITE)，可被读取(PROT_READ)，映射区不能存取(PROT_NONE) */  
	/* flags:映射区的特性，对映射区的写入数据会复制回文件，且允许其他映射文件的进城共享(MAP_SHARED)，对映射区的写入操作会产生一个映射的复制，对此区域所做的修改不会写会源文件(MAP_PRIVATE) */  
	/* fd:由open返回的文件描述符，代表要映射的文件 */  
	/* offset:以文件开始出的偏移，必须是分页大小的整数倍，通常为0，表示从头开始映射 */  
	/* 注意:在修改映射文件时，只能在原长度上修改，不能增加文件长度，因为内存是已经分配好的 */   
	fbd->pfb = mmap(NULL, fbd->size, PROT_READ | PROT_WRITE, MAP_SHARED, fbd->fd, 0);  
	if((int)fbd->pfb == -1)  {  
		printf("Error: Failed to map frambuffer device to memory!\n");  
		_exit(EXIT_FAILURE);  
	}  
	printf("open success\n");
	return 0;  
}  

int fb_close(fb_dev_t *fbd)  
{  
	/* 解除映射 */  
	munmap(fbd->pfb,fbd->size);  
  
	/* 关闭设备文件 */  
	close(fbd->fd);
	return 0;
} 


int main()
{
	int i, ret;
	// 打开摄像头设备
	int fd_video;
	char file_name[100];
	memset(file_name,0,sizeof(file_name));
	fd_video = open(CAMERA_DEVICE, O_RDWR, 0);
	if (fd_video < 0) {
		printf("Open %s failed\n", CAMERA_DEVICE);
		return -1;
	}
 
	// 获取驱动信息
	struct v4l2_capability cap;
	ret = ioctl(fd_video, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		printf("VIDIOC_QUERYCAP failed (%d)\n", ret);
		goto __error1;
	}
	printf("Capability Informations:\n");
	printf(" driver: %s\n", cap.driver);
	printf(" card: %s\n", cap.card);
	printf(" bus_info: %s\n", cap.bus_info);
	printf(" version: %u.%u.%u\n", (cap.version>>16)&0XFF, (cap.version>>8)&0XFF,cap.version&0XFF);
	printf(" capabilities: %08X\n", cap.capabilities);

 
    	struct v4l2_format fmt;
	char fmtstr[8];
    	//先获取视频格式
    	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd_video, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		printf("VIDIOC_G_FMT failed (%d)\n", ret);
 		goto __error1;
	}
	// Print Stream Format
	printf("Stream Format Informations default:\n");
	printf(" type: %d\n", fmt.type);
	printf(" width: %d\n", fmt.fmt.pix.width);
	printf(" height: %d\n", fmt.fmt.pix.height);
	memset(fmtstr, 0, 8);
	memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
	printf(" pixelformat: %s\n", fmtstr);
	printf(" field: %d\n", fmt.fmt.pix.field);
	printf(" bytesperline: %d\n", fmt.fmt.pix.bytesperline);
	printf(" sizeimage: %d\n", fmt.fmt.pix.sizeimage);
	printf(" colorspace: %d\n", fmt.fmt.pix.colorspace);
	printf(" priv: %d\n", fmt.fmt.pix.priv);
	printf(" raw_date: %s\n", fmt.fmt.raw_data);

	
	//setting the video format,our lcd format is 240*320,so we modify the output format
	fmt.fmt.pix.width=VIDEO_WIDTH;
	fmt.fmt.pix.height=VIDEO_HEIGHT;
	fmt.fmt.pix.bytesperline=VIDEO_WIDTH*2;
	fmt.fmt.pix.sizeimage=VIDEO_WIDTH*VIDEO_HEIGHT*2;
	ret = ioctl(fd_video, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
		printf("VIDIOC_S_FMT failed (%d)\n", ret);
		goto __error1;
	}

	//after set finish,get the format agagin,make sure it set successful
	ret = ioctl(fd_video, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		printf("VIDIOC_G_FMT failed (%d)\n", ret);
        goto __error1;
	}
	// Print Stream Format
	printf("Stream Format Informations after:\n");
	printf(" type: %d\n", fmt.type);
	printf(" width: %d\n", fmt.fmt.pix.width);
	printf(" height: %d\n", fmt.fmt.pix.height);
	memset(fmtstr, 0, 8);
	memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
	printf(" pixelformat: %s\n", fmtstr);
	printf(" field: %d\n", fmt.fmt.pix.field);
	printf(" bytesperline: %d\n", fmt.fmt.pix.bytesperline);
	printf(" sizeimage: %d\n", fmt.fmt.pix.sizeimage);
	printf(" colorspace: %d\n", fmt.fmt.pix.colorspace);
	printf(" priv: %d\n", fmt.fmt.pix.priv);
	printf(" raw_date: %s\n", fmt.fmt.raw_data);


//-----------------------------------------------
    //请求分配内存
	struct v4l2_requestbuffers reqbuf;
	reqbuf.count = BUFFER_COUNT;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd_video , VIDIOC_REQBUFS, &reqbuf);//在内核虚拟地址空间中申请reqbuf.count个连续的内存
	if(ret < 0) {
		printf("VIDIOC_REQBUFS failed (%d)\n", ret);
		goto __error1;
	}
	struct v4l2_buffer buf;
	for (i = 0; i < reqbuf.count; i++)
	{
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		//根据buf.index询访内存为后面的映射做准备（本质就是设置buf.m.offset+=buf.length偏移）
		ret = ioctl(fd_video , VIDIOC_QUERYBUF, &buf);
		if(ret < 0) {
			printf("VIDIOC_QUERYBUF (%d) failed (%d)\n", i, ret);
			goto __error1;
		}
		//映射到用户空间
		//就是将之前内核分配的视频缓冲（VIDIOC_REQBUFS）映射到用户空间，这样用户空间就可以直接读取内核扑获的视频数据
		//buf.m.offset表示要对内核中的哪个video buffer进行映射操作
		VideoCapturebuf[i].length = buf.length;
		VideoCapturebuf[i].start = (char *) mmap(0, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd_video, buf.m.offset);
		memset(VideoCapturebuf[i].start,i+1,VideoCapturebuf[i].length);
		if (VideoCapturebuf[i].start == MAP_FAILED) {
			printf("mmap (%d) failed: %s\n", i, strerror(errno));
			ret = -1;
			goto __error1;
		}
		//内存入队列
		ret = ioctl(fd_video , VIDIOC_QBUF, &buf);
		if (ret < 0) {
			printf("VIDIOC_QBUF (%d) failed (%d)\n", i, ret);
			goto __error2;
		}
 
		printf("video capture buffer %d: address=0x%x, length=%d\n", i,(unsigned long)VideoCapturebuf[i].start,
        		(unsigned int)VideoCapturebuf[i].length);
	}
//--------------------------------------
 
    // 启动视频流
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd_video, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		printf("VIDIOC_STREAMON failed (%d)\n", ret);
		goto __error2;
	}

	printf("start camera testing...\n");
	//开始视频显示
	fd_set fds;
	struct timeval tv;
	int returnValue;
	/* Timeout. */
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	int count_f;
	fb_dev_t *fbd=NULL;
	if(VIDEO_LCD_SHOW_TEST){
		fbd = (fb_dev_t *)malloc(sizeof(fb_dev_t));
		memset(fbd,0,sizeof(fb_dev_t));
		fb_open(fbd, FB_DEVICE_NAME);
	}
	while(1){
		FD_ZERO (&fds);//将指定的文件描述符集清空
		FD_SET (fd_video, &fds);//在文件描述符集合中增加一个新的文件描述符
		returnValue= select (fd_video + 1, &fds, NULL, NULL, &tv);//判断是否可读（即摄像头是否准备好），tv是定时
		if (-1 == returnValue) {
			printf ("select err/n");
   		 	if (EINTR == errno)
				continue;
		}
		if (0 == returnValue) {
			printf ("select timeout/n");
		}
		memset(&buf,0,sizeof(buf));
		buf.index = 0;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		//内存空间出队列
		ret = ioctl(fd_video, VIDIOC_DQBUF, &buf);
		if (ret < 0){
			printf("VIDIOC_DQBUF failed (%d)\n", ret);
			break;
		}
		//RGB picture test
		if(VIDEO_RGB_BMP_TEST){
			sprintf(file_name,"%s%d%s",RGB_BMP_STORE_LOCATION,count_f,".bmp");
 			Rgb565ConvertBmp(VideoCapturebuf[buf.index].start,VIDEO_WIDTH,VIDEO_HEIGHT,file_name);
			count_f++; 
			if(count_f>=20)
				count_f=0;
		}
		//RGB lcd show test
		if(VIDEO_LCD_SHOW_TEST){
			memcpy(fbd->pfb,VideoCapturebuf[buf.index].start,VideoCapturebuf[buf.index].length);
		}
		// 内存重新入队列
		ret = ioctl(fd_video, VIDIOC_QBUF, &buf);
		if (ret < 0){
			printf("VIDIOC_QBUF failed (%d)\n", ret);
			break;
		}
 
	}
	ret = ioctl(fd_video, VIDIOC_STREAMOFF, &type);
	if (ret < 0) {
		printf("VIDIOC_STREAMON failed (%d)\n", ret);
		goto __error2;
	}
	
__error2://释放framebuf资源
	for (i=0; i< BUFFER_COUNT; i++){
		munmap(VideoCapturebuf[i].start, VideoCapturebuf[i].length);
	}
__error1:
	if(VIDEO_LCD_SHOW_TEST&&fbd){
		fb_close(fbd);
		free(fbd);
	}
	close(fd_video);
}
 
 