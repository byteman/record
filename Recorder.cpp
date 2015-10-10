
#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef	__cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/audio_fifo.h"
#include "libswresample/swresample.h"
#include "libavfilter/avfiltergraph.h" 
#include "libavfilter/buffersink.h"  
#include "libavfilter/buffersrc.h" 

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "swscale.lib")
#ifdef __cplusplus
};
#endif
#include "Recorder.h"
#include "CaptureDevices.h"
#include "Utils.h"
#include "RateCtrl.h"
#include "VideoCapture.h"
#include "AudioCapture.h"
#include "RecordMux.h"
static int FPS = 25;
RecordMux recMux;

int init_ffmpeg_env(HMODULE handle)
{
	
	string path = GetProgramDir(handle);
	string log_cfg = path+"\\record.ini";
	string log_out = path+"\\"+get_log_filename();
	av_register_all();
	avdevice_register_all();
	avfilter_register_all();

	init_report_file(log_cfg,log_out);
	return 0;
}

int  CloseDevices()
{
	//先停止录像
	CloudWalk_RecordStop();
	//再停止采集线程
	recMux.Close();

	return 0;
}
int  SDK_CallMode CloudWalk_CloseDevices(void)
{
	return 	CloseDevices();
}
/*
停止录像，写入剩余的录像数据.
*/
int  SDK_CallMode CloudWalk_RecordStop (void)
{
	av_log(NULL,AV_LOG_DEBUG, "CloudWalk_RecordStop\r\n");
	//停止录像，挂起音频采集线程，然后禁止推送音视频数据到录像队列.

	recMux.Stop();
	return ERR_RECORD_OK;
}

/*
启动录像，指定存储目录，指定视频叠加的内容.
这个接口可能会反复调用，考虑反复调用的情况
*/
CLOUDWALKFACESDK_API  int  SDK_CallMode CloudWalk_RecordStart (const char* filePath,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle)
{
	av_log(NULL,AV_LOG_DEBUG, "CloudWalk_RecordStart\r\n");
	
	pVideoInfo->width  =  FFALIGN(pVideoInfo->width,  16);
	pVideoInfo->height =  FFALIGN(pVideoInfo->height, 16);
	
	recMux.Start(filePath,pVideoInfo,pAudioInfo,pSubTitle);
	//设置完输出文件的参数后，启动录制
	return ERR_RECORD_OK;
}


int  SDK_CallMode   CloudWalk_OpenDevices(
													const char* pVideoDevice,
													const char* pVideoDevice2,
													const char* pAudioDevice,
													const unsigned  int width,
													const unsigned  int height,
													const unsigned  int FrameRate,
													int sampleRateInHz,
													int channelConfig,
													Video_Callback video_callback)
{

	int ret = 0;
	FPS = FrameRate;
	av_log(NULL,AV_LOG_ERROR,"CloudWalk_OpenDevices vidoe=%s,audio=%s width=%d height=%d framerate=%d\r\n",\
			pVideoDevice,pAudioDevice,width,height,FrameRate);
	AVInputFormat *pDShowInputFmt = av_find_input_format("dshow");
	if(pDShowInputFmt == NULL)
	{
		av_log(NULL,AV_LOG_ERROR,"dshow init failed\r\n");
		return ERR_RECORD_DSHOW_OPEN;
	}
	ret = recMux.OpenCamera(getDevicePath("video",pVideoDevice).c_str(),getDevicePath("video",pVideoDevice2).c_str(),width,height,FrameRate,AV_PIX_FMT_RGB24, video_callback);
	if(ret != ERR_RECORD_OK)
	{
		av_log(NULL,AV_LOG_ERROR,"OpenCamera failed\r\n");
		return ret;
	}
	ret = recMux.OpenAudio(getDevicePath("audio",pAudioDevice).c_str());
	if(ret != ERR_RECORD_OK)
	{
		av_log(NULL,AV_LOG_ERROR,"OpenAudio failed\r\n");
		return ret;
	}

	return 0;

}

//不能再dll卸载的时候调用此函数，否则会导致程序无法退出，摄像头没法再次启用.
void FreeAllRes()
{
	//直接卸载dll，导致没有调用关闭采集线程，但是因为线程在这里已经被强制杀掉了，所以没有执行到audioThreadQuit=1

	CloseDevices();
}
#define MAX_DEVICES_NUM 10
#define MAX_DEVICES_NAME_SIZE 128
static char pStrDevices[MAX_DEVICES_NUM][MAX_DEVICES_NAME_SIZE]={{}};
static char* pStrings[MAX_DEVICES_NUM];

bool resetDevciesString(int num)
{
	for(int i = 0; i < num; i++)
	{
		memset(pStrDevices[i],0 , sizeof(MAX_DEVICES_NAME_SIZE));
	} 
	return true;
}
/*
列出本机上所有的音视频设备
*/
char** SDK_CallMode CloudWalk_ListDevices(int  devType, int* devCount)
{
	CaptureDevices *capDev = new CaptureDevices();
	vector<wstring> videoDevices, audioDevices;

	//这里获取到的字符串就是以GBK编码的宽字符.
	capDev->GetVideoDevices(&videoDevices);
	capDev->GetAudioDevices(&audioDevices);

	delete capDev;
	
	for(int i = 0;  i < MAX_DEVICES_NUM;i++)
	{
		pStrings[i] = &pStrDevices[i][0];
	}
	resetDevciesString(MAX_DEVICES_NUM);
	if(devType == 0)
	{
		for(int i = 0; i < videoDevices.size(); i++)
		{
			std::string str = ws2s(videoDevices[i]);
			strcpy(pStrDevices[i],str.c_str());
			av_log(NULL,AV_LOG_INFO,"video[%d]=%s\r\n",i+1,str.c_str());
		}
		*devCount =  videoDevices.size();
	}
	else
	{
		for(int i = 0; i < audioDevices.size(); i++)
		{
			std::string str = ws2s(audioDevices[i]);
			strcpy(pStrDevices[i],str.c_str());
			av_log(NULL,AV_LOG_INFO,"audio[%d]=%s\r\n",i+1,str.c_str());
		}
		*devCount =  audioDevices.size();
	}
	return pStrings;

}

extern int muxing_func(int argc, char **argv);
int  SDK_CallMode CloudWalk_Muxing(int argc, char **argv)
{
	return muxing_func(argc, argv);
}