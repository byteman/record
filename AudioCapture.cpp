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
#include <Windows.h>

#include "AudioCapture.h"
#include "Utils.h"

int OpenAudioCapture(AVFormatContext** pFmtCtx, const char * psDevName, AVInputFormat *ifmt)
{

	//以Direct Show的方式打开设备，并将 输入方式 关联到格式上下文
	//char * psDevName = dup_wchar_to_utf8(L"audio=麦克风 (Realtek High Definition Au");
	//char * psDevName = dup_wchar_to_utf8(L"audio=麦克风 (2- USB Audio Device)");
	std::string dshow_path = getDevicePath("audio",psDevName);
	if (avformat_open_input(pFmtCtx, dshow_path.c_str(), ifmt,NULL) < 0)
	{
		av_log(NULL,AV_LOG_ERROR,"Couldn't open input stream.（无法打开音频输入流）\n");
		return -1;
	}

	if(avformat_find_stream_info(*pFmtCtx,NULL)<0)  
		return -2; 
	
	if((*pFmtCtx)->streams[0]->codec->codec_type != AVMEDIA_TYPE_AUDIO)
	{
		av_log(NULL,AV_LOG_ERROR,"Couldn't find video stream information.（无法获取音频流信息）\n");
		return -3;
	}

	AVCodec *tmpCodec = avcodec_find_decoder((*pFmtCtx)->streams[0]->codec->codec_id);
	if(0 > avcodec_open2((*pFmtCtx)->streams[0]->codec, tmpCodec, NULL))
	{
		av_log(NULL,AV_LOG_ERROR,"can not find or open audio decoder!\n");
		return -4;
	}
	
	

	return 0;
}

DWORD WINAPI AudioCapThreadProc( LPVOID lpParam )
{
	if(lpParam != NULL)
	{
		AudioCap* vc = (AudioCap*)lpParam;
		vc->Run();
	}
	return 0;
}

AudioCap::AudioCap()
{
	pFormatContext = NULL;
	pCodecContext= NULL;
	bCapture= false;
	bQuit=false;
	bStartRecord = false;
	fifo_audio = NULL;
	InitializeCriticalSection(&section);
	sws_ctx=NULL;
	pAudioFrame = NULL;
	audio_swr_ctx = NULL;

}
int AudioCap::Open(const char * psDevName, AVInputFormat *ifmt)
{
	int ret = 0;
	if(bCapture) 
	{
		//摄像头已经被打开了.
		return 0;
	}
	bQuit	 = false;
	
	ret = OpenAudioCapture(&pFormatContext,psDevName,ifmt);
	if(ret != 0) return ret;
	CreateThread( NULL, 0, AudioCapThreadProc, this, 0, NULL);
	//evt_ready.wait(10000);
	return ret;
}

int AudioCap::Close()
{
	if(bCapture==false)
		return 0;
	bCapture = false;
	
	evt_quit.wait(1000);
	//等待线程结束.
	return 0;
}
AVCodecContext* AudioCap::GetCodecContext()
{
	return pFormatContext->streams[0]->codec;
}


AVFrame* AudioCap::GrabFrame()
{
	AVPacket packet;
	static AVFrame	*pFrame = NULL;
	int gotframe = 0;
	if(pFrame == NULL)
	{
		pFrame = av_frame_alloc();
	}
	while(1)
	{
		av_init_packet(&packet);
		if (av_read_frame(pFormatContext, &packet) < 0)
		{
			return NULL;
		}
		//解码成指定的格式到pkt  fixme
		if (avcodec_decode_audio4(pFormatContext->streams[0]->codec, pFrame, &gotframe, &packet) < 0)
		{
			//解码失败后，退出了线程，这里需要修复
			av_frame_free(&pFrame);
			av_log(NULL,AV_LOG_ERROR,"can not decoder a frame");
			//break;
			return NULL;
		}
		av_free_packet(&packet);
		if (!gotframe)
		{
			continue;//没有获取到数据，继续下一次
		}
		break;
	}
	return pFrame;
}
void AudioCap::Run( )
{
	AVPacket pkt;
	AVFrame *frame;
	bCapture = true;
	evt_ready.set();
	
	frame = av_frame_alloc();
	int gotframe;
	
	while(bCapture)// 退出标志
	{
		pkt.data = NULL;
		pkt.size = 0;
		
		//从音频设备中读取一帧音频数据
		if(av_read_frame(pFormatContext,&pkt) < 0)
		{
			continue;
		}
		//解码成指定的格式到pkt  fixme
		if (avcodec_decode_audio4(pFormatContext->streams[0]->codec, frame, &gotframe, &pkt) < 0)
		{
			//解码失败后，退出了线程，这里需要修复
			av_frame_free(&frame);
			av_log(NULL,AV_LOG_ERROR,"can not decoder a frame");
			//break;
		}
		av_free_packet(&pkt);

		if (!gotframe)
		{
			continue;//没有获取到数据，继续下一次
		}
		//分配audio的fifo.
		if (NULL == fifo_audio)
		{
			fifo_audio = av_audio_fifo_alloc2(pFormatContext->streams[0]->codec->sample_fmt, 
				pFormatContext->streams[0]->codec->channels, 300 * frame->nb_samples);
		}
		
		if(bStartRecord)
		{
			int buf_space = av_audio_fifo_space2(fifo_audio);
			if (av_audio_fifo_space2(fifo_audio) >= frame->nb_samples)
			{
				//音频数据入录像队列.
				EnterCriticalSection(&section);
				//tickqueue.push_back(GetTickCount());
				av_audio_fifo_write2(fifo_audio, (void **)frame->data, frame->nb_samples);
				LeaveCriticalSection(&section);
			}
			else
			{
				av_log(NULL,AV_LOG_PANIC,"audio lost\r\n");
			}
		}
		
		
		
	}
	av_frame_free(&frame);
	if(pFormatContext)
		avformat_close_input(&pFormatContext);
	pFormatContext = NULL;

	if(fifo_audio)
	{
		av_audio_fifo_free2(fifo_audio);
		fifo_audio = NULL;
	}

	bQuit = true;
	evt_quit.set();
	av_log(NULL,AV_LOG_ERROR,"video thread exit\r\n");

}
//重采样音频包到目标格式.
AVFrame* AudioCap::ReSampleFrame(AVFrame* pFrame)
{
	int dst_nb_samples;
	//如果输入和输出的音频格式不一样 需要重采样，这里是一样的就没做
	dst_nb_samples = av_rescale_rnd(swr_get_delay(audio_swr_ctx, pOutputCtx->sample_rate) + pFrame->nb_samples,
		pOutputCtx->sample_rate, pOutputCtx->sample_rate, AV_ROUND_UP);

	int ret = swr_convert(audio_swr_ctx,
		pAudioFrame->data, dst_nb_samples,
		(const uint8_t **)pFrame->data, pFrame->nb_samples);
	return pAudioFrame;
}
bool AudioCap::NeedReSample(AVCodecContext *ctx1, AVCodecContext *ctx2)
{

	if (ctx1->sample_fmt != ctx2->sample_fmt 
		|| ctx1->channels != ctx2->channels 
		|| ctx1->sample_rate != ctx2->sample_rate)
	{
		return true;
	}
	return false;
}
bool AudioCap::StartRecord(AVCodecContext* pOutputCtx)
{
	
	this->pOutputCtx = pOutputCtx;
	pAudioFrame = alloc_audio_frame(pOutputCtx->sample_fmt,pOutputCtx->channel_layout,pOutputCtx->sample_rate,pOutputCtx->frame_size);
	audio_swr_ctx = swr_alloc();
	if (!audio_swr_ctx) {
		av_log(NULL,AV_LOG_ERROR, "Could not allocate resampler context\n");
		return false;
	}
	av_opt_set_int       (audio_swr_ctx, "in_channel_count",   GetCodecContext()->channels,       0);
	av_opt_set_int       (audio_swr_ctx, "in_sample_rate",     GetCodecContext()->sample_rate,    0);
	av_opt_set_sample_fmt(audio_swr_ctx, "in_sample_fmt",      GetCodecContext()->sample_fmt, 0);
	av_opt_set_int       (audio_swr_ctx, "out_channel_count",  pOutputCtx->channels,       0);
	av_opt_set_int       (audio_swr_ctx, "out_sample_rate",    pOutputCtx->sample_rate,    0);
	av_opt_set_sample_fmt(audio_swr_ctx, "out_sample_fmt",     pOutputCtx->sample_fmt,     0);
	/* initialize the resampling context */
	if ((swr_init(audio_swr_ctx)) < 0) {
		av_log(NULL,AV_LOG_ERROR, "Failed to initialize the resampling context\n");
		return false;
	}
	pTmpFrame = alloc_audio_frame(GetCodecContext()->sample_fmt,\
		pOutputCtx->channel_layout,\
		pOutputCtx->sample_rate,\
		pOutputCtx->frame_size);
	bStartRecord = true;
	return true;
}
bool AudioCap::StartRecord()
{
	bStartRecord = true;
	return true;
}
int AudioCap::StopRecord()
{
	bStartRecord = false;
	EnterCriticalSection(&section);
	av_audio_fifo_reset2(fifo_audio);
	LeaveCriticalSection(&section);
	return 0;
}
int AudioCap::SimpleSize()
{
	int size = 0;
	EnterCriticalSection(&section);
	size = av_audio_fifo_size2(fifo_audio);
	LeaveCriticalSection(&section);
	return 	size;
}
AVFrame* AudioCap::GetAudioFrame()
{
	DWORD timeStamp = 0;
	AVFrame* pFrame = NULL;
	int nSize = pOutputCtx->frame_size > 0? pOutputCtx->frame_size:1024;
	if(SimpleSize() < nSize) 
		return NULL;

	if(GetSample((void**)pTmpFrame->data,nSize,timeStamp) == 0) 
		return NULL;
	pFrame = pTmpFrame;
	if(NeedReSample(pOutputCtx,GetCodecContext()))
	{
		pFrame = ReSampleFrame(pTmpFrame);
	}
	return pFrame;
}
int AudioCap::GetSample(void **data, int nb_samples,DWORD &timestamp)
{
	int nRead = 0;
	unsigned long tick = 0;
	if(fifo_audio==NULL)return 0;

	EnterCriticalSection(&section);
			//从音频fifo中读取编码器需要的样本个数.
	nRead = av_audio_fifo_read2(fifo_audio, data, 
		nb_samples);
	timestamp = tick;
	LeaveCriticalSection(&section);


	return nRead;
}
bool AudioCap::GetTimeStamp(DWORD &timeStamp)
{
	return false;

}