#include <unistd.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <linux/fb.h>  
#include <sys/mman.h>  
  
#define FB_DEVICE_NAME "/dev/fb0"  
  
#define MID_COLOR565        0X0F11f
#define RED_COLOR565        0X0F100  
#define GREEN_COLOR565      0X007E0  
#define BLUE_COLOR565       0X0001F  
  
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
}  
  
int fb_drawrect(fb_dev_t *fbd, int x0, int y0, int w, int h, int color)  
{  
	int x,y;  
	printf(" fb_drawrect1 \n");
	for(y=y0; y<y0+h; y++){  
		for(x=x0; x<x0+w; x++){  
			*((unsigned short *)(fbd->pfb) + y*fbd->xres +x) = color;  
		}  
	}  
	printf(" fb_drawrect2 \n");
	return 0;  
}  
  
int main(int argc, char **argv)  
{  
	fb_dev_t *fbd;
	fbd = (fb_dev_t *)malloc(sizeof(fb_dev_t));  
	fb_open(fbd, FB_DEVICE_NAME);  
	if(fbd->bits_per_pixel == 16){  
		printf("Red/Green/Blue Screen!");  
		fb_drawrect(fbd, 0, 0, fbd->xres, fbd->yres/2, RED_COLOR565);  
		fb_drawrect(fbd, 0, 0, fbd->xres, fbd->yres/3, RED_COLOR565);  
		fb_drawrect(fbd, 0, fbd->yres/3, fbd->xres, fbd->yres/3, GREEN_COLOR565);  
		fb_drawrect(fbd, fbd->xres/2, fbd->yres/3, fbd->xres/2, fbd->yres/3, MID_COLOR565);
		fb_drawrect(fbd, 0, fbd->yres*2/3, fbd->xres, fbd->yres/3, BLUE_COLOR565);  
	}  
	else      
		printf("16 bits only!");  
	fb_close(fbd);  
	free(fbd);
	return 0;  
}  
