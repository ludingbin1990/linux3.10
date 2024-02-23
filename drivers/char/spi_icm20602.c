#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/***************************************************************/
#define ICM20602_CNT	1
#define ICM20602_NAME	"ICM20602"

#define     ICM20602_DEV_ADDR           0x69 //SA0�ӵأ�0x68   SA0������0x69  ģ��Ĭ������


#define     ICM20602_SPI_W              0x00
#define     ICM20602_SPI_R              0x80


#define     ICM20602_XG_OFFS_TC_H       0x04
#define     ICM20602_XG_OFFS_TC_L       0x05
#define     ICM20602_YG_OFFS_TC_H       0x07
#define     ICM20602_YG_OFFS_TC_L       0x08
#define     ICM20602_ZG_OFFS_TC_H       0x0A
#define     ICM20602_ZG_OFFS_TC_L       0x0B
#define     ICM20602_SELF_TEST_X_ACCEL  0x0D
#define     ICM20602_SELF_TEST_Y_ACCEL  0x0E
#define     ICM20602_SELF_TEST_Z_ACCEL  0x0F
#define     ICM20602_XG_OFFS_USRH       0x13
#define     ICM20602_XG_OFFS_USRL       0x14
#define     ICM20602_YG_OFFS_USRH       0x15
#define     ICM20602_YG_OFFS_USRL       0x16
#define     ICM20602_ZG_OFFS_USRH       0x17
#define     ICM20602_ZG_OFFS_USRL       0x18
#define     ICM20602_SMPLRT_DIV         0x19
#define     ICM20602_CONFIG             0x1A
#define     ICM20602_GYRO_CONFIG        0x1B
#define     ICM20602_ACCEL_CONFIG       0x1C
#define     ICM20602_ACCEL_CONFIG_2     0x1D
#define     ICM20602_LP_MODE_CFG        0x1E
#define     ICM20602_ACCEL_WOM_X_THR    0x20
#define     ICM20602_ACCEL_WOM_Y_THR    0x21
#define     ICM20602_ACCEL_WOM_Z_THR    0x22
#define     ICM20602_FIFO_EN            0x23
#define     ICM20602_FSYNC_INT          0x36
#define     ICM20602_INT_PIN_CFG        0x37
#define     ICM20602_INT_ENABLE         0x38
#define     ICM20602_FIFO_WM_INT_STATUS 0x39 
#define     ICM20602_INT_STATUS         0x3A
#define     ICM20602_ACCEL_XOUT_H       0x3B
#define     ICM20602_ACCEL_XOUT_L       0x3C
#define     ICM20602_ACCEL_YOUT_H       0x3D
#define     ICM20602_ACCEL_YOUT_L       0x3E
#define     ICM20602_ACCEL_ZOUT_H       0x3F
#define     ICM20602_ACCEL_ZOUT_L       0x40
#define     ICM20602_TEMP_OUT_H         0x41
#define     ICM20602_TEMP_OUT_L         0x42
#define     ICM20602_GYRO_XOUT_H        0x43
#define     ICM20602_GYRO_XOUT_L        0x44
#define     ICM20602_GYRO_YOUT_H        0x45
#define     ICM20602_GYRO_YOUT_L        0x46
#define     ICM20602_GYRO_ZOUT_H        0x47
#define     ICM20602_GYRO_ZOUT_L        0x48
#define     ICM20602_SELF_TEST_X_GYRO   0x50
#define     ICM20602_SELF_TEST_Y_GYRO   0x51
#define     ICM20602_SELF_TEST_Z_GYRO   0x52
#define     ICM20602_FIFO_WM_TH1        0x60
#define     ICM20602_FIFO_WM_TH2        0x61
#define     ICM20602_SIGNAL_PATH_RESET  0x68
#define     ICM20602_ACCEL_INTEL_CTRL   0x69
#define     ICM20602_USER_CTRL          0x6A
#define     ICM20602_PWR_MGMT_1         0x6B
#define     ICM20602_PWR_MGMT_2         0x6C
#define     ICM20602_I2C_IF             0x70
#define     ICM20602_FIFO_COUNTH        0x72
#define     ICM20602_FIFO_COUNTL        0x73
#define     ICM20602_FIFO_R_W           0x74
#define     ICM20602_WHO_AM_I           0x75
#define     ICM20602_XA_OFFSET_H        0x77
#define     ICM20602_XA_OFFSET_L        0x78
#define     ICM20602_YA_OFFSET_H        0x7A
#define     ICM20602_YA_OFFSET_L        0x7B
#define     ICM20602_ZA_OFFSET_H        0x7D
#define     ICM20602_ZA_OFFSET_L        0x7E

#define GPGCON 0x0
#define GPGDAT 0x4
#define GPGUP 0x8


struct icm20602_dev {
	dev_t devid;				/* 设备号 	 */
	struct cdev cdev;			/* cdev 	*/
	struct class *class;		/* 类 		*/
	struct device *device;		/* 设备 	 */
	struct device_node	*nd; 	/* 设备节点 */
	int major;					/* 主设备号 */
	void *private_data;			/* 私有数据 		*/
	int cs_gpio;				/* 片选所使用的GPIO编号		*/
	signed int gyro_x_adc;		/* 陀螺仪X轴原始值 	 */
	signed int gyro_y_adc;		/* 陀螺仪Y轴原始值		*/
	signed int gyro_z_adc;		/* 陀螺仪Z轴原始值 		*/
	signed int accel_x_adc;		/* 加速度计X轴原始值 	*/
	signed int accel_y_adc;		/* 加速度计Y轴原始值	*/
	signed int accel_z_adc;		/* 加速度计Z轴原始值 	*/
	signed int temp_adc;		/* 温度原始值 			*/
    void __iomem		*spi_gpio_regs;
};

static struct icm20602_dev icm20602dev;

/*
 * @description	: 从icm20602读取多个寄存器数据
 * @param - dev:  icm20602设备
 * @param - reg:  要读取的寄存器首地址
 * @param - val:  读取到的数据
 * @param - len:  要读取的数据长度
 * @return 		: 操作结果
 */

static void s3c2440_spi_gpio_set_cs(int pol)
{
    unsigned int tmp_value;
    tmp_value=*((volatile u32 *)(icm20602dev.spi_gpio_regs + GPGDAT));
    tmp_value = (tmp_value & (~(1 << 3))) | (pol << 3);
    *((volatile u32 *)(icm20602dev.spi_gpio_regs + GPGDAT))=tmp_value;

    return ;
}

static int icm20602_read_regs(struct icm20602_dev *dev, u8 reg, void *buf, int len)
{
	int ret;
	unsigned char txdata[1] = {0};
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device *spi = (struct spi_device *)dev->private_data;

	//gpio_set_value(dev->cs_gpio, 0);				/* 片选拉低，选中ICM20602 */
    s3c2440_spi_gpio_set_cs(0);
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);	/* 申请内存 */

	/* 第1次，发送要读取的寄存地址 */
	txdata[0] = reg | 0x80;		/* 读数据的时候寄存器地址bit8要置1 */
    t->cs_change = 0;
    t->speed_hz = 1000000;
	t->tx_buf = txdata;			/* 要发送的数据 */
	t->len = 1;					/* 1个字节 */
	spi_message_init(&m);		/* 初始化spi_message */
	spi_message_add_tail(t, &m);/* 将spi_transfer添加到spi_message队列 */
	ret = spi_sync(spi, &m);	/* 同步发送 */

	/* 第2次，读取数据 */
	//txdata[0] = 0xff;			/* 随便一个值，此处无意义 */
    t->tx_buf = NULL;
	t->rx_buf = buf;			/* 读取到的数据 */
	t->len = len;				/* 要读取的数据长度 */
	spi_message_init(&m);		/* 初始化spi_message */
	spi_message_add_tail(t, &m);/* 将spi_transfer添加到spi_message队列 */
	ret = spi_sync(spi, &m);	/* 同步发送 */

	kfree(t);									/* 释放内存 */
	s3c2440_spi_gpio_set_cs(1);			/* 片选拉高，释放ICM20602 */

	return ret;
}

/*
 * @description	: 向icm20602多个寄存器写入数据
 * @param - dev:  icm20602设备
 * @param - reg:  要写入的寄存器首地址
 * @param - val:  要写入的数据缓冲区
 * @param - len:  要写入的数据长度
 * @return 	  :   操作结果
 */
static s32 icm20602_write_regs(struct icm20602_dev *dev, u8 reg, u8 *buf, u8 len)
{
	int ret;

	unsigned char txdata[1] = {0};
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device *spi = (struct spi_device *)dev->private_data;

	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);	/* 申请内存 */
	s3c2440_spi_gpio_set_cs(0);			/* 片选拉低 */

	/* 第1次，发送要读取的寄存地址 */
	txdata[0] = reg & ~0x80;	/* 写数据的时候寄存器地址bit8要清零 */
    t->speed_hz = 10000000;
    t->cs_change = 0;
	t->tx_buf = txdata;			/* 要发送的数据 */
	t->len = 1;					/* 1个字节 */
	spi_message_init(&m);		/* 初始化spi_message */
	spi_message_add_tail(t, &m);/* 将spi_transfer添加到spi_message队列 */
	ret = spi_sync(spi, &m);	/* 同步发送 */

	/* 第2次，发送要写入的数据 */
	t->tx_buf = buf;			/* 要写入的数据 */
	t->len = len;				/* 写入的字节数 */
	spi_message_init(&m);		/* 初始化spi_message */
	spi_message_add_tail(t, &m);/* 将spi_transfer添加到spi_message队列 */
	ret = spi_sync(spi, &m);	/* 同步发送 */

	kfree(t);					/* 释放内存 */
	s3c2440_spi_gpio_set_cs(1);/* 片选拉高，释放ICM20602 */
	return ret;
}

/*
 * @description	: 读取icm20602指定寄存器值，读取一个寄存器
 * @param - dev:  icm20602设备
 * @param - reg:  要读取的寄存器
 * @return 	  :   读取到的寄存器值
 */
static unsigned char icm20602_read_onereg(struct icm20602_dev *dev, u8 reg)
{
	u8 data = 0;
	icm20602_read_regs(dev, reg, &data, 1);
	return data;
}

/*
 * @description	: 向icm20602指定寄存器写入指定的值，写一个寄存器
 * @param - dev:  icm20602设备
 * @param - reg:  要写的寄存器
 * @param - data: 要写入的值
 * @return   :    无
 */	

static void icm20602_write_onereg(struct icm20602_dev *dev, u8 reg, u8 value)
{
	u8 buf = value;
	icm20602_write_regs(dev, reg, &buf, 1);
}

/*
 * @description	: 读取ICM20602的数据，读取原始数据，包括三轴陀螺仪、
 * 				: 三轴加速度计和内部温度。
 * @param - dev	: ICM20602设备
 * @return 		: 无。
 */
void icm20602_readdata(struct icm20602_dev *dev)
{
	unsigned char data[14];
	icm20602_read_regs(dev, ICM20602_ACCEL_XOUT_H, data, 14);
	dev->accel_x_adc = (signed short)((data[0] << 8) | data[1]); 
	dev->accel_y_adc = (signed short)((data[2] << 8) | data[3]); 
	dev->accel_z_adc = (signed short)((data[4] << 8) | data[5]); 
	dev->temp_adc    = (signed short)((data[6] << 8) | data[7]); 
	dev->gyro_x_adc  = (signed short)((data[8] << 8) | data[9]); 
	dev->gyro_y_adc  = (signed short)((data[10] << 8) | data[11]);
	dev->gyro_z_adc  = (signed short)((data[12] << 8) | data[13]);
}

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做privateate_data的成员变量
 * 					  一般在open的时候将private_data似有向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int icm20602_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &icm20602dev; /* 设置私有数据 */
	return 0;
}

/*
 * @description		: 从设备读取数据 
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t icm20602_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	signed int data[7];
	long err = 0;
	struct icm20602_dev *dev = (struct icm20602_dev *)filp->private_data;

	icm20602_readdata(dev);
	data[0] = dev->gyro_x_adc;
	data[1] = dev->gyro_y_adc;
	data[2] = dev->gyro_z_adc;
	data[3] = dev->accel_x_adc;
	data[4] = dev->accel_y_adc;
	data[5] = dev->accel_z_adc;
	data[6] = dev->temp_adc;
	err = copy_to_user(buf, data, sizeof(data));
	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int icm20602_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* icm20602操作函数 */
static const struct file_operations icm20602_ops = {
	.owner = THIS_MODULE,
	.open = icm20602_open,
	.read = icm20602_read,
	.release = icm20602_release,
};

/*
 * ICM20602内部寄存器初始化函数 
 * @param  	: 无
 * @return 	: 无
 */

void icm20602_reginit(void)
{
	u8 value = 0;

    dev_info(icm20602dev.device, "icm20602_reginit\r\n");
    icm20602_write_onereg(&icm20602dev, ICM20602_I2C_IF,     0x40);
    value = icm20602_read_onereg(&icm20602dev, ICM20602_WHO_AM_I);
    dev_info(icm20602dev.device, "ICM20602 ID = %#X\r\n", value); 

    icm20602_write_onereg(&icm20602dev, ICM20602_PWR_MGMT_1, 0x80);//��λ�豸
    mdelay(50);
    icm20602_write_onereg(&icm20602dev, ICM20602_PWR_MGMT_1,     0x01);            //ʱ������
    mdelay(50);
    icm20602_write_onereg(&icm20602dev, ICM20602_PWR_MGMT_2,     0x00);            //���������Ǻͼ��ٶȼ�
    icm20602_write_onereg(&icm20602dev, ICM20602_CONFIG,         0x01);            //176HZ 1KHZ
    icm20602_write_onereg(&icm20602dev, ICM20602_SMPLRT_DIV,     0x07);            //�������� SAMPLE_RATE = INTERNAL_SAMPLE_RATE / (1 + SMPLRT_DIV)
    icm20602_write_onereg(&icm20602dev, ICM20602_GYRO_CONFIG,    0x18);            //��2000 dps
    icm20602_write_onereg(&icm20602dev, ICM20602_ACCEL_CONFIG,   0x10);            //��8g
    icm20602_write_onereg(&icm20602dev, ICM20602_ACCEL_CONFIG_2, 0x03);            //Average 4 samples   44.8HZ   //0x23 Average 16 samples

    return;
}

 /*
  * @description     : spi驱动的probe函数，当驱动与
  *                    设备匹配以后此函数就会执行
  * @param - client  : spi设备
  * @param - id      : spi设备ID
  * 
  */	
static int icm20602_probe(struct spi_device *spi)
{
    unsigned int tmp_value = 0;
	/* 1、构建设备号 */
	if (icm20602dev.major) {
		icm20602dev.devid = MKDEV(icm20602dev.major, 0);
		register_chrdev_region(icm20602dev.devid, ICM20602_CNT, ICM20602_NAME);
	} else {
		alloc_chrdev_region(&icm20602dev.devid, 0, ICM20602_CNT, ICM20602_NAME);
		icm20602dev.major = MAJOR(icm20602dev.devid);
	}

	/* 2、注册设备 */
	cdev_init(&icm20602dev.cdev, &icm20602_ops);
	cdev_add(&icm20602dev.cdev, icm20602dev.devid, ICM20602_CNT);

	/* 3、创建类 */
	icm20602dev.class = class_create(THIS_MODULE, ICM20602_NAME);
	if (IS_ERR(icm20602dev.class)) {
		return PTR_ERR(icm20602dev.class);
	}

	/* 4、创建设备 */
	icm20602dev.device = device_create(icm20602dev.class, NULL, icm20602dev.devid, NULL, ICM20602_NAME);
	if (IS_ERR(icm20602dev.device)) {
		return PTR_ERR(icm20602dev.device);
	}

	/*初始化spi_device */
	spi->mode = SPI_MODE_0;	/*MODE0，CPOL=0，CPHA=0*/
	spi_setup(spi);
	icm20602dev.private_data = spi; /* 设置私有数据 */

    icm20602dev.spi_gpio_regs=ioremap(0x56000060, 0x10);

    tmp_value=*((volatile u32 *)(icm20602dev.spi_gpio_regs + GPGCON));
    tmp_value = tmp_value & (~(0x3 << 6));
    /*et gpg3 cs pin */
    tmp_value |= 0x1 << 6;
	*((volatile u32 *)(icm20602dev.spi_gpio_regs + GPGCON))=tmp_value;

	/* 初始化ICM20602内部寄存器 */
	icm20602_reginit();		

	return 0;
}

/*
 * @description     : spi驱动的remove函数，移除spi驱动的时候此函数会执行
 * @param - client 	: spi设备
 * @return          : 0，成功;其他负值,失败
 */
static int icm20602_remove(struct spi_device *spi)
{
	/* 删除设备 */
	cdev_del(&icm20602dev.cdev);
	unregister_chrdev_region(icm20602dev.devid, ICM20602_CNT);

	/* 注销掉类和设备 */
	device_destroy(icm20602dev.class, icm20602dev.devid);
	class_destroy(icm20602dev.class);
    if(icm20602dev.spi_gpio_regs) {
		iounmap(icm20602dev.spi_gpio_regs);
    }
	return 0;
}

/* 传统匹配方式ID列表 */
static const struct spi_device_id icm20602_id[] = {
	{"icm20602", 0},  
	{}
};

/* 设备树匹配列表 */
static const struct of_device_id icm20602_of_match[] = {
	{ .compatible = "icm20602" },
	{ /* Sentinel */ }
};

/* SPI驱动结构体 */	
static struct spi_driver icm20602_driver = {
	.probe = icm20602_probe,
	.remove = icm20602_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "icm20602",
		   	.of_match_table = icm20602_of_match, 
		   },
	.id_table = icm20602_id,
};
		   
/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init icm20602_init(void)
{
	return spi_register_driver(&icm20602_driver);
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit icm20602_exit(void)
{
	spi_unregister_driver(&icm20602_driver);
}

module_init(icm20602_init);
module_exit(icm20602_exit);
MODULE_LICENSE("GPL");

