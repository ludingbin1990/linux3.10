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

#define     ICM20602_DEV_ADDR           0x69 //SA0接地：0x68   SA0上拉：0x69  模块默认上拉


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
	dev_t devid;				/* 璁惧鍙� 	 */
	struct cdev cdev;			/* cdev 	*/
	struct class *class;		/* 绫� 		*/
	struct device *device;		/* 璁惧 	 */
	struct device_node	*nd; 	/* 璁惧鑺傜偣 */
	int major;					/* 涓昏澶囧彿 */
	void *private_data;			/* 绉佹湁鏁版嵁 		*/
	int cs_gpio;				/* 鐗囬�夋墍浣跨敤鐨凣PIO缂栧彿		*/
	signed int gyro_x_adc;		/* 闄�铻轰华X杞村師濮嬪�� 	 */
	signed int gyro_y_adc;		/* 闄�铻轰华Y杞村師濮嬪��		*/
	signed int gyro_z_adc;		/* 闄�铻轰华Z杞村師濮嬪�� 		*/
	signed int accel_x_adc;		/* 鍔犻�熷害璁杞村師濮嬪�� 	*/
	signed int accel_y_adc;		/* 鍔犻�熷害璁杞村師濮嬪��	*/
	signed int accel_z_adc;		/* 鍔犻�熷害璁杞村師濮嬪�� 	*/
	signed int temp_adc;		/* 娓╁害鍘熷鍊� 			*/
    void __iomem		*spi_gpio_regs;
};

static struct icm20602_dev icm20602dev;

/*
 * @description	: 浠巌cm20602璇诲彇澶氫釜瀵勫瓨鍣ㄦ暟鎹�
 * @param - dev:  icm20602璁惧
 * @param - reg:  瑕佽鍙栫殑瀵勫瓨鍣ㄩ鍦板潃
 * @param - val:  璇诲彇鍒扮殑鏁版嵁
 * @param - len:  瑕佽鍙栫殑鏁版嵁闀垮害
 * @return 		: 鎿嶄綔缁撴灉
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

	//gpio_set_value(dev->cs_gpio, 0);				/* 鐗囬�夋媺浣庯紝閫変腑ICM20602 */
    s3c2440_spi_gpio_set_cs(0);
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);	/* 鐢宠鍐呭瓨 */

	/* 绗�1娆★紝鍙戦�佽璇诲彇鐨勫瘎瀛樺湴鍧� */
	txdata[0] = reg | 0x80;		/* 璇绘暟鎹殑鏃跺�欏瘎瀛樺櫒鍦板潃bit8瑕佺疆1 */
    t->cs_change = 0;
    t->speed_hz = 1000000;
	t->tx_buf = txdata;			/* 瑕佸彂閫佺殑鏁版嵁 */
	t->len = 1;					/* 1涓瓧鑺� */
	spi_message_init(&m);		/* 鍒濆鍖杝pi_message */
	spi_message_add_tail(t, &m);/* 灏唖pi_transfer娣诲姞鍒皊pi_message闃熷垪 */
	ret = spi_sync(spi, &m);	/* 鍚屾鍙戦�� */

	/* 绗�2娆★紝璇诲彇鏁版嵁 */
	//txdata[0] = 0xff;			/* 闅忎究涓�涓�硷紝姝ゅ鏃犳剰涔� */
    t->tx_buf = NULL;
	t->rx_buf = buf;			/* 璇诲彇鍒扮殑鏁版嵁 */
	t->len = len;				/* 瑕佽鍙栫殑鏁版嵁闀垮害 */
	spi_message_init(&m);		/* 鍒濆鍖杝pi_message */
	spi_message_add_tail(t, &m);/* 灏唖pi_transfer娣诲姞鍒皊pi_message闃熷垪 */
	ret = spi_sync(spi, &m);	/* 鍚屾鍙戦�� */

	kfree(t);									/* 閲婃斁鍐呭瓨 */
	s3c2440_spi_gpio_set_cs(1);			/* 鐗囬�夋媺楂橈紝閲婃斁ICM20602 */

	return ret;
}

/*
 * @description	: 鍚慽cm20602澶氫釜瀵勫瓨鍣ㄥ啓鍏ユ暟鎹�
 * @param - dev:  icm20602璁惧
 * @param - reg:  瑕佸啓鍏ョ殑瀵勫瓨鍣ㄩ鍦板潃
 * @param - val:  瑕佸啓鍏ョ殑鏁版嵁缂撳啿鍖�
 * @param - len:  瑕佸啓鍏ョ殑鏁版嵁闀垮害
 * @return 	  :   鎿嶄綔缁撴灉
 */
static s32 icm20602_write_regs(struct icm20602_dev *dev, u8 reg, u8 *buf, u8 len)
{
	int ret;

	unsigned char txdata[1] = {0};
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device *spi = (struct spi_device *)dev->private_data;

	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);	/* 鐢宠鍐呭瓨 */
	s3c2440_spi_gpio_set_cs(0);			/* 鐗囬�夋媺浣� */

	/* 绗�1娆★紝鍙戦�佽璇诲彇鐨勫瘎瀛樺湴鍧� */
	txdata[0] = reg & ~0x80;	/* 鍐欐暟鎹殑鏃跺�欏瘎瀛樺櫒鍦板潃bit8瑕佹竻闆� */
    t->speed_hz = 10000000;
    t->cs_change = 0;
	t->tx_buf = txdata;			/* 瑕佸彂閫佺殑鏁版嵁 */
	t->len = 1;					/* 1涓瓧鑺� */
	spi_message_init(&m);		/* 鍒濆鍖杝pi_message */
	spi_message_add_tail(t, &m);/* 灏唖pi_transfer娣诲姞鍒皊pi_message闃熷垪 */
	ret = spi_sync(spi, &m);	/* 鍚屾鍙戦�� */

	/* 绗�2娆★紝鍙戦�佽鍐欏叆鐨勬暟鎹� */
	t->tx_buf = buf;			/* 瑕佸啓鍏ョ殑鏁版嵁 */
	t->len = len;				/* 鍐欏叆鐨勫瓧鑺傛暟 */
	spi_message_init(&m);		/* 鍒濆鍖杝pi_message */
	spi_message_add_tail(t, &m);/* 灏唖pi_transfer娣诲姞鍒皊pi_message闃熷垪 */
	ret = spi_sync(spi, &m);	/* 鍚屾鍙戦�� */

	kfree(t);					/* 閲婃斁鍐呭瓨 */
	s3c2440_spi_gpio_set_cs(1);/* 鐗囬�夋媺楂橈紝閲婃斁ICM20602 */
	return ret;
}

/*
 * @description	: 璇诲彇icm20602鎸囧畾瀵勫瓨鍣ㄥ�硷紝璇诲彇涓�涓瘎瀛樺櫒
 * @param - dev:  icm20602璁惧
 * @param - reg:  瑕佽鍙栫殑瀵勫瓨鍣�
 * @return 	  :   璇诲彇鍒扮殑瀵勫瓨鍣ㄥ��
 */
static unsigned char icm20602_read_onereg(struct icm20602_dev *dev, u8 reg)
{
	u8 data = 0;
	icm20602_read_regs(dev, reg, &data, 1);
	return data;
}

/*
 * @description	: 鍚慽cm20602鎸囧畾瀵勫瓨鍣ㄥ啓鍏ユ寚瀹氱殑鍊硷紝鍐欎竴涓瘎瀛樺櫒
 * @param - dev:  icm20602璁惧
 * @param - reg:  瑕佸啓鐨勫瘎瀛樺櫒
 * @param - data: 瑕佸啓鍏ョ殑鍊�
 * @return   :    鏃�
 */	

static void icm20602_write_onereg(struct icm20602_dev *dev, u8 reg, u8 value)
{
	u8 buf = value;
	icm20602_write_regs(dev, reg, &buf, 1);
}

/*
 * @description	: 璇诲彇ICM20602鐨勬暟鎹紝璇诲彇鍘熷鏁版嵁锛屽寘鎷笁杞撮檧铻轰华銆�
 * 				: 涓夎酱鍔犻�熷害璁″拰鍐呴儴娓╁害銆�
 * @param - dev	: ICM20602璁惧
 * @return 		: 鏃犮��
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
 * @description		: 鎵撳紑璁惧
 * @param - inode 	: 浼犻�掔粰椹卞姩鐨刬node
 * @param - filp 	: 璁惧鏂囦欢锛宖ile缁撴瀯浣撴湁涓彨鍋歱rivateate_data鐨勬垚鍛樺彉閲�
 * 					  涓�鑸湪open鐨勬椂鍊欏皢private_data浼兼湁鍚戣澶囩粨鏋勪綋銆�
 * @return 			: 0 鎴愬姛;鍏朵粬 澶辫触
 */
static int icm20602_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &icm20602dev; /* 璁剧疆绉佹湁鏁版嵁 */
	return 0;
}

/*
 * @description		: 浠庤澶囪鍙栨暟鎹� 
 * @param - filp 	: 瑕佹墦寮�鐨勮澶囨枃浠�(鏂囦欢鎻忚堪绗�)
 * @param - buf 	: 杩斿洖缁欑敤鎴风┖闂寸殑鏁版嵁缂撳啿鍖�
 * @param - cnt 	: 瑕佽鍙栫殑鏁版嵁闀垮害
 * @param - offt 	: 鐩稿浜庢枃浠堕鍦板潃鐨勫亸绉�
 * @return 			: 璇诲彇鐨勫瓧鑺傛暟锛屽鏋滀负璐熷�硷紝琛ㄧず璇诲彇澶辫触
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
 * @description		: 鍏抽棴/閲婃斁璁惧
 * @param - filp 	: 瑕佸叧闂殑璁惧鏂囦欢(鏂囦欢鎻忚堪绗�)
 * @return 			: 0 鎴愬姛;鍏朵粬 澶辫触
 */
static int icm20602_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* icm20602鎿嶄綔鍑芥暟 */
static const struct file_operations icm20602_ops = {
	.owner = THIS_MODULE,
	.open = icm20602_open,
	.read = icm20602_read,
	.release = icm20602_release,
};

/*
 * ICM20602鍐呴儴瀵勫瓨鍣ㄥ垵濮嬪寲鍑芥暟 
 * @param  	: 鏃�
 * @return 	: 鏃�
 */

void icm20602_reginit(void)
{
	u8 value = 0;

    dev_info(icm20602dev.device, "icm20602_reginit\r\n");
    icm20602_write_onereg(&icm20602dev, ICM20602_I2C_IF,     0x40);
    value = icm20602_read_onereg(&icm20602dev, ICM20602_WHO_AM_I);
    dev_info(icm20602dev.device, "ICM20602 ID = %#X\r\n", value); 

    icm20602_write_onereg(&icm20602dev, ICM20602_PWR_MGMT_1, 0x80);//复位设备
    mdelay(50);
    icm20602_write_onereg(&icm20602dev, ICM20602_PWR_MGMT_1,     0x01);            //时钟设置
    mdelay(50);
    icm20602_write_onereg(&icm20602dev, ICM20602_PWR_MGMT_2,     0x00);            //开启陀螺仪和加速度计
    icm20602_write_onereg(&icm20602dev, ICM20602_CONFIG,         0x01);            //176HZ 1KHZ
    icm20602_write_onereg(&icm20602dev, ICM20602_SMPLRT_DIV,     0x07);            //采样速率 SAMPLE_RATE = INTERNAL_SAMPLE_RATE / (1 + SMPLRT_DIV)
    icm20602_write_onereg(&icm20602dev, ICM20602_GYRO_CONFIG,    0x18);            //±2000 dps
    icm20602_write_onereg(&icm20602dev, ICM20602_ACCEL_CONFIG,   0x10);            //±8g
    icm20602_write_onereg(&icm20602dev, ICM20602_ACCEL_CONFIG_2, 0x03);            //Average 4 samples   44.8HZ   //0x23 Average 16 samples

    return;
}

 /*
  * @description     : spi椹卞姩鐨刾robe鍑芥暟锛屽綋椹卞姩涓�
  *                    璁惧鍖归厤浠ュ悗姝ゅ嚱鏁板氨浼氭墽琛�
  * @param - client  : spi璁惧
  * @param - id      : spi璁惧ID
  * 
  */	
static int icm20602_probe(struct spi_device *spi)
{
    unsigned int tmp_value = 0;
	/* 1銆佹瀯寤鸿澶囧彿 */
	if (icm20602dev.major) {
		icm20602dev.devid = MKDEV(icm20602dev.major, 0);
		register_chrdev_region(icm20602dev.devid, ICM20602_CNT, ICM20602_NAME);
	} else {
		alloc_chrdev_region(&icm20602dev.devid, 0, ICM20602_CNT, ICM20602_NAME);
		icm20602dev.major = MAJOR(icm20602dev.devid);
	}

	/* 2銆佹敞鍐岃澶� */
	cdev_init(&icm20602dev.cdev, &icm20602_ops);
	cdev_add(&icm20602dev.cdev, icm20602dev.devid, ICM20602_CNT);

	/* 3銆佸垱寤虹被 */
	icm20602dev.class = class_create(THIS_MODULE, ICM20602_NAME);
	if (IS_ERR(icm20602dev.class)) {
		return PTR_ERR(icm20602dev.class);
	}

	/* 4銆佸垱寤鸿澶� */
	icm20602dev.device = device_create(icm20602dev.class, NULL, icm20602dev.devid, NULL, ICM20602_NAME);
	if (IS_ERR(icm20602dev.device)) {
		return PTR_ERR(icm20602dev.device);
	}

	/*鍒濆鍖杝pi_device */
	spi->mode = SPI_MODE_0;	/*MODE0锛孋POL=0锛孋PHA=0*/
	spi_setup(spi);
	icm20602dev.private_data = spi; /* 璁剧疆绉佹湁鏁版嵁 */

    icm20602dev.spi_gpio_regs=ioremap(0x56000060, 0x10);

    tmp_value=*((volatile u32 *)(icm20602dev.spi_gpio_regs + GPGCON));
    tmp_value = tmp_value & (~(0x3 << 6));
    /*et gpg3 cs pin */
    tmp_value |= 0x1 << 6;
	*((volatile u32 *)(icm20602dev.spi_gpio_regs + GPGCON))=tmp_value;

	/* 鍒濆鍖朓CM20602鍐呴儴瀵勫瓨鍣� */
	icm20602_reginit();		

	return 0;
}

/*
 * @description     : spi椹卞姩鐨剅emove鍑芥暟锛岀Щ闄pi椹卞姩鐨勬椂鍊欐鍑芥暟浼氭墽琛�
 * @param - client 	: spi璁惧
 * @return          : 0锛屾垚鍔�;鍏朵粬璐熷��,澶辫触
 */
static int icm20602_remove(struct spi_device *spi)
{
	/* 鍒犻櫎璁惧 */
	cdev_del(&icm20602dev.cdev);
	unregister_chrdev_region(icm20602dev.devid, ICM20602_CNT);

	/* 娉ㄩ攢鎺夌被鍜岃澶� */
	device_destroy(icm20602dev.class, icm20602dev.devid);
	class_destroy(icm20602dev.class);
    if(icm20602dev.spi_gpio_regs) {
		iounmap(icm20602dev.spi_gpio_regs);
    }
	return 0;
}

/* 浼犵粺鍖归厤鏂瑰紡ID鍒楄〃 */
static const struct spi_device_id icm20602_id[] = {
	{"icm20602", 0},  
	{}
};

/* 璁惧鏍戝尮閰嶅垪琛� */
static const struct of_device_id icm20602_of_match[] = {
	{ .compatible = "icm20602" },
	{ /* Sentinel */ }
};

/* SPI椹卞姩缁撴瀯浣� */	
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
 * @description	: 椹卞姩鍏ュ彛鍑芥暟
 * @param 		: 鏃�
 * @return 		: 鏃�
 */
static int __init icm20602_init(void)
{
	return spi_register_driver(&icm20602_driver);
}

/*
 * @description	: 椹卞姩鍑哄彛鍑芥暟
 * @param 		: 鏃�
 * @return 		: 鏃�
 */
static void __exit icm20602_exit(void)
{
	spi_unregister_driver(&icm20602_driver);
}

module_init(icm20602_init);
module_exit(icm20602_exit);
MODULE_LICENSE("GPL");

