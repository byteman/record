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
#include "VideoCapture.h"
#include "Utils.h"

SwsContext *rgb24_convert_ctx= NULL;   //将源格式转换为RGB24格式

int VideoCap::OpenVideoCapture(AVFormatContext** pFmtCtx, AVCodecContext	** pCodecCtx,const char* psDevName,AVInputFormat *ifmt,const unsigned  int width,
													const unsigned  int height,
													const unsigned  int FrameRate)
{
	int fps = 0;
	int idx = 0;
	AVCodec* pCodec = NULL;
	dshow_dump_params(pFmtCtx,psDevName,ifmt);
	dshow_dump_devices(pFmtCtx,psDevName,ifmt);
	fps = dshow_try_open_devices(pFmtCtx, psDevName, ifmt,width, height, FrameRate);
	if(fps == 0) 
	{
		return -1;
	}
	if(avformat_find_stream_info(*pFmtCtx,NULL)<0)
	{
		av_log(NULL,AV_LOG_ERROR,"Couldn't find stream information.（无法获取视频流信息）\n");
		return -2;
	}
	VideoIndex = -1;
	for(int i = 0; i < (*pFmtCtx)->nb_streams; i++)
	{
		if ((*pFmtCtx)->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			VideoIndex = i;
			break;
		}
	}
	
	if(VideoIndex == -1)
	{
		av_log(NULL,AV_LOG_ERROR,"Couldn't find video stream information.（无法获取视频流信息）\n");
		return -3;
	}
	*pCodecCtx = (*pFmtCtx)->streams[VideoIndex]->codec;
	//这里的pCodecCtx_Video->codec_id是什么时候初始化的? 在dshow.c中的dshow_add_device中设置了 codec->codec_id = AV_CODEC_ID_RAWVIDEO;

	pCodec = avcodec_find_decoder((*pCodecCtx)->codec_id);
	if(pCodec == NULL)
	{
		av_log(NULL,AV_LOG_ERROR,"Codec not found.（没有找到解码器）\n");
		return -4;
	}
	if(avcodec_open2((*pCodecCtx), pCodec, NULL) < 0)
	{
		av_log(NULL,AV_LOG_ERROR,"Could not open codec.（无法打开解码器）\n");
		return -5;
	}
	if( ((*pCodecCtx)->width != 640) || ((*pCodecCtx)->height != 480))
	{
		av_log(NULL,AV_LOG_ERROR,"width=%d,height=%d fmt=%d\r\n",(*pCodecCtx)->width,(*pCodecCtx)->height,(*pCodecCtx)->pix_fmt);
	}
	
	return 0;
}

bool VideoCap::SetCallBackAttr(int width, int height, AVPixelFormat format,Video_Callback pCbFunc)
{
	if(sws_ctx!=NULL)
	{
		sws_freeContext(sws_ctx);
		sws_ctx = NULL;
	}
	//设置需要转换的目标格式为BGR24, 尺寸就是预览图像的大小.
	sws_ctx = sws_getContext(pCodecContext->width, pCodecContext->height, pCodecContext->pix_fmt, 
		width, height, format, SWS_BICUBIC, NULL, NULL, NULL);

	return (sws_ctx!=NULL);
}
DWORD WINAPI VideoCapThreadProc( LPVOID lpParam )
{
	if(lpParam != NULL)
	{
		VideoCap* vc = (VideoCap*)lpParam;
		vc->Run();
	}
	return 0;
}
/*
该线程在设备被打开后，就被创建，一直到设备被关闭后，线程退出。
在录像停止后，该线程仅仅采集视频并回调，但是并不推送数据到视频队列中.
*/
void VideoCap::Run( )
{
	AVPacket packet;
	int got_picture = 0;
	AVFrame	*pFrame; //存放从摄像头解码后得到的一帧数据.这个数据应该就是YUYV422，高度和宽度是预览的高度和宽度.

	pFrame = av_frame_alloc();//分配一个帧，用于解码输入视频流.
	
	AVFrame* pPreviewFrame = av_frame_alloc(); //分配一个帧，用于存放预览视频数据,目标是RGB24

	int nRGB24size = avpicture_get_size(AV_PIX_FMT_RGB24, 
		pCodecContext->width, pCodecContext->height);

	uint8_t* video_cap_buf = new uint8_t[nRGB24size];

	avpicture_fill((AVPicture *)pPreviewFrame, video_cap_buf, 
		AV_PIX_FMT_RGB24, 
		pCodecContext->width, 
		pCodecContext->height);


	av_init_packet(&packet);
	
	while(bCapture)
	{
		packet.data = NULL;
		packet.size = 0;
		
		//从摄像头读取一 包数据，该数据未解码
		if (av_read_frame(pFormatContext, &packet) < 0)
		{
			continue;
		}
		if(packet.stream_index == 0)
		{
			//解码视频流 
			if (avcodec_decode_video2(pCodecContext, pFrame, &got_picture, &packet) < 0)
			{
				av_log(NULL,AV_LOG_ERROR,"Decode Error.\n");
				continue;
			}
			if (got_picture)
			{
				
				//转换到RGB24

				if(pCallback)
				{
					//回调视频数据.

					sws_scale(rgb24_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, 
							pCodecContext->height, pPreviewFrame->data, pPreviewFrame->linesize);
					pCallback(pPreviewFrame->data[0], pCodecContext->width ,pCodecContext->height);
					if( (pCodecContext->width!=640) || (pCodecContext->height!=480))
					{
						av_log(NULL,AV_LOG_INFO,"width=%d,height=%d\r\n",pCodecContext->width,pCodecContext->height);
					}

				}
	
				

				
			}
		}
		//释放采集到的一包数据.
		av_free_packet(&packet);
		
	}
	if(pFrame)
	av_frame_free(&pFrame);

	if(pPreviewFrame)
	av_frame_free(&pPreviewFrame);
	
	if(video_cap_buf)
	{
		delete []video_cap_buf;
	}
	bQuit = 1;
	
	av_log(NULL,AV_LOG_INFO,"video thread exit\r\n");
	
}

int VideoCap::OpenPreview(const char* psDevName,AVInputFormat *ifmt,const unsigned  int width,
													const unsigned  int height,
													const unsigned  int FrameRate,AVPixelFormat format, Video_Callback pCbFunc)
{
	int ret = 0;
	bQuit	 = false;
	bCapture = true;
	ret = OpenVideoCapture(&pFormatContext,&pCodecContext,psDevName,ifmt,width,height,FrameRate);
	SetCallBackAttr(width,height,format,pCbFunc);
	CreateThread( NULL, 0, VideoCapThreadProc, this, 0, NULL);

	return ret;
}

VideoCap::VideoCap()
{
	pFormatContext = NULL;
	pCodecContext = NULL;
	bCapture = false;
	VideoIndex = 0;
	bQuit = false;
	pCallback = NULL;

	sws_ctx = NULL;
}
int VideoCap::Close()
{
	bCapture = 1;
	return 0;
}
AVRational VideoCap::GetVideoTimeBase()
{
	return pFormatContext->streams[VideoIndex]->codec->time_base;
}
AVRational VideoCap::Get_aspect_ratio()
{
	return pFormatContext->streams[VideoIndex]->codec->sample_aspect_ratio;
}
int VideoCap::GetWidth()
{
	return pCodecContext->width;
}
int VideoCap::GetHeight()
{
	return pCodecContext->height;
}
AVPixelFormat VideoCap::GetFormat()
{
	return pCodecContext->pix_fmt;
}