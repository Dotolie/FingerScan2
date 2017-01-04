#ifndef __FPGA_H__
#define __FPGA_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	SPI_DEV1 = 0,
	SPI_DEV2 = 1,
	SPI_DEV3 = 2,
	SPI_DEV4 = 3,
	SPI_DEV5 = 4,
};

enum {
	FPGA_CMD_STANBY = 0x0,
 	FPGA_CMD_RUN = 0x1,
};

#define DIR_INPUT	0
#define DIR_OUTPUT	1
#define SINGLE_ADC	0
#define DIFF_ADC	1

#define GPIO_NR(bank, nr)		(((bank) - 1) * 32 + (nr))

#define AD_CMD0				GPIO_NR(3, 22)		//EIM_D22
#define AD_CMD1				GPIO_NR(3, 21)		//EIM_D21
#define AD_CMD2				GPIO_NR(3, 20)		//EIM_D20

#define FPGA_INTB			GPIO_NR(4, 28)		//DISP0_DAT7

#define FPGA_DONE			GPIO_NR(5, 2)		//EIM_A25
#define PROG_B				GPIO_NR(3, 17)		//EIM_A20 -> EIM_D17   81

#define FPGASPI_READ				_IO('S', 1)
#define FPGASPI_WRITE				_IO('S', 2)
#define FPGASPI_GET_DATA			_IO('S', 3)
#define FPGASPI_GET_DATA2			_IO('S', 4)

int fpga_GpioSetDirection(int gpio, int dir);
int fpga_GpioGetValue(int gpio);
int fpga_GpioSetValue(int gpio, int value);
int fpga_Setcmd(unsigned char cmd);
int fpga_SingleInit(int freq, int high_resolution);
int fpga_taskStart(int freq);
int fpga_taskStop(void);


#ifdef __cplusplus
}
#endif

#endif
