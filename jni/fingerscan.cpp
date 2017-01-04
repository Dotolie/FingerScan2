//mvtech 2016. 09. 29
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <termio.h>

#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <jni.h>

#include "fingerscan.h"
#include "flash.h"
#include "fpga.h"

static int gDev = -1;
static int thread_Exit = 0;
static unsigned int irq_cnt = 0;
static pthread_t fingerscan_pthread;
unsigned char adc_data[1280  * 2] = {0};
unsigned char capture_buffer[512  * 6] = {0};

JavaVM * g_vm;
jobject g_obj;
jmethodID g_mid;

jmethodID	g_jpost_eventId = NULL;

extern unsigned char fpga_data2[80000];

int spi_dev_open (void)
{
	int err;
	
	if (gDev < 0) {
		if ((gDev = open("/dev/spi", O_RDWR)) < 0) {
			LOGE("spi_open fail\n");
			return -1;
		}

		//if ((err=ioctl(gDev, IOCTL_SPI_INFORM_SIG, NULL)) < 0) {
		//	LOGE("SPI_INFORM_SIG error\n");
		//	return -1;
		//}
	}
	return 0;
}

void spi_dev_close (void)
{
	if (gDev >= 0) {
		close (gDev);
		gDev = -1;
	}
}

unsigned short spi_regread(int channel, unsigned char reg)
{
	struct spi_info sit;
	unsigned char rReg[2];
	unsigned char rData[2];
	int ret;
	memset(&sit, 0, sizeof(sit));
	memset(rReg, 0, sizeof(rReg));
	memset(rData, 0, sizeof(rData));
	
	rReg[0] = reg;

	sit.channel = channel;
	sit.inbuff = rReg;
	sit.outbuff = rData;
	sit.size = 2;

	if (gDev >= 0) {
		ret = ioctl(gDev, IOCTL_SPI_READ, &sit);
		if (ret) {
			LOGE("SPI_REGREAD error\n");
			return -1;
		}
	}

	//LOGD("reg(0x%02x) 0 0x%02x\n", reg, rData[0]);
	//LOGD("reg(0x%02x) 1 0x%02x\n", reg, rData[1]);

	return (rData[0]<<8) | rData[1];
}

int spi_regwrite(int channel, unsigned char reg, unsigned short data)
{
	int ret;
	struct spi_info sit;
	unsigned char wReg[4];

	memset(&sit, 0, sizeof(sit));
	memset(wReg, 0, sizeof(wReg));
	
	wReg[0] = reg;
	wReg[1] = (data>>8)&0xff;
	wReg[2] = data&0xff;

	sit.channel = channel;
	sit.inbuff = wReg;
	sit.outbuff = NULL;
	sit.size = 3;

	if (gDev >= 0) {
		ret = ioctl(gDev, IOCTL_SPI_WRITE, &sit);
		if (ret) {
			LOGE("SPI_WRITE error\n");
			return -1;
		}
	} else
		LOGE("SPI_WRITE error\n");

	return 0;
}

int spi_regbytewrite(int channel, unsigned char reg, unsigned char data)
{
	int ret;
	struct spi_info sit;
	unsigned char wReg[4];

	memset(&sit, 0, sizeof(sit));
	memset(wReg, 0, sizeof(wReg));
	
	wReg[0] = reg;
	wReg[1] = data&0xff;

	sit.channel = channel;
	sit.inbuff = wReg;
	sit.outbuff = NULL;
	sit.size = 2;

	if (gDev >= 0) {
		ret = ioctl(gDev, IOCTL_SPI_BYTE_WRITE, &sit);
		if (ret) {
			LOGE("SPI_WRITE error\n");
			return -1;
		}
	} else
		LOGE("SPI_WRITE error\n");

	return 0;
}

int spi_get_data(void)
{
	struct spi_info sit;
	int i, ret, cnt = 0;

	sit.channel = 0;
	sit.inbuff =  0;
	sit.outbuff = adc_data;

	if (gDev >= 0) {
		if ((ret = ioctl(gDev, IOCTL_SPI_GET_DATA, &sit)) < 0) {
			LOGE("spi_get_data fail %d\n", ret);
			return -1;
		}
	}
#if 0
	LOGD("spi size = %d", sit.size);
	for (i = 0; i < 10; i++) {
		
		cnt *= 5;
		LOGD("[0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]", 
				adc_data[cnt], adc_data[cnt + 1], adc_data[cnt + 2], adc_data[cnt + 3], adc_data[cnt + 4]);
	}
#endif
	return 0;
}

int spi_manual_time_capture(void)
{
	struct spi_info sit;
	int i, ret, cnt = 0;

	sit.channel = 0;
	sit.inbuff =  0;
	sit.outbuff = capture_buffer;

	if (gDev >= 0) {
		if ((ret = ioctl(gDev, IOCTL_SPI_MANUAL_CAPTURE, &sit)) < 0) {
			LOGE("IOCTL_SPI_MANUAL_CAPTURE fail %d\n", ret);
			return -1;
		}
	}
#if 1
	LOGD("spi size = %d", sit.size);
	cnt = 0;
	LOGD("%d [0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]", cnt,
			capture_buffer[cnt], capture_buffer[cnt + 1], 
			capture_buffer[cnt + 2], capture_buffer[cnt + 3], 
			capture_buffer[cnt + 4], capture_buffer[cnt + 5],
			capture_buffer[cnt + 6], capture_buffer[cnt + 7], 
			capture_buffer[cnt + 8], capture_buffer[cnt + 9], 
			capture_buffer[cnt + 10], capture_buffer[cnt + 11]);
	cnt = 1024;
	LOGD("%d [0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]",  cnt,
		capture_buffer[cnt], capture_buffer[cnt + 1], 
		capture_buffer[cnt + 2], capture_buffer[cnt + 3], 
		capture_buffer[cnt + 4], capture_buffer[cnt + 5],
		capture_buffer[cnt + 6], capture_buffer[cnt + 7], 
		capture_buffer[cnt + 8], capture_buffer[cnt + 9], 
		capture_buffer[cnt + 10], capture_buffer[cnt + 11]);
	cnt = 2048;
	LOGD("%d [0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]",  cnt,
		capture_buffer[cnt], capture_buffer[cnt + 1], 
		capture_buffer[cnt + 2], capture_buffer[cnt + 3], 
		capture_buffer[cnt + 4], capture_buffer[cnt + 5],
		capture_buffer[cnt + 6], capture_buffer[cnt + 7], 
		capture_buffer[cnt + 8], capture_buffer[cnt + 9], 
		capture_buffer[cnt + 10], capture_buffer[cnt + 11]);
#endif
	return 0;
}

char *jbyteArray2cstr( JNIEnv *env, jbyteArray javaBytes )
{
	size_t len = (size_t)env->GetArrayLength(javaBytes);
	jbyte *nativeBytes = env->GetByteArrayElements(javaBytes, 0);
	char *nativeStr = (char *)malloc(len+1);
	
	strncpy( nativeStr, (char *)nativeBytes, len );
	nativeStr[len] = '\0';
	env->ReleaseByteArrayElements(javaBytes, nativeBytes, JNI_ABORT);
	
	return nativeStr;
} 

void NotifyFromNative(unsigned char *buff, int buff_size)
{		
	jstring str;
	jbyteArray retArray;
	JNIEnv * env;
	int isAttached = 0;
	char data[] = {'a','b','3','4','5'};

	if (g_vm) {
		// double check it's all ok
		int getEnvStat = g_vm->GetEnv((void **)&env, JNI_VERSION_1_6);
		if (getEnvStat == JNI_EDETACHED) {
			if (g_vm->AttachCurrentThread(&env, NULL) != 0) {			
				LOGD("Failed to attach");
			} else
				isAttached = 1;
		} else if (getEnvStat == JNI_OK) {
			//
		} else if (getEnvStat == JNI_EVERSION) {
			//std::cout << "GetEnv: version not supported" << std::endl;
			LOGD("GetEnv: version not supported");
		}		

		retArray = env->NewByteArray(buff_size);
		env->SetByteArrayRegion(retArray, 0, buff_size, (const jbyte *)buff);
		env->CallVoidMethod(g_obj, g_mid, retArray);
		env->ReleaseByteArrayElements(retArray, (jbyte*) buff, JNI_ABORT);
		env->DeleteLocalRef(retArray);

		if (env->ExceptionCheck()) {
			env->ExceptionDescribe();
		}

		if(isAttached) {
			g_vm->DetachCurrentThread();
		}
	}
}

void* fingerscan_thread(void* arg)
{
	LOGD("fingerscan_thread start");
	
	while (!thread_Exit) {		
		spi_get_data();
		NotifyFromNative(adc_data, 1280);

		usleep(10);
	}

	LOGD("fingerscan_thread exit");
	pthread_exit(NULL);
	return NULL;
}

JNIEXPORT void
Java_com_example_fingerscan_MainActivity_GpioSetDirection( JNIEnv* env,
                                                  jobject thiz, jstring port, jboolean dir )
{
	int gpio;
	const char *strPort = env->GetStringUTFChars(port, NULL);

	gpio = atoi(strPort);
	if (dir) {
		//LOGD("GpioSetDirection %d value %d", gpio, DIR_OUTPUT);
		fpga_GpioSetDirection(gpio, DIR_OUTPUT);
	} else {
		//LOGD("GpioSetDirection %d value %d", gpio, DIR_INPUT);
		fpga_GpioSetDirection(gpio, DIR_INPUT);
	}
}

JNIEXPORT void
Java_com_example_fingerscan_MainActivity_GpioSetValue( JNIEnv* env,
                                                  jobject thiz, jstring port, jboolean value )
{
	int gpio;
	const char *strPort = env->GetStringUTFChars(port, NULL);

	gpio = atoi(strPort);
	if (value) {
		//LOGD("fpga_GpioSetValue %d value %d", gpio, 1);
		fpga_GpioSetValue(gpio, 1);
	} else {
		//LOGD("fpga_GpioSetValue %d value %d", gpio, 0);
		fpga_GpioSetValue(gpio, 0);
	}
}

JNIEXPORT jboolean
Java_com_example_fingerscan_MainActivity_GpioGetValue( JNIEnv* env,
                                                  jobject thiz, jstring port )
{
	int gpio;
	const char *utf8 = env->GetStringUTFChars(port, NULL);

	gpio = atoi(utf8);

	//LOGD("fpga_GpioGetValue %d value %d", gpio, fpga_GpioGetValue(gpio));
	return fpga_GpioGetValue(gpio);
}

JNIEXPORT int
Java_com_example_fingerscan_MainActivity_fpgaStart( JNIEnv* env,
                                                  jobject thiz, jint freq )
{
	int ret;
	ret = fpga_taskStart(freq);

	return ret;
}

JNIEXPORT int
Java_com_example_fingerscan_MainActivity_fpgaStop( JNIEnv* env,
                                                  jobject thiz )
{
	return fpga_taskStop();
}

JNIEXPORT int
Java_com_example_fingerscan_MainActivity_flashRead( JNIEnv* env,
                                                  jobject thiz, jstring fname )
{
	int ret;
	pthread_t pthread;
	const char *utf8 = env->GetStringUTFChars(fname, NULL);
	
	ret = pthread_create(&pthread, NULL, flashRead, (void *)utf8);
	if (ret) {
		LOGE("pthread create failed");
		return -1;
	}

	return 0;
}

JNIEXPORT int
Java_com_example_fingerscan_MainActivity_flashUpdate( JNIEnv* env,
                                                  jobject thiz, jstring fname )
{
	int ret;
	pthread_t pthread;
	const char *utf8 = env->GetStringUTFChars(fname, NULL);

	ret = pthread_create(&pthread, NULL, flashUpdate, (void *)utf8);
	if (ret) {
		LOGE("pthread create failed");
		return -1;
	}

	return 0;
}

JNIEXPORT void
Java_com_example_fingerscan_MainActivity_DeviceClose( JNIEnv* env,
                                                  jobject thiz )
{
	spi_dev_close();
	thread_Exit = 1;
	if (fingerscan_pthread)
		pthread_join(fingerscan_pthread, NULL);

	fpga_taskStop();
	LOGD("DeviceClose");
}

JNIEXPORT jbyteArray
Java_com_example_fingerscan_MainActivity_GetData( JNIEnv* env,
                                                  jobject thiz )
{
	int finger_size = 80000;
	int sizeByteUrl = 5;

	jbyteArray data = env->NewByteArray(finger_size);
	if (data == NULL) {
		return NULL; //  out of memory error thrown
	}

	jbyte *bytes = env->GetByteArrayElements(data, 0);
	memcpy(bytes, fpga_data2, finger_size);
	env->SetByteArrayRegion(data, 0, finger_size, bytes);
	
    return data;
}

JNIEXPORT void JNICALL Java_com_example_fingerscan_MainActivity_CallBackInit(JNIEnv * env, jobject obj)
{		
	LOGD("CallBackInit");
	int status;

	bool returnValue = true;
	// convert local to global reference 
			// (local will die after this method call)
	g_obj = env->NewGlobalRef(obj);

	// save refs for callback
	jclass g_clazz = env->GetObjectClass(g_obj);
	if (g_clazz == NULL) {
		LOGE("Failed to find class");
	}

	g_mid = env->GetMethodID(g_clazz, "Callback", "([B)V");
	if (g_mid == NULL) {
		LOGE("Unable to get method ref");
	}

		
	LOGD("CallBackInit SUCCESS");
	//return (jboolean)returnValue;
	return;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{	
	LOGD("JNI_OnLoad start");	

	JNIEnv	*env;
	g_vm = vm;

	LOGD("JNI_OnLoad end"); 

	return JNI_VERSION_1_6;
}

void  JNI_OnUnload(JavaVM *jvm, void *reserved)
{
	LOGD("JNI_OnUnload start");	

	JNIEnv *env;
	if (jvm->GetEnv((void **)&env, JNI_VERSION_1_6)) {
		return;
	}

	LOGD("JNI_OnUnload end");	

	return;
}

#ifdef __cplusplus
}
#endif

