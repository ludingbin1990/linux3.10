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
