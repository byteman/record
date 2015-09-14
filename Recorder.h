/********************************************************************
	created:	2012/04/24
	created:	24:4:2012   16:29
	filename: 	scs-sdk\include\sdk.h
	file path:	scs-sdk\include
	file base:	sdk
	file ext:	h
	author:		 
	
	purpose:	
*********************************************************************/

#ifndef CLOUDWALK_SDK_H
#define CLOUDWALK_SDK_H

#include <windows.h>

#ifdef COULDWALKRECORDER_EXPORTS
#define CLOUDWALKFACESDK_API __declspec(dllexport)
#else
#define CLOUDWALKFACESDK_API __declspec(dllimport)
#endif



#ifdef _WIN32
#define SDK_CallMode WINAPI
#define SDK_CallBack CALLBACK
#else
#define SDK_CallMode
#define SDK_CallBack
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
录像模块返回错误码
*/
enum SDK_RECORD_ERR{
	ERR_RECORD_OK=0, //成功.
	ERR_RECORD_DSHOW_OPEN=900, //DirectShow打开失败
	ERR_RECORD_VIDEO_OPEN, //视频设备打开失败
	ERR_RECORD_AUDIO_OPEN, //音频设备打开失败
	ERR_RECORD_NOT_OPEN_DEVS,	//未打开音视频设备,录像启动失败.
	ERR_RECORD_OPEN_FILE,	//创建录像文件失败.
	ERR_RECORD_OPEN_FILTER,
};

/**
功能: 列出本机支持的所有摄像头和音频采集设备列表
参数描述：
	DevType[in]: 获取设备类型 0:摄像头 1:麦克风
	devCount[in/out]: 返回设备的个数
	返回值:
	返回GBK编码的描述设备名称的字符串数组，数组的个数为devCount的个数。

*/

CLOUDWALKFACESDK_API  char** SDK_CallMode CloudWalk_ListDevices(int  devType, int* devCount);


/**
视频回调接口
参数描述：
	rgb24： rgb24格式的一帧图像缓冲数据
	width:  图像的宽度
	height:  图像的高度  

*/
typedef  int (*Video_Callback)(unsigned char* rgb24, int width, int height);

/**
功能: 打开音视频设备，注册回调函数
参数描述：
	pVideoDevice: 视频设备名
	pAudioDevice: 音频设备名
	width:图像的宽度
	height:图像的高度
	FrameRate：摄像头采集的帧率
	FrameBitRate: 压缩后的视频码率
	sampleRateInHz：音频采样频率
	channelConfig：音频通道数
	_callback: 采集到一帧视频后回调该函数，传递RGB24格式的视频数据。回调函数内部应该尽快返回，不要阻塞，否则会影响到视频采集。
	返回值:
	///0-成功，非0  为错误代码

*/
CLOUDWALKFACESDK_API  int  SDK_CallMode   CloudWalk_OpenDevices(
													const char* pVideoDevice,
													const char* pAudioDevice,
													const  unsigned  int width,
													const unsigned  int height,
													const unsigned  int FrameRate,
													int sampleRateInHz,
													int channelConfig,
													Video_Callback video_callback);

/**
功能: 启动录像
参数描述：
filePath： 保存的文件路径和文件名
pText: 视频叠加的文字
返回值:
///0-成功，非0  为错误代码

*/

/*
录像的音视频参数.
*/
enum VideoEncType
{
	ENC_H264=0,
	ENC_MPEG4
};
typedef struct _VideoInfo
{
	int width,height;	//视频的高度和宽度,如果没有按16字节对齐，内部会强制对齐.
	int fps;		//视频的帧率
	int bitrate;	//视频的码率

}VideoInfo;

typedef struct _AudioInfo
{
	int channle; //音频通道数
	int bitrate; //编码的码率
}AudioInfo;
#define MAX_SUBTITLE_SIZE 128
typedef struct _SubTitleInfo
{
	int  x,y; // 字幕的位置
	const char *text; //字幕的内容
	const char *fontname; //字体名称. [simfang.ttf]
	const char *fontcolor; //字体颜色[red,white,black]
	int  fontsize; //字体的大小.
}SubTitleInfo;
typedef enum{

}REC_ERR;


CLOUDWALKFACESDK_API  int  SDK_CallMode CloudWalk_RecordStart (const char* filePath,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle);


/**
功能: 停止录像(停止录像后，保存到文件中，关闭录像文件)

参数描述：
返回值:
///0-成功，非0  为错误代码

*/
CLOUDWALKFACESDK_API  int  SDK_CallMode CloudWalk_RecordStop (void);

/**
功能: 关闭设备,关闭设备后，释放所有资源。


参数描述：
返回值:
///0-成功，非0  为错误代码

*/

CLOUDWALKFACESDK_API  int  SDK_CallMode CloudWalk_CloseDevices(void);


CLOUDWALKFACESDK_API  int  SDK_CallMode CloudWalk_Muxing(int argc, char **argv);
#ifdef __cplusplus
};
#endif


#endif  // SCSSDK_SDK_H

