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
	dev_t devid;			/* è®¾å¤‡å· 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* ç±» 		*/
	struct device *device;	/* è®¾å¤‡ 	 */
	struct device_node	*nd; /* è®¾å¤‡èŠ‚ç‚¹ */
	int major;			/* ä¸»è®¾å¤‡å· */
	void *private_data;	/* ç§æœ‰æ•°æ® */
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
 * @description	: ä»ap3216cè¯»å–å¤šä¸ªå¯„å­˜å™¨æ•°æ®
 * @param - dev:  ap3216cè®¾å¤‡
 * @param - reg:  è¦è¯»å–çš„å¯„å­˜å™¨é¦–åœ°å€
 * @param - val:  è¯»å–åˆ°çš„æ•°æ®
 * @param - len:  è¦è¯»å–çš„æ•°æ®é•¿åº¦
 * @return 		: æ“ä½œç»“æœ
 */
static int icm20602_read_regs(struct icm20602_dev *dev, u8 reg, void *val, int len)
{
	int ret;
	struct i2c_msg msg[2];
	struct i2c_client *client = (struct i2c_client *)dev->private_data;

	/* msg[0]ä¸ºå‘é€è¦è¯»å–çš„é¦–åœ°å€ */
	msg[0].addr = client->addr;			/* ap3216cåœ°å€ */
	msg[0].flags = 0;					/* æ ‡è®°ä¸ºå‘é€æ•°æ® */
	msg[0].buf = &reg;					/* è¯»å–çš„é¦–åœ°å€ */
	msg[0].len = 1;						/* regé•¿åº¦*/

	/* msg[1]è¯»å–æ•°æ® */
	msg[1].addr = client->addr;			/* ap3216cåœ°å€ */
	msg[1].flags = I2C_M_RD;			/* æ ‡è®°ä¸ºè¯»å–æ•°æ®*/
	msg[1].buf = val;					/* è¯»å–æ•°æ®ç¼“å†²åŒº */
	msg[1].len = len;					/* è¦è¯»å–çš„æ•°æ®é•¿åº¦*/

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
 * @description	: å‘icm20602å¤šä¸ªå¯„å­˜å™¨å†™å…¥æ•°æ®
 * @param - dev:  icm20602è®¾å¤‡
 * @param - reg:  è¦å†™å…¥çš„å¯„å­˜å™¨é¦–åœ°å€
 * @param - val:  è¦å†™å…¥çš„æ•°æ®ç¼“å†²åŒº
 * @param - len:  è¦å†™å…¥çš„æ•°æ®é•¿åº¦
 * @return 	  :   æ“ä½œç»“æœ
 */
static s32 icm20602_write_regs(struct icm20602_dev *dev, u8 reg, u8 *buf, u8 len)
{
	u8 b[256] = {0};
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	
	b[0] = reg;					/* å¯„å­˜å™¨é¦–åœ°å€ */
	memcpy(&b[1],buf,len);		/* å°†è¦å†™å…¥çš„æ•°æ®æ‹·è´åˆ°æ•°ç»„bé‡Œé¢ */
		
	msg.addr = client->addr;	/* icm20602åœ°å€ */
	msg.flags = 0;				/* æ ‡è®°ä¸ºå†™æ•°æ® */

	msg.buf = b;				/* è¦å†™å…¥çš„æ•°æ®ç¼“å†²åŒº */
	msg.len = len + 1;			/* è¦å†™å…¥çš„æ•°æ®é•¿åº¦ */

	return i2c_transfer(client->adapter, &msg, 1);
}

/*
 * @description	: è¯»å–icm20602æŒ‡å®šå¯„å­˜å™¨å€¼ï¼Œè¯»å–ä¸€ä¸ªå¯„å­˜å™¨
 * @param - dev:  icm20602è®¾å¤‡
 * @param - reg:  è¦è¯»å–çš„å¯„å­˜å™¨
 * @return 	  :   è¯»å–åˆ°çš„å¯„å­˜å™¨å€¼
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
 * @description	: å‘ap3216cæŒ‡å®šå¯„å­˜å™¨å†™å…¥æŒ‡å®šçš„å€¼ï¼Œå†™ä¸€ä¸ªå¯„å­˜å™¨
 * @param - dev:  ap3216cè®¾å¤‡
 * @param - reg:  è¦å†™çš„å¯„å­˜å™¨
 * @param - data: è¦å†™å…¥çš„å€¼
 * @return   :    æ— 
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
//  @brief      »ñÈ¡ICM20602ÍÓÂİÒÇÊı¾İ
//  @param      NULL
//  @return     void
//  @since      v1.0
//  Sample usage:				Ö´ĞĞ¸Ãº¯Êıºó£¬Ö±½Ó²é¿´¶ÔÓ¦µÄ±äÁ¿¼´¿É
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
 * @description	: è¯»å–AP3216Cçš„æ•°æ®ï¼Œè¯»å–åŸå§‹æ•°æ®ï¼ŒåŒ…æ‹¬ALS,PSå’ŒIR, æ³¨æ„ï¼
 *				: å¦‚æœåŒæ—¶æ‰“å¼€ALS,IR+PSçš„è¯ä¸¤æ¬¡æ•°æ®è¯»å–çš„æ—¶é—´é—´éš”è¦å¤§äº112.5ms
 * @param - ir	: iræ•°æ®
 * @param - ps 	: psæ•°æ®
 * @param - ps 	: alsæ•°æ® 
 * @return 		: æ— ã€‚
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
    while(0x12 != data_1) //¶ÁÈ¡ICM20602 ID
    {
		
		data_1 = icm20602_read_reg(&icm20602dev, ICM20602_WHO_AM_I);
        //¿¨ÔÚÕâÀïÔ­ÒòÓĞÒÔÏÂ¼¸µã
        //1 MPU6050»µÁË£¬Èç¹ûÊÇĞÂµÄÕâÑùµÄ¸ÅÂÊ¼«µÍ
        //2 ½ÓÏß´íÎó»òÕßÃ»ÓĞ½ÓºÃ
        //3 ¿ÉÄÜÄãĞèÒªÍâ½ÓÉÏÀ­µç×è£¬ÉÏÀ­µ½3.3V
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
    icm20602_write_reg(&icm20602dev,ICM20602_SMPLRT_DIV,0x07);               //²ÉÑùËÙÂÊ SAMPLE_RATE = INTERNAL_SAMPLE_RATE / (1 + SMPLRT_DIV)
    icm20602_write_reg(&icm20602dev,ICM20602_GYRO_CONFIG,0x18);              //¡À2000 dps
    icm20602_write_reg(&icm20602dev,ICM20602_ACCEL_CONFIG,0x10);             //¡À8g
    icm20602_write_reg(&icm20602dev,ICM20602_ACCEL_CONFIG_2,0x03);           //Average 4 samples   44.8HZ   //0x23 Average 16 samples
	//ICM20602_GYRO_CONFIG¼Ä´æÆ÷
    //ÉèÖÃÎª:0x00 ÍÓÂİÒÇÁ¿³ÌÎª:¡À250 dps     »ñÈ¡µ½µÄÍÓÂİÒÇÊı¾İ³ıÒÔ131           ¿ÉÒÔ×ª»¯Îª´øÎïÀíµ¥Î»µÄÊı¾İ£¬ µ¥Î»Îª£º¡ã/s
    //ÉèÖÃÎª:0x08 ÍÓÂİÒÇÁ¿³ÌÎª:¡À500 dps     »ñÈ¡µ½µÄÍÓÂİÒÇÊı¾İ³ıÒÔ65.5          ¿ÉÒÔ×ª»¯Îª´øÎïÀíµ¥Î»µÄÊı¾İ£¬µ¥Î»Îª£º¡ã/s
    //ÉèÖÃÎª:0x10 ÍÓÂİÒÇÁ¿³ÌÎª:¡À1000dps     »ñÈ¡µ½µÄÍÓÂİÒÇÊı¾İ³ıÒÔ32.8          ¿ÉÒÔ×ª»¯Îª´øÎïÀíµ¥Î»µÄÊı¾İ£¬µ¥Î»Îª£º¡ã/s
    //ÉèÖÃÎª:0x18 ÍÓÂİÒÇÁ¿³ÌÎª:¡À2000dps     »ñÈ¡µ½µÄÍÓÂİÒÇÊı¾İ³ıÒÔ16.4          ¿ÉÒÔ×ª»¯Îª´øÎïÀíµ¥Î»µÄÊı¾İ£¬µ¥Î»Îª£º¡ã/s

    //ICM20602_ACCEL_CONFIG¼Ä´æÆ÷
    //ÉèÖÃÎª:0x00 ¼ÓËÙ¶È¼ÆÁ¿³ÌÎª:¡À2g          »ñÈ¡µ½µÄ¼ÓËÙ¶È¼ÆÊı¾İ ³ıÒÔ16384      ¿ÉÒÔ×ª»¯Îª´øÎïÀíµ¥Î»µÄÊı¾İ£¬µ¥Î»£ºg(m/s^2)
    //ÉèÖÃÎª:0x08 ¼ÓËÙ¶È¼ÆÁ¿³ÌÎª:¡À4g          »ñÈ¡µ½µÄ¼ÓËÙ¶È¼ÆÊı¾İ ³ıÒÔ8192       ¿ÉÒÔ×ª»¯Îª´øÎïÀíµ¥Î»µÄÊı¾İ£¬µ¥Î»£ºg(m/s^2)
    //ÉèÖÃÎª:0x10 ¼ÓËÙ¶È¼ÆÁ¿³ÌÎª:¡À8g          »ñÈ¡µ½µÄ¼ÓËÙ¶È¼ÆÊı¾İ ³ıÒÔ4096       ¿ÉÒÔ×ª»¯Îª´øÎïÀíµ¥Î»µÄÊı¾İ£¬µ¥Î»£ºg(m/s^2)
    //ÉèÖÃÎª:0x18 ¼ÓËÙ¶È¼ÆÁ¿³ÌÎª:¡À16g         »ñÈ¡µ½µÄ¼ÓËÙ¶È¼ÆÊı¾İ ³ıÒÔ2048       ¿ÉÒÔ×ª»¯Îª´øÎïÀíµ¥Î»µÄÊı¾İ£¬µ¥Î»£ºg(m/s^2)
    dev_info(icm20602dev.device, "icm20602_initial done\n");

    return;
}

/*
 * @description		: æ‰“å¼€è®¾å¤‡
 * @param - inode 	: ä¼ é€’ç»™é©±åŠ¨çš„inode
 * @param - filp 	: è®¾å¤‡æ–‡ä»¶ï¼Œfileç»“æ„ä½“æœ‰ä¸ªå«åšprivate_dataçš„æˆå‘˜å˜é‡
 * 					  ä¸€èˆ¬åœ¨opençš„æ—¶å€™å°†private_dataæŒ‡å‘è®¾å¤‡ç»“æ„ä½“ã€‚
 * @return 			: 0 æˆåŠŸ;å…¶ä»– å¤±è´¥
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
 * @description		: ä»è®¾å¤‡è¯»å–æ•°æ® 
 * @param - filp 	: è¦æ‰“å¼€çš„è®¾å¤‡æ–‡ä»¶(æ–‡ä»¶æè¿°ç¬¦)
 * @param - buf 	: è¿”å›ç»™ç”¨æˆ·ç©ºé—´çš„æ•°æ®ç¼“å†²åŒº
 * @param - cnt 	: è¦è¯»å–çš„æ•°æ®é•¿åº¦
 * @param - offt 	: ç›¸å¯¹äºæ–‡ä»¶é¦–åœ°å€çš„åç§»
 * @return 			: è¯»å–çš„å­—èŠ‚æ•°ï¼Œå¦‚æœä¸ºè´Ÿå€¼ï¼Œè¡¨ç¤ºè¯»å–å¤±è´¥
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
 * @description		: å…³é—­/é‡Šæ”¾è®¾å¤‡
 * @param - filp 	: è¦å…³é—­çš„è®¾å¤‡æ–‡ä»¶(æ–‡ä»¶æè¿°ç¬¦)
 * @return 			: 0 æˆåŠŸ;å…¶ä»– å¤±è´¥
 */
static int icm20602_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* AP3216Cæ“ä½œå‡½æ•° */
static const struct file_operations icm20602_ops = {
	.owner = THIS_MODULE,
	.open = icm20602_open,
	.read = icm20602_read,
	.release = icm20602_release,
};

 /*
  * @description     : i2cé©±åŠ¨çš„probeå‡½æ•°ï¼Œå½“é©±åŠ¨ä¸
  *                    è®¾å¤‡åŒ¹é…ä»¥åæ­¤å‡½æ•°å°±ä¼šæ‰§è¡Œ
  * @param - client  : i2cè®¾å¤‡
  * @param - id      : i2cè®¾å¤‡ID
  * @return          : 0ï¼ŒæˆåŠŸ;å…¶ä»–è´Ÿå€¼,å¤±è´¥
  */
static int icm20602_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

    dev_info(&client->dev, "icm20602_probe\n");
	/* 1ã€æ„å»ºè®¾å¤‡å· */
	if (icm20602dev.major) {
		icm20602dev.devid = MKDEV(icm20602dev.major, 0);
		register_chrdev_region(icm20602dev.devid, ICM20602_CNT, ICM20602_NAME);
	} else {
		alloc_chrdev_region(&icm20602dev.devid, 0, ICM20602_CNT, ICM20602_NAME);
		icm20602dev.major = MAJOR(icm20602dev.devid);
	}

	/* 2ã€æ³¨å†Œè®¾å¤‡ */
	cdev_init(&icm20602dev.cdev, &icm20602_ops);
	cdev_add(&icm20602dev.cdev, icm20602dev.devid, ICM20602_CNT);

	/* 3ã€åˆ›å»ºç±» */
	icm20602dev.class = class_create(THIS_MODULE, ICM20602_NAME);
	if (IS_ERR(icm20602dev.class)) {
		return PTR_ERR(icm20602dev.class);
	}

	/* 4ã€åˆ›å»ºè®¾å¤‡ */
	icm20602dev.device = device_create(icm20602dev.class, NULL, icm20602dev.devid, NULL, ICM20602_NAME);
	if (IS_ERR(icm20602dev.device)) {
		return PTR_ERR(icm20602dev.device);
	}

	icm20602dev.private_data = client;

	return 0;
}

/*
 * @description     : i2cé©±åŠ¨çš„removeå‡½æ•°ï¼Œç§»é™¤i2cé©±åŠ¨çš„æ—¶å€™æ­¤å‡½æ•°ä¼šæ‰§è¡Œ
 * @param - client 	: i2cè®¾å¤‡
 * @return          : 0ï¼ŒæˆåŠŸ;å…¶ä»–è´Ÿå€¼,å¤±è´¥
 */
static int icm20602_remove(struct i2c_client *client)
{
	/* åˆ é™¤è®¾å¤‡ */
	cdev_del(&icm20602dev.cdev);
	unregister_chrdev_region(icm20602dev.devid, ICM20602_CNT);

	/* æ³¨é”€æ‰ç±»å’Œè®¾å¤‡ */
	device_destroy(icm20602dev.class, icm20602dev.devid);
	class_destroy(icm20602dev.class);
	return 0;
}

/* ä¼ ç»ŸåŒ¹é…æ–¹å¼IDåˆ—è¡¨ */
static const struct i2c_device_id icm20602_id[] = {
	{"icm20602", 0},  
	{}
};

/* è®¾å¤‡æ ‘åŒ¹é…åˆ—è¡¨ */
static const struct of_device_id icm20602_of_match[] = {
	{ .compatible = "icm20602" },
	{ /* Sentinel */ }
};

/* i2cé©±åŠ¨ç»“æ„ä½“ */	
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
 * @description	: é©±åŠ¨å…¥å£å‡½æ•°
 * @param 		: æ— 
 * @return 		: æ— 
 */
static int __init icm20602_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&icm20602_driver);
	return ret;
}

/*
 * @description	: é©±åŠ¨å‡ºå£å‡½æ•°
 * @param 		: æ— 
 * @return 		: æ— 
 */
static void __exit icm20602_exit(void)
{
	i2c_del_driver(&icm20602_driver);
}

/* module_i2c_driver(ap3216c_driver) */

module_init(icm20602_init);
module_exit(icm20602_exit);
MODULE_LICENSE("GPL");


