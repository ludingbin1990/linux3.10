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
	WORD bfType;//λͼ�ļ������ͣ���Windows�У����ֶε�ֵ��Ϊ��BM��(1-2�ֽڣ�
	DWORD bfSize;//λͼ�ļ��Ĵ�С�����ֽ�Ϊ��λ��3-6�ֽڣ���λ��ǰ��
	WORD bfReserved1;//λͼ�ļ������֣�����Ϊ0(7-8�ֽڣ�
	WORD bfReserved2;//λͼ�ļ������֣�����Ϊ0(9-10�ֽڣ�
	DWORD bfOffBits;//λͼ���ݵ���ʼλ�ã��������λͼ��11-14�ֽڣ���λ��ǰ��
	//�ļ�ͷ��ƫ������ʾ�����ֽ�Ϊ��λ
}__attribute__((packed)) BitMapFileHeader;	//BITMAPFILEHEADER;
 
typedef struct tagBITMAPINFOHEADER{
	DWORD biSize;//���ṹ��ռ���ֽ�����15-18�ֽڣ�
	LONG biWidth;//λͼ�Ŀ�ȣ�������Ϊ��λ��19-22�ֽڣ�
	LONG biHeight;//λͼ�ĸ߶ȣ�������Ϊ��λ��23-26�ֽڣ�
	WORD biPlanes;//Ŀ���豸�ļ��𣬱���Ϊ1(27-28�ֽڣ�
	WORD biBitCount;//ÿ�����������λ����������1��˫ɫ������29-30�ֽڣ�
	//4(16ɫ����8(256ɫ��16(�߲�ɫ)��24�����ɫ��֮һ
	DWORD biCompression;//λͼѹ�����ͣ�������0����ѹ��������31-34�ֽڣ�
	//  1(BI_RLE8ѹ�����ͣ���2(BI_RLE4ѹ�����ͣ�֮һ
	DWORD biSizeImage;//λͼ�Ĵ�С(���а�����Ϊ�˲���������4�ı�������ӵĿ��ֽ�)�����ֽ�Ϊ��λ��35-38�ֽڣ�
	LONG biXPelsPerMeter;//λͼˮƽ�ֱ��ʣ���������39-42�ֽڣ�
	LONG biYPelsPerMeter;//λͼ��ֱ�ֱ��ʣ���������43-46�ֽ�)
	DWORD biClrUsed;//λͼʵ��ʹ�õ���ɫ���е���ɫ����47-50�ֽڣ�
	DWORD biClrImportant;//λͼ��ʾ��������Ҫ����ɫ����51-54�ֽڣ�
}__attribute__((packed)) BitMapInfoHeader;	//BITMAPINFOHEADER;
 
 
typedef struct tagRGBQUAD{
	BYTE rgbBlue;//��ɫ�����ȣ�ֵ��ΧΪ0-255)
	BYTE rgbGreen;//��ɫ�����ȣ�ֵ��ΧΪ0-255)
	BYTE rgbRed;//��ɫ�����ȣ�ֵ��ΧΪ0-255)
	BYTE rgbReserved;//����������Ϊ0
}__attribute__((packed)) RgbQuad;	//RGBQUAD;
 
 
 
 
 
int Rgb565ConvertBmp(char *buf,int width,int height, const char *filename)
{
	FILE* fp;
 	int i;
	BitMapFileHeader bmfHdr; //�����ļ�ͷ
	BitMapInfoHeader bmiHdr; //������Ϣͷ
	RgbQuad bmiClr[3]; //�����ɫ��
 
	bmiHdr.biSize = sizeof(BitMapInfoHeader);
	bmiHdr.biWidth = width;//ָ��ͼ��Ŀ�ȣ���λ������
	bmiHdr.biHeight = height;//ָ��ͼ��ĸ߶ȣ���λ������
	bmiHdr.biPlanes = 1;//Ŀ���豸�ļ��𣬱�����1
	bmiHdr.biBitCount = 16;//��ʾ�õ���ɫʱ�õ���λ�� 16λ��ʾ�߲�ɫͼ
	bmiHdr.biCompression = BI_BITFIELDS;//BI_RGB����RGB555��ʽ
	bmiHdr.biSizeImage = (width * height * 2);//ָ��ʵ��λͼ��ռ�ֽ���
	bmiHdr.biXPelsPerMeter = 0;//ˮƽ�ֱ��ʣ���λ�����ڵ�������
	bmiHdr.biYPelsPerMeter = 0;//��ֱ�ֱ��ʣ���λ�����ڵ�������
	bmiHdr.biClrUsed = 0;//λͼʵ��ʹ�õĲ�ɫ���е���ɫ����������Ϊ0�Ļ�����˵��ʹ�����е�ɫ���
	bmiHdr.biClrImportant = 0;//˵����ͼ����ʾ����ҪӰ�����ɫ��������Ŀ��0��ʾ������ɫ����Ҫ
	//RGB565��ʽ����
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
 
	bmfHdr.bfType = (WORD)0x4D42;//�ļ����ͣ�0x4D42Ҳ�����ַ�'BM'
	bmfHdr.bfSize = (DWORD)(sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader) + sizeof(RgbQuad) * 3 + bmiHdr.biSizeImage);//�ļ���С
	bmfHdr.bfReserved1 = 0;//����������Ϊ0
	bmfHdr.bfReserved2 = 0;//����������Ϊ0
	bmfHdr.bfOffBits = (DWORD)(sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader)+ sizeof(RgbQuad) * 3);//ʵ��ͼ������ƫ����
 
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
	int fd;             /* ֡�����豸Ӳ�������� */  
	void *pfb;          /* ָ��֡����ӳ�䵽�û��ռ���׵�ַ */  
	int xres;           /* һ֡ͼ��Ŀ�� */  
	int yres;           /* һ֡ͼ��ĸ߶� */  
	int size;           /* һ֡ͼ��Ĵ�С */  
	int bits_per_pixel; /* ÿ�����صĴ�С */  
} fb_dev_t;  
  
int fb_open(fb_dev_t *fbd, char *fbn)  
{  
	struct fb_var_screeninfo vinfo;  
	if((fbd->fd = open(fbn, O_RDWR)) == -1)  {  
		printf("Error: Cannot open framebuffer device.\n");  
		_exit(EXIT_FAILURE);  
	}  
  
	/* ��ȡLCD �Ŀɱ���� */  
	ioctl(fbd->fd, FBIOGET_VSCREENINFO, &vinfo);  
  
	fbd->xres = vinfo.xres;  
	fbd->yres = vinfo.yres;  
	fbd->bits_per_pixel = vinfo.bits_per_pixel;  
  
	/* ����һ֡ͼ��Ĵ�С */  
	fbd->size = fbd->xres * fbd->yres * fbd->bits_per_pixel / 8;  	  
	printf("%d * %d,%d bits_per_pixel,screensize = %d.\n",fbd->xres,fbd->yres,fbd->bits_per_pixel,fbd->size);  
  
	/* ��֡ӳ�䵽�ڴ� */  
	/* mmap��Ӧ�� */  
	/* mmap���԰��ļ�����ӳ�䵽һ���ڴ��У�׼ȷ˵�������ڴ棬ͨ��������ڴ�Ķ�ȡ���޸ģ�ʵ�ֶ��ļ��Ķ�ȡ���޸ġ� */  
	/* addr:ָ��ӳ�����ʼ��ַ��ͨ��ΪNULL����ϵͳָ�� */  
	/* length:���ļ��Ķ�󳤶�ӳ�䵽�ڴ� */  
	/* prot:ӳ�����ı�����ʽ�������ǿɱ�ִ��(PROT_EXEC)���ɱ�д��(PROT_WRITE)���ɱ���ȡ(PROT_READ)��ӳ�������ܴ�ȡ(PROT_NONE) */  
	/* flags:ӳ���������ԣ���ӳ������д�����ݻḴ�ƻ��ļ�������������ӳ���ļ��Ľ��ǹ���(MAP_SHARED)����ӳ������д����������һ��ӳ��ĸ��ƣ��Դ������������޸Ĳ���д��Դ�ļ�(MAP_PRIVATE) */  
	/* fd:��open���ص��ļ�������������Ҫӳ����ļ� */  
	/* offset:���ļ���ʼ����ƫ�ƣ������Ƿ�ҳ��С����������ͨ��Ϊ0����ʾ��ͷ��ʼӳ�� */  
	/* ע��:���޸�ӳ���ļ�ʱ��ֻ����ԭ�������޸ģ����������ļ����ȣ���Ϊ�ڴ����Ѿ�����õ� */   
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
	/* ���ӳ�� */  
	munmap(fbd->pfb,fbd->size);  
  
	/* �ر��豸�ļ� */  
	close(fbd->fd);
	return 0;
} 


int main()
{
	int i, ret;
	// ������ͷ�豸
	int fd_video;
	char file_name[100];
	memset(file_name,0,sizeof(file_name));
	fd_video = open(CAMERA_DEVICE, O_RDWR, 0);
	if (fd_video < 0) {
		printf("Open %s failed\n", CAMERA_DEVICE);
		return -1;
	}
 
	// ��ȡ������Ϣ
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
    	//�Ȼ�ȡ��Ƶ��ʽ
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
    //��������ڴ�
	struct v4l2_requestbuffers reqbuf;
	reqbuf.count = BUFFER_COUNT;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd_video , VIDIOC_REQBUFS, &reqbuf);//���ں������ַ�ռ�������reqbuf.count���������ڴ�
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
		//����buf.indexѯ���ڴ�Ϊ�����ӳ����׼�������ʾ�������buf.m.offset+=buf.lengthƫ�ƣ�
		ret = ioctl(fd_video , VIDIOC_QUERYBUF, &buf);
		if(ret < 0) {
			printf("VIDIOC_QUERYBUF (%d) failed (%d)\n", i, ret);
			goto __error1;
		}
		//ӳ�䵽�û��ռ�
		//���ǽ�֮ǰ�ں˷������Ƶ���壨VIDIOC_REQBUFS��ӳ�䵽�û��ռ䣬�����û��ռ�Ϳ���ֱ�Ӷ�ȡ�ں��˻����Ƶ����
		//buf.m.offset��ʾҪ���ں��е��ĸ�video buffer����ӳ�����
		VideoCapturebuf[i].length = buf.length;
		VideoCapturebuf[i].start = (char *) mmap(0, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd_video, buf.m.offset);
		memset(VideoCapturebuf[i].start,i+1,VideoCapturebuf[i].length);
		if (VideoCapturebuf[i].start == MAP_FAILED) {
			printf("mmap (%d) failed: %s\n", i, strerror(errno));
			ret = -1;
			goto __error1;
		}
		//�ڴ������
		ret = ioctl(fd_video , VIDIOC_QBUF, &buf);
		if (ret < 0) {
			printf("VIDIOC_QBUF (%d) failed (%d)\n", i, ret);
			goto __error2;
		}
 
		printf("video capture buffer %d: address=0x%x, length=%d\n", i,(unsigned long)VideoCapturebuf[i].start,
        		(unsigned int)VideoCapturebuf[i].length);
	}
//--------------------------------------
 
    // ������Ƶ��
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd_video, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		printf("VIDIOC_STREAMON failed (%d)\n", ret);
		goto __error2;
	}

	printf("start camera testing...\n");
	//��ʼ��Ƶ��ʾ
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
		FD_ZERO (&fds);//��ָ�����ļ������������
		FD_SET (fd_video, &fds);//���ļ�����������������һ���µ��ļ�������
		returnValue= select (fd_video + 1, &fds, NULL, NULL, &tv);//�ж��Ƿ�ɶ���������ͷ�Ƿ�׼���ã���tv�Ƕ�ʱ
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
		//�ڴ�ռ������
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
		// �ڴ����������
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
	
__error2://�ͷ�framebuf��Դ
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
 
 