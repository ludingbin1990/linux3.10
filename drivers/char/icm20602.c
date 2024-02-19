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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define ICM20602_CNT	1
#define ICM20602_NAME	"ICM20602"

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


struct icm20602_dev {
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* 类 		*/
	struct device *device;	/* 设备 	 */
	struct device_node	*nd; /* 设备节点 */
	int major;			/* 主设备号 */
	void *private_data;	/* 私有数据 */
    unsigned short icm_gyro_x;
    unsigned short icm_gyro_y;
    unsigned short icm_gyro_z;
    unsigned short icm_acc_x;
    unsigned short icm_acc_y;
    unsigned short icm_acc_z;
};

static struct icm20602_dev icm20602dev;
static int sensor_init = 0;
/*
 * @description	: 从ap3216c读取多个寄存器数据
 * @param - dev:  ap3216c设备
 * @param - reg:  要读取的寄存器首地址
 * @param - val:  读取到的数据
 * @param - len:  要读取的数据长度
 * @return 		: 操作结果
 */
static int icm20602_read_regs(struct icm20602_dev *dev, u8 reg, void *val, int len)
{
	int ret;
	struct i2c_msg msg[2];
	struct i2c_client *client = (struct i2c_client *)dev->private_data;

	/* msg[0]为发送要读取的首地址 */
	msg[0].addr = client->addr;			/* ap3216c地址 */
	msg[0].flags = 0;					/* 标记为发送数据 */
	msg[0].buf = &reg;					/* 读取的首地址 */
	msg[0].len = 1;						/* reg长度*/

	/* msg[1]读取数据 */
	msg[1].addr = client->addr;			/* ap3216c地址 */
	msg[1].flags = I2C_M_RD;			/* 标记为读取数据*/
	msg[1].buf = val;					/* 读取数据缓冲区 */
	msg[1].len = len;					/* 要读取的数据长度*/

	ret = i2c_transfer(client->adapter, msg, 2);
	if(ret == 2) {
		ret = 0;
	} else {
		printk("i2c rd failed=%d reg=%06x len=%d\n",ret, reg, len);
		ret = -EREMOTEIO;
	}
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
	u8 b[256] = {0};
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	
	b[0] = reg;					/* 寄存器首地址 */
	memcpy(&b[1],buf,len);		/* 将要写入的数据拷贝到数组b里面 */
		
	msg.addr = client->addr;	/* icm20602地址 */
	msg.flags = 0;				/* 标记为写数据 */

	msg.buf = b;				/* 要写入的数据缓冲区 */
	msg.len = len + 1;			/* 要写入的数据长度 */

	return i2c_transfer(client->adapter, &msg, 1);
}

/*
 * @description	: 读取icm20602指定寄存器值，读取一个寄存器
 * @param - dev:  icm20602设备
 * @param - reg:  要读取的寄存器
 * @return 	  :   读取到的寄存器值
 */
static unsigned char icm20602_read_reg(struct icm20602_dev *dev, u8 reg)
{
	u8 data = 0;

	icm20602_read_regs(dev, reg, &data, 1);
	return data;

#if 0
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	return i2c_smbus_read_byte_data(client, reg);
#endif
}

/*
 * @description	: 向ap3216c指定寄存器写入指定的值，写一个寄存器
 * @param - dev:  ap3216c设备
 * @param - reg:  要写的寄存器
 * @param - data: 要写入的值
 * @return   :    无
 */
static void icm20602_write_reg(struct icm20602_dev *dev, u8 reg, u8 data)
{
	u8 buf = 0;
	buf = data;
	icm20602_write_regs(dev, reg, &buf, 1);

    return;
}

void get_icm20602_accdata(void)
{
    unsigned char dat[6] = {0};

    dat[0] = icm20602_read_reg(&icm20602dev, ICM20602_ACCEL_XOUT_H);
    dat[1] = icm20602_read_reg(&icm20602dev, ICM20602_ACCEL_XOUT_L);
    dat[2] = icm20602_read_reg(&icm20602dev, ICM20602_ACCEL_YOUT_H);
    dat[3] = icm20602_read_reg(&icm20602dev, ICM20602_ACCEL_YOUT_L);
    dat[4] = icm20602_read_reg(&icm20602dev, ICM20602_ACCEL_ZOUT_H);
    dat[5] = icm20602_read_reg(&icm20602dev, ICM20602_ACCEL_ZOUT_L);

    icm20602dev.icm_gyro_x = (unsigned short)(((unsigned short)dat[0]<<8 | dat[1]));
    icm20602dev.icm_acc_y = (unsigned short)(((unsigned short)dat[2]<<8 | dat[3]));
    icm20602dev.icm_acc_z = (unsigned short)(((unsigned short)dat[4]<<8 | dat[5]));

    return ;
}

//-------------------------------------------------------------------------------------------------------------------
//  @brief      ��ȡICM20602����������
//  @param      NULL
//  @return     void
//  @since      v1.0
//  Sample usage:				ִ�иú�����ֱ�Ӳ鿴��Ӧ�ı�������
//-------------------------------------------------------------------------------------------------------------------
void get_icm20602_gyro(void)
{
    unsigned char  dat[6] = {0};

    dat[0] = icm20602_read_reg(&icm20602dev, ICM20602_GYRO_XOUT_H);
    dat[1] = icm20602_read_reg(&icm20602dev, ICM20602_GYRO_XOUT_L);
    dat[2] = icm20602_read_reg(&icm20602dev, ICM20602_GYRO_YOUT_H);
    dat[3] = icm20602_read_reg(&icm20602dev, ICM20602_GYRO_YOUT_L);
    dat[4] = icm20602_read_reg(&icm20602dev, ICM20602_GYRO_ZOUT_H);
    dat[5] = icm20602_read_reg(&icm20602dev, ICM20602_GYRO_ZOUT_L);

    icm20602dev.icm_gyro_x = (unsigned short)(((unsigned short)dat[0]<<8 | dat[1]));
    icm20602dev.icm_gyro_y = (unsigned short)(((unsigned short)dat[2]<<8 | dat[3]));
    icm20602dev.icm_gyro_z = (unsigned short)(((unsigned short)dat[4]<<8 | dat[5]));

    return ;
}



/*
 * @description	: 读取AP3216C的数据，读取原始数据，包括ALS,PS和IR, 注意！
 *				: 如果同时打开ALS,IR+PS的话两次数据读取的时间间隔要大于112.5ms
 * @param - ir	: ir数据
 * @param - ps 	: ps数据
 * @param - ps 	: als数据 
 * @return 		: 无。
 */
void icm20602_readdata(struct icm20602_dev *dev)
{
    get_icm20602_accdata();

    get_icm20602_gyro();

    return ;
	
}

void icm20602_self1_check(void)
{
	u8 data_1 = 0;
    while(0x12 != data_1) //��ȡICM20602 ID
    {
		
		data_1 = icm20602_read_reg(&icm20602dev, ICM20602_WHO_AM_I);
        //��������ԭ�������¼���
        //1 MPU6050���ˣ�������µ������ĸ��ʼ���
        //2 ���ߴ������û�нӺ�
        //3 ��������Ҫ����������裬������3.3V
    }
}

void icm20602_initial(void)
{
    mdelay(10);
    dev_info(icm20602dev.device, "icm20602_initial start\n");
    icm20602_self1_check();

    icm20602_write_reg(&icm20602dev,ICM20602_PWR_MGMT_1,0x80);
    mdelay(20);
    while(0x80 & icm20602_read_reg(&icm20602dev,ICM20602_PWR_MGMT_1));
    
    icm20602_write_reg(&icm20602dev,ICM20602_PWR_MGMT_1,0x01);
    icm20602_write_reg(&icm20602dev,ICM20602_PWR_MGMT_2,0x00);
    icm20602_write_reg(&icm20602dev,ICM20602_CONFIG,0x01);                   //176HZ 1KHZ
    icm20602_write_reg(&icm20602dev,ICM20602_SMPLRT_DIV,0x07);               //�������� SAMPLE_RATE = INTERNAL_SAMPLE_RATE / (1 + SMPLRT_DIV)
    icm20602_write_reg(&icm20602dev,ICM20602_GYRO_CONFIG,0x18);              //��2000 dps
    icm20602_write_reg(&icm20602dev,ICM20602_ACCEL_CONFIG,0x10);             //��8g
    icm20602_write_reg(&icm20602dev,ICM20602_ACCEL_CONFIG_2,0x03);           //Average 4 samples   44.8HZ   //0x23 Average 16 samples
	//ICM20602_GYRO_CONFIG�Ĵ���
    //����Ϊ:0x00 ����������Ϊ:��250 dps     ��ȡ�������������ݳ���131           ����ת��Ϊ������λ�����ݣ� ��λΪ����/s
    //����Ϊ:0x08 ����������Ϊ:��500 dps     ��ȡ�������������ݳ���65.5          ����ת��Ϊ������λ�����ݣ���λΪ����/s
    //����Ϊ:0x10 ����������Ϊ:��1000dps     ��ȡ�������������ݳ���32.8          ����ת��Ϊ������λ�����ݣ���λΪ����/s
    //����Ϊ:0x18 ����������Ϊ:��2000dps     ��ȡ�������������ݳ���16.4          ����ת��Ϊ������λ�����ݣ���λΪ����/s

    //ICM20602_ACCEL_CONFIG�Ĵ���
    //����Ϊ:0x00 ���ٶȼ�����Ϊ:��2g          ��ȡ���ļ��ٶȼ����� ����16384      ����ת��Ϊ������λ�����ݣ���λ��g(m/s^2)
    //����Ϊ:0x08 ���ٶȼ�����Ϊ:��4g          ��ȡ���ļ��ٶȼ����� ����8192       ����ת��Ϊ������λ�����ݣ���λ��g(m/s^2)
    //����Ϊ:0x10 ���ٶȼ�����Ϊ:��8g          ��ȡ���ļ��ٶȼ����� ����4096       ����ת��Ϊ������λ�����ݣ���λ��g(m/s^2)
    //����Ϊ:0x18 ���ٶȼ�����Ϊ:��16g         ��ȡ���ļ��ٶȼ����� ����2048       ����ת��Ϊ������λ�����ݣ���λ��g(m/s^2)
    dev_info(icm20602dev.device, "icm20602_initial done\n");

    return;
}

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int icm20602_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &icm20602dev;

    if (!sensor_init) {
        icm20602_initial();
        sensor_init = 1;
    }

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
	unsigned short data[6] = {0};
	long err = 0;

	struct icm20602_dev *dev = (struct icm20602_dev *)filp->private_data;
	
	icm20602_readdata(dev);

	data[0] = dev->icm_gyro_x;
	data[1] = dev->icm_gyro_y;
	data[2] = dev->icm_gyro_z;
    data[3] = dev->icm_acc_x;
    data[4] = dev->icm_acc_y;
    data[5] = dev->icm_acc_z;
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

/* AP3216C操作函数 */
static const struct file_operations icm20602_ops = {
	.owner = THIS_MODULE,
	.open = icm20602_open,
	.read = icm20602_read,
	.release = icm20602_release,
};

 /*
  * @description     : i2c驱动的probe函数，当驱动与
  *                    设备匹配以后此函数就会执行
  * @param - client  : i2c设备
  * @param - id      : i2c设备ID
  * @return          : 0，成功;其他负值,失败
  */
static int icm20602_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

    dev_info(&client->dev, "icm20602_probe\n");
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

	icm20602dev.private_data = client;

	return 0;
}

/*
 * @description     : i2c驱动的remove函数，移除i2c驱动的时候此函数会执行
 * @param - client 	: i2c设备
 * @return          : 0，成功;其他负值,失败
 */
static int icm20602_remove(struct i2c_client *client)
{
	/* 删除设备 */
	cdev_del(&icm20602dev.cdev);
	unregister_chrdev_region(icm20602dev.devid, ICM20602_CNT);

	/* 注销掉类和设备 */
	device_destroy(icm20602dev.class, icm20602dev.devid);
	class_destroy(icm20602dev.class);
	return 0;
}

/* 传统匹配方式ID列表 */
static const struct i2c_device_id icm20602_id[] = {
	{"icm20602", 0},  
	{}
};

/* 设备树匹配列表 */
static const struct of_device_id icm20602_of_match[] = {
	{ .compatible = "icm20602" },
	{ /* Sentinel */ }
};

/* i2c驱动结构体 */	
static struct i2c_driver icm20602_driver = {
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
	int ret = 0;

	ret = i2c_add_driver(&icm20602_driver);
	return ret;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit icm20602_exit(void)
{
	i2c_del_driver(&icm20602_driver);
}

/* module_i2c_driver(ap3216c_driver) */

module_init(icm20602_init);
module_exit(icm20602_exit);
MODULE_LICENSE("GPL");


