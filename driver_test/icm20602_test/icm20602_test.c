#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>


int main(int argc, char *argv[])
{
	int fd;
	int err = 0;
	char filename[]= "/dev/ICM20602";
	unsigned short data[6];
    unsigned short icm_gyro_x;
	unsigned short icm_gyro_y;
	unsigned short icm_gyro_z;
    unsigned short icm_acc_x;
    unsigned short icm_acc_y;
    unsigned short icm_acc_z;
    float unit_icm_gyro_x;
    float unit_icm_gyro_y;
    float unit_icm_gyro_z;
    float unit_icm_acc_x;
    float unit_icm_acc_y;
    float unit_icm_acc_z;
    int count = 10;
	
	fd = open(filename, O_RDWR);
	if (fd < 0) {
		printf("Can't open file %s\r\n", filename);
		return -1;
	}
    printf("Enter test\r\n");
	//als:环境光强度	ps:接近距离		ir:红外线强度
	while(count--){
		err = read(fd, data, sizeof(data));
		if(err == 0){
			icm_gyro_x = data[0];
			icm_gyro_y = data[1];
			icm_gyro_z = data[2];
            icm_acc_x = data[3];
            icm_acc_y = data[4];
            icm_acc_z = data[5];
			unit_icm_gyro_x = (float)icm_gyro_x/131;
            unit_icm_gyro_y = (float)icm_gyro_y/131;
            unit_icm_gyro_z = (float)icm_gyro_z/131;

            unit_icm_acc_x = (float)icm_acc_x/4096;
            unit_icm_acc_y = (float)icm_acc_y/4096;
            unit_icm_acc_z = (float)icm_acc_z/4096;
            printf("unit_icm_gyro_x = %f, unit_icm_gyro_y = %f, unit_icm_gyro_z = %f\r\n", unit_icm_gyro_x, unit_icm_gyro_y, unit_icm_gyro_z);
            printf("unit_icm_acc_x = %f, unit_icm_acc_y = %f, unit_icm_acc_z = %f\r\n", unit_icm_acc_x, unit_icm_acc_y, unit_icm_acc_z);
		}
		usleep(200000);
	}

	close(fd);
	return 0;
}

