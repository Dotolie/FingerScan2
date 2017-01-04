#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "fingerscan.h"
#include "fpga.h"
#include "flash.h"

//#define FILE_SAVE

static int gFpgaDev = -1;
static int fpgathread_Exit = 0;
static int gFreq = 0;

#ifdef FILE_SAVE
FILE *g_fp = NULL;
#endif

int fpga_GpioSetDirection(int gpio, int dir)
{
	int fd;
	char buf[255];

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
	
	fd = open(buf, O_WRONLY);
	if (fd < 0)
		return -1;

	if (dir == DIR_OUTPUT)
		write(fd, "out", 3);
	else if (dir == DIR_INPUT)
		write(fd, "in", 2); 
	
	close(fd);

	return 0;
}

int fpga_GpioGetValue(int gpio)
{
	int fd, fd2, val;
	char value;
	char buf[255] = {0}; 
	
	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
	
	fd2 = open(buf, O_WRONLY);
	if (fd2 < 0)
		return -1;

	write(fd2, "in", 2); 
	
	close(fd2);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	
	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return -1;

	read(fd, &value, 1);
	
	if(value == '0') { 
		val = 0;
	} else if(value == '1'){
		val = 1;
	} else
		return -1;
	
	close(fd);

	return val;
}

int fpga_GpioSetValue(int gpio, int value)
{
	int fd, fd2, fd3;
	char buf[255]; 

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd < 0) {
		LOGE("fail export %d", gpio);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", gpio); 

	write(fd, buf, strlen(buf));

	close(fd);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);

	fd2 = open(buf, O_WRONLY);
	if (fd2 < 0) {
		LOGE("fail direction %d", gpio);
		return -1;
	}

	write(fd2, "out", 3);

	close(fd2);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);

	fd3 = open(buf, O_WRONLY);
	if (fd3 < 0) {
		LOGE("fail value %d", gpio);
		return -1;
	}

	if (value)
		write(fd3, "1", 1);
	else
		write(fd3, "0", 1); 

	close(fd3);

	return 0;
}

int fpga_Setcmd(unsigned char cmd)
{
	int ret;
	ret = fpga_GpioSetValue(AD_CMD0, cmd & 0x01);
	if (ret)
		return -1;
	ret = fpga_GpioSetValue(AD_CMD1, cmd & 0x02);
	if (ret)
		return -1;
	ret = fpga_GpioSetValue(AD_CMD2, cmd & 0x04);
	if (ret)
		return -1;
	return 0;
}

int fpga_hardwareEnable (int on)
{
	int ret;

	//Enable : 1, Disable : 0
	ret = fpga_GpioSetValue(PROG_B, on);
	if (ret < 0) {
		LOGE("fpga_GpioSetValue fail\n");
		return -1;
	}

	sleep(2);
	return 0;
}

int fpga_dev_open (void)
{
	int err;
	
	if (gFpgaDev < 0) {
		if ((gFpgaDev = open("/dev/spi", O_RDWR)) < 0) {
			LOGE("gFpgaDev_open fail\n");
			return -1;
		}
	}
	return 0;
}

void fpga_dev_close (void)
{
	if (gFpgaDev >= 0) {
		close (gFpgaDev);
		gFpgaDev = -1;
	}
}

unsigned char fpga_data[80000] = {0};
unsigned char fpga_data2[80000] = {0};

int fpga_GetData(void)
{
	struct spi_info sit;
	int i = 0, j, ret, cnt = 0;
	static int first = 1;
	sit.channel = 0;	//SPI_DEV1
	sit.inbuff =  0;
	sit.outbuff = fpga_data;
	
	memset(fpga_data, 0, 80000);

	//200ms get data
	if (gFpgaDev > 0) {
		if ((ret = ioctl(gFpgaDev, FPGASPI_GET_DATA2, &sit)) < 0) {
			LOGE("spi_get_data fail %d\n", ret);
			return -1;
		}
	}

	//for (i = 0; i < 8; i++) {
	//	if (fpga_data[i] != 0x0)
	//		LOGD("[%d][0x%02x][0x%02x]", i, fpga_data[i], fpga_data[i + 1]);
	//}
		

	//LOGD("[0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]", 
	//	fpga_data[0], fpga_data[1], fpga_data[2], fpga_data[3], fpga_data[4], fpga_data[5]);

	memcpy(fpga_data2, fpga_data, 80000);

	if (first) {
		first = 0;
		return 0;
	}
#ifdef FILE_SAVE
	if (g_fp) {
		cnt = gFreq * 20;
		

		fprintf(g_fp, "=====================================================================================\r\n"); 
	}
#endif
	return 0;
}

void* fpga_thread(void* arg)
{
	int ret;
	LOGD("fpga_thread start freq %d", gFreq);
	
#ifdef FILE_SAVE
	g_fp = fopen("/mnt/sdcard/sem.txt", "wt");
	if (!g_fp) {
		LOGE("fpga_thread file open fail");
		return NULL;
	}
#endif

	//fpga init
	ret = fpga_dev_open();
	if (ret < 0) {
		LOGE("fpga dev open fail");
		return NULL;
	}

	//fpga stop
	fpga_Setcmd(FPGA_CMD_RUN);
	
	while (!fpgathread_Exit) {		
		fpga_GetData();
		NotifyFromNative(fpga_data2, 80000);

		usleep(1000 * 300);
	}

#ifdef FILE_SAVE
	if (g_fp)
		fclose(g_fp);
	g_fp = NULL;
#endif
	LOGD("fpga_thread exit");
	fpga_dev_close();
	pthread_exit(NULL);
	return NULL;
}

static pthread_t fpga_pthread = 0;

int fpga_taskStart(int freq)
{
	int ret;

	if (fpgathread_Exit)
		return -1;
	
	gFreq = freq;
	fpgathread_Exit = 0;
	ret = pthread_create(&fpga_pthread, NULL, fpga_thread, NULL);
	if (ret) {
		LOGE("pthread create failed");
		return -1;
	}

	return 0;
}

int fpga_taskStop(void)
{
	int ret;
	
	LOGD("fpga_taskStop");
	
	fpga_Setcmd(FPGA_CMD_STANBY);

	fpgathread_Exit = 1;
	if (fpga_pthread)
		pthread_join(fpga_pthread, NULL);
	fpga_pthread = 0;
	fpgathread_Exit = 0;
	LOGD("fpga_taskStop success");

	return 0;
}

