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
#include "libavutil/pixdesc.h"


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
#include "CaptureDevices.h"
#include "Utils.h"


int VideoCap::OpenVideoCapture(AVFormatContext** pFmtCtx, AVCodecContext	** pCodecCtx,const char* psDevName,int index,AVInputFormat *ifmt,const unsigned  int width,
													const unsigned  int height,
													unsigned  int &FrameRate,const char* fmt)
{
	int fps = 0;
	int idx = 0;

	AVCodec* pCodec = NULL;
	std::string dshow_path = getDevicePath("video",psDevName);
	//dshow_dump_params(pFmtCtx, dshow_path.c_str(),ifmt);
	//dshow_dump_devices(pFmtCtx,dshow_path.c_str(),ifmt);
	CaptureDevices cap_devs;
	cap_devs.ListCapablities(psDevName,0);
	My_Info info;
	info.pixel_format = AV_PIX_FMT_YUYV422;
	info.fps = FrameRate;
	info.height = height;
	info.width  = width;

	fps = FrameRate;
	if(!cap_devs.GetBestCap(psDevName,AV_PIX_FMT_NONE,width,height,FrameRate,info))
	{
		//return -6;
	}

	fps = dshow_try_open_devices(pFmtCtx, dshow_path.c_str(),index, ifmt,info.pixel_format,info.width, info.height, info.fps);
	
	
	if(fps == 0) 
	{
		return -1;
	}
	AVDictionary *options = NULL;
	//av_dict_set_int(&options,"analyzeduration",10000000,0);
	//av_dict_set_int(&options,"probesize",1000000,0);
	//analyzeduration
	if(avformat_find_stream_info(*pFmtCtx,NULL)<0)
	{
		av_log(NULL,AV_LOG_ERROR,"Couldn't find stream information.（无法获取视频流信息）\n");
		return -2;
	}
	VideoIndex = find_stream_index(*pFmtCtx,AVMEDIA_TYPE_VIDEO);
		
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
	FrameRate = fps;
	return 0;
}

bool VideoCap::SetCallBackAttr(int width, int height, AVPixelFormat format,Video_Callback pCbFunc)
{
	if(sws_ctx!=NULL)
	{
		sws_freeContext(sws_ctx);
		sws_ctx = NULL;
	}
	if(pCodecContext->pix_fmt==-1)
	{
		pCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
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
	bCapture = true;
	AVFrame	*pFrame; //存放从摄像头解码后得到的一帧数据.这个数据应该就是YUYV422，高度和宽度是预览的高度和宽度.
	evt_ready.set();
	pFrame = av_frame_alloc();//分配一个帧，用于解码输入视频流.
	
	//分配一个Frame用作存放RGB24格式的预览视频.
	AVFrame* pPreviewFrame = alloc_picture(AV_PIX_FMT_BGR24,pCodecContext->width, pCodecContext->height,16);

	av_init_packet(&packet);
	av_log(NULL,AV_LOG_DEBUG,"camera[%d] start capture thread\r\n ",channel);
	while(bCapture)
	{
		packet.data = NULL;
		packet.size = 0;
		
		//从摄像头读取一 包数据，该数据未解码
		if (av_read_frame(pFormatContext, &packet) < 0)
		{
			continue;
		}
		if(packet.stream_index == VideoIndex)
		{
			//解码视频流 
			if (avcodec_decode_video2(pCodecContext, pFrame, &got_picture, &packet) < 0)
			{
				av_log(NULL,AV_LOG_ERROR,"Camera[%d] Decode Error.\n",channel);
				continue;
			}
			if (got_picture)
			{
				
				//转换到RGB24

				if(pCallback)
				{
					//回调视频数据.

					sws_scale(sws_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, 
							pCodecContext->height, pPreviewFrame->data, pPreviewFrame->linesize);
					pCallback(channel,pPreviewFrame->data[0], pCodecContext->width ,pCodecContext->height);
					if( (pCodecContext->width!=640) || (pCodecContext->height!=480))
					{
						av_log(NULL,AV_LOG_INFO,"width=%d,height=%d\r\n",pCodecContext->width,pCodecContext->height);
					}

				}

				if(bStartRecord && fifo_video) //如果启动了录像，才推送视频数据到录像队列.
				{
					//转换视频帧为录像视频格式 YUYV422->YUV420P
					//picture is yuv420 data.

					sws_scale(record_sws_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, 
							pFrame->height, pRecFrame->data, pRecFrame->linesize);

					if (av_fifo_space(fifo_video) >= nSampleSize)
					{
						EnterCriticalSection(&section);	
					
						av_fifo_generic_write(fifo_video, pRecFrame->data[0], y_size , NULL);
						av_fifo_generic_write(fifo_video, pRecFrame->data[1], y_size/4, NULL);
						av_fifo_generic_write(fifo_video, pRecFrame->data[2], y_size/4, NULL);
			
						LeaveCriticalSection(&section);
	
					}
				}
				
			}
		}
		//释放采集到的一包数据.
		av_free_packet(&packet);
		
	}
	av_log(NULL,AV_LOG_DEBUG,"camera[%d] stop capture thread\r\n ",channel);
	if(pFrame)
		av_frame_free(&pFrame);

	if(pPreviewFrame)
		av_frame_free(&pPreviewFrame);
	
	if(pFormatContext)
		avformat_close_input(&pFormatContext);
	pFormatContext = NULL;

	if(fifo_video)
	{
		av_fifo_free(fifo_video);
		fifo_video = 0;
	}
	evt_quit.set();
	bQuit = 1;

	
}

int VideoCap::OpenPreview(const char* psDevName,int index,AVInputFormat *ifmt,const unsigned  int width,
													const unsigned  int height,
													unsigned  int &FrameRate,AVPixelFormat cap_format,AVPixelFormat format, Video_Callback pCbFunc)
{
	int ret = 0;
	bQuit	 = false;
	bCapture = true;
	pCallback = pCbFunc;
	const char* name = av_get_pix_fmt_name(cap_format);
	ret = OpenVideoCapture(&pFormatContext,&pCodecContext,psDevName,index,ifmt,width,height,FrameRate,name);
	if(ret != ERR_RECORD_OK) 
	{
		return ERR_RECORD_VIDEO_OPEN;
	}
	if(!SetCallBackAttr(width,height,format,pCbFunc))
	{
		av_log(NULL,AV_LOG_ERROR,"SetCallBackAttr failed\r\n");
		return ERR_RECORD_VIDEO_OPEN;
	}
	
	return ret;
}
int VideoCap::Start()
{
	HANDLE handle = CreateThread( NULL, 0, VideoCapThreadProc, this, 0, NULL);
	if(handle == NULL)
	{
		av_log(NULL,AV_LOG_ERROR,"CreateThread VideoCapThreadProc failed\r\n");
		return ERR_RECORD_VIDEO_OPEN;
	}
	//evt_ready.wait(1000);
	if(!evt_ready.wait(1000))return ERR_RECORD_VIDEO_OPEN;

	return ERR_RECORD_OK;
}
int VideoCap::Stop()
{
	return ERR_RECORD_OK;
}
VideoCap::VideoCap(int chan):
	channel(chan)
{
	pFormatContext = NULL;
	pCodecContext = NULL;
	bCapture = true;
	VideoIndex = 0;
	bQuit = false;
	bStartRecord = false;
	pCallback = NULL;
	fifo_video = NULL;
	sws_ctx = NULL;
	InitializeCriticalSection(&section);
}
int VideoCap::Close()
{
	bCapture = false;
	if(!evt_quit.wait(1000))return ERR_RECORD_VIDEO_OPEN;
	//evt_quit.wait(1000);
	return ERR_RECORD_OK;
	//等待线程结束.
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
AVFrame* VideoCap::GetLastSample()
{
	AVFrame* frame = NULL;
	if(fifo_video == NULL) return NULL;
	frame = GetSample();
	if(frame != NULL) return frame;

	return pRecFrame2;
}
AVFrame* VideoCap::GetSample()
{
	if(fifo_video == NULL) return NULL;
	if(av_fifo_size(fifo_video) < nSampleSize) return NULL;

	EnterCriticalSection(&section);
	av_fifo_generic_read(fifo_video, pRecFrame2->data[0], y_size, NULL); //从队列中读取一帧YUV420P的数据帧,帧的大小预先算出.
	av_fifo_generic_read(fifo_video, pRecFrame2->data[1], y_size/4, NULL); 
	av_fifo_generic_read(fifo_video, pRecFrame2->data[2], y_size/4, NULL); 
	LeaveCriticalSection(&section);

	/*
				
	pEncFrame->format = pFmtContext->streams[VideoIndex]->codec->pix_fmt;
	pEncFrame->width  = pFmtContext->streams[VideoIndex]->codec->width;
	pEncFrame->height =	pFmtContext->streams[VideoIndex]->codec->height;
	*/
	return pRecFrame2;
}
int VideoCap::StartRecord(AVPixelFormat format, int width, int height)
{
	_fmt = format;
	_width = width;
	_height = height;
	 
	record_sws_ctx = sws_getContext(pCodecContext->width, pCodecContext->height,  pCodecContext->pix_fmt, 
		width, height, _fmt, SWS_POINT, NULL, NULL, NULL); 

	if(record_sws_ctx == NULL)
	{
		return -1;
	}

	nSampleSize = avpicture_get_size(format, width, height);
	//申请30帧目标帧大小做video的fifo
	fifo_video = av_fifo_alloc(30 * nSampleSize);
	if(fifo_video == NULL)
	{
		av_log(NULL,AV_LOG_ERROR,"alloc pic fifo failed\r\n");
		return -8;
	}
	
	pRecFrame = alloc_picture(format,width,height,16);//AV_PIX_FMT_YUV420P
	pRecFrame2= alloc_picture(format,width,height,16);//AV_PIX_FMT_YUV420P

	y_size = width*height;
	bStartRecord = true;

	return 0;
}
int VideoCap::StopRecord()
{
	bStartRecord = false;
	return 0;
}
