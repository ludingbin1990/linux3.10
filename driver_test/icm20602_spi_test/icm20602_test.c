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
	unsigned short data[7];
    unsigned short icm_gyro_x;
	unsigned short icm_gyro_y;
	unsigned short icm_gyro_z;
    unsigned short icm_temp;
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
            icm_temp = data[6];
            printf("icm_gyro_x = %u, icm_gyro_y = %u, icm_gyro_z = %u\r\n", icm_gyro_x, icm_gyro_y, icm_gyro_z);
            printf("icm_acc_x = %u, icm_acc_y = %u, icm_acc_z = %u\r\n", icm_acc_x, icm_acc_y, icm_acc_z);
            printf("icm_temp = %u\r\n", icm_temp);
		}
		usleep(200000);
	}

	close(fd);
	return 0;
}

