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

#include "Utils.h"
#include "Recorder.h"
#include "RecordMux.h"
#include "VideoCapture.h"
#include "AudioCapture.h"
#include "RateCtrl.h"

static AVFrame* pAudioFrame = NULL;
static AVFrame* pEncFrame = NULL;
static AVFrame* pRecFrame = NULL;

SwrContext *audio_swr_ctx = NULL;

MyFile file1;
MyFile file2;
/*
录像的时候才会指定输出宽度和高度，码率，编码类型,帧率
还有编码器的参数(gop等）
设置输出文件的相关参数.
主要是分配输出文件的FormatContext，创建2个流，设置流的相关参数。打开对于的编码器.
*/
int RecordMux::OpenOutPut(const char* outFileName,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle)
{
	AVStream *pVideoStream = NULL, *pAudioStream = NULL;

	//为输出文件分配FormatContext
	avformat_alloc_output_context2(&pFmtContext, NULL, NULL, outFileName); //这个函数调用后pFormatCtx_Out->oformat中就猜出来了目标编码器
	//创建视频流，并且视频编码器初始化.
	//if (pFormatCtx_Video->streams[0]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		VideoIndex = 0;
		//为输出文件新建一个视频流,创建成果后，pFormatCtx_Out中的streams成员就已经添加为pVideoStream了.
		pVideoStream = avformat_new_stream(pFmtContext, NULL);

		if (!pVideoStream)
		{
			av_log(NULL,AV_LOG_ERROR,"can not new video stream for output!\n");
			avformat_free_context(pFmtContext);
			return -2;
		}

		//set codec context param
#ifdef H264_ENC
		pVideoStream->codec->codec  = avcodec_find_encoder(AV_CODEC_ID_H264); //视频流的编码器为MPEG4
#else
		if(pFmtContext->oformat->video_codec == AV_CODEC_ID_H264)
		{
			pFmtContext->oformat->video_codec = AV_CODEC_ID_MPEG4;
		}
		pVideoStream->codec->codec =  avcodec_find_encoder(pFmtContext->oformat->video_codec);
		
#endif

		//open encoder
		if (!pVideoStream->codec->codec)
		{
			av_log(NULL,AV_LOG_ERROR,"can not find the encoder!\n");
			return -3;
		}

		pVideoStream->codec->height = pVideoInfo->height; //输出文件视频流的高度
		pVideoStream->codec->width  = 2*pVideoInfo->width;  //输出文件视频流的宽度
		AVRational ar;
		ar.den = 15;
		ar.num = 1;
		pVideoStream->codec->time_base = pVideoCap->GetVideoTimeBase(); //输出文件视频流的高度和输入文件的时基一致.
		pVideoStream->time_base = pVideoCap->GetVideoTimeBase();
		pVideoStream->codec->sample_aspect_ratio = pVideoCap->Get_aspect_ratio();
		// take first format from list of supported formats
		//可查看ff_mpeg4_encoder中指定的第一个像素格式，这个是采用MPEG4编码的时候，输入视频帧的像素格式，这里是YUV420P
		pVideoStream->codec->pix_fmt = pFmtContext->streams[VideoIndex]->codec->codec->pix_fmts[0]; //像素格式，采用MPEG4支持的第一个格式
		
		//CBR_Set(pVideoStream->codec, pVideoInfo->bitrate); //设置固定码率
		VBR_Set(pVideoStream->codec, pVideoInfo->bitrate, 2*pVideoInfo->bitrate  , 0);
		
		if (pFmtContext->oformat->flags & AVFMT_GLOBALHEADER)
			pVideoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		AVDictionary *options = NULL;
		av_dict_set(&options, "bufsize", "5120k", NULL);//这里可以指定编码器的参数
		//打开输出文件的视频编码器
#ifdef H264_ENC
		pVideoStream->codec->me_range = 16;
        pVideoStream->codec->max_qdiff = 4;
        pVideoStream->codec->qmin = 10;
        pVideoStream->codec->qmax = 51;
        pVideoStream->codec->qcompress = 0.6;
#endif
		//pVideoStream->rc_lookahead=0;//这样就不会延迟编码器的输出了
		//av_opt_set(pVideoStream->codec->priv_data, "preset", "slow", 0);
		if ((avcodec_open2(pVideoStream->codec, pVideoStream->codec->codec, &options)) < 0)
		{
			av_log(NULL,AV_LOG_ERROR,"can not open the encoder\n");
			return -4;
		}
	}
	//创建音频流，并且音频编码器初始化.
	//if(pFormatCtx_Audio->streams[0]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		AVCodecContext *pOutputCodecCtx = NULL;
		AudioIndex = 1;
		pAudioStream = avformat_new_stream(pFmtContext, NULL);
		//在avformat_alloc_output_context2 中就找到了 pFormatCtx_Out->oformat。根据文件的后缀，匹配到了AVOutputFormat ff_mp4_muxer
		//    .audio_codec       = AV_CODEC_ID_AAC 
		
		//pAudioStream->codec->codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		pAudioStream->codec->codec = avcodec_find_encoder(pFmtContext->oformat->audio_codec);

		pOutputCodecCtx = pAudioStream->codec;

		pOutputCodecCtx->sample_fmt = pOutputCodecCtx->codec->sample_fmts?pOutputCodecCtx->codec->sample_fmts[0]:AV_SAMPLE_FMT_FLTP; //输出的采样率等于采集的采样率
		pOutputCodecCtx->bit_rate = 64000;//pAudioInfo->bitrate;
		pOutputCodecCtx->sample_rate = 44100;
		if (pOutputCodecCtx->codec->supported_samplerates) {
            pOutputCodecCtx->sample_rate = pOutputCodecCtx->codec->supported_samplerates[0];
            for (int i = 0; pOutputCodecCtx->codec->supported_samplerates[i]; i++) {
                if (pOutputCodecCtx->codec->supported_samplerates[i] == 44100)
                    pOutputCodecCtx->sample_rate = 44100;
            }
        }

		//pOutputCodecCtx->channel_layout = pFormatCtx_Audio->streams[0]->codec->channel_layout;
		pOutputCodecCtx->channels = av_get_channel_layout_nb_channels(pOutputCodecCtx->channel_layout);//输出的通道等于采集的通道数
		pOutputCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
		if (pOutputCodecCtx->codec->channel_layouts) {
            pOutputCodecCtx->channel_layout = pOutputCodecCtx->codec->channel_layouts[0];
            for (int i = 0; pOutputCodecCtx->codec->channel_layouts[i]; i++) {
                if (pOutputCodecCtx->codec->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    pOutputCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
	
		pOutputCodecCtx->channels        = av_get_channel_layout_nb_channels(pOutputCodecCtx->channel_layout);
       
		AVRational time_base={1, pAudioStream->codec->sample_rate};
		pAudioStream->time_base = time_base; //设置
		
		pOutputCodecCtx->codec_tag = 0;  
		//mpeg4    .flags             = AVFMT_GLOBALHEADER | AVFMT_ALLOW_FLUSH | AVFMT_TS_NEGATIVE,
		if (pFmtContext->oformat->flags & AVFMT_GLOBALHEADER)  
			pOutputCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
		//打开输出文件的编码器.
		if (avcodec_open2(pOutputCodecCtx, pOutputCodecCtx->codec, 0) < 0)
		{
			//编码器打开失败，退出程序
			av_log(NULL,AV_LOG_ERROR,"can not open output codec!\n");
			return -5;
		}
		pAudioFrame = alloc_audio_frame(pAudioStream->codec->sample_fmt,pAudioStream->codec->channel_layout,pAudioStream->codec->sample_rate,pAudioStream->codec->frame_size);
		audio_swr_ctx = swr_alloc();
        if (!audio_swr_ctx) {
            av_log(NULL,AV_LOG_ERROR, "Could not allocate resampler context\n");
            return -9;
        }
		av_opt_set_int       (audio_swr_ctx, "in_channel_count",   pAudioCap->GetCodecContext()->channels,       0);
        av_opt_set_int       (audio_swr_ctx, "in_sample_rate",     pAudioCap->GetCodecContext()->sample_rate,    0);
		av_opt_set_sample_fmt(audio_swr_ctx, "in_sample_fmt",      pAudioCap->GetCodecContext()->sample_fmt, 0);
        av_opt_set_int       (audio_swr_ctx, "out_channel_count",  pAudioStream->codec->channels,       0);
        av_opt_set_int       (audio_swr_ctx, "out_sample_rate",    pAudioStream->codec->sample_rate,    0);
        av_opt_set_sample_fmt(audio_swr_ctx, "out_sample_fmt",     pAudioStream->codec->sample_fmt,     0);

        /* initialize the resampling context */
        if ((swr_init(audio_swr_ctx)) < 0) {
            av_log(NULL,AV_LOG_ERROR, "Failed to initialize the resampling context\n");
            return -10;
        }
	}
	//打开文件
	if (!(pFmtContext->oformat->flags & AVFMT_NOFILE))
	{
		if(avio_open(&pFmtContext->pb, outFileName, AVIO_FLAG_WRITE) < 0)
		{
			av_log(NULL,AV_LOG_ERROR,"can not open output file handle!\n");
			return -6;
		}
	}
	//写入文件头
	if(avformat_write_header(pFmtContext, NULL) < 0)
	{
		av_log(NULL,AV_LOG_ERROR,"can not write the header of the output file!\n");
		return -7;
	}	
	pRecFrame = alloc_picture(AV_PIX_FMT_YUV420P,pVideoInfo->width,pVideoInfo->height,16);

	return 0;
}


//录像线程.
DWORD WINAPI RecordThreadProc( LPVOID lpParam )
{
	
	if(lpParam != NULL)
	{
		RecordMux* rm = (RecordMux*)lpParam;
		rm->Run();
	}

	return 0;
}
RecordMux::RecordMux()
{
	InitializeCriticalSection(&VideoSection);
	pVideoCap = new VideoCap(1);
	pVideoCap2 = new VideoCap(2);
	pAudioCap = new AudioCap();
	bStartRecord = false;
	bCapture = false;
	recordThreadQuit = true;
	cur_pts_v = cur_pts_a = 0;
	pDShowInputFmt = NULL;
#if 0
	MyFile f("111.yuv");
	f.FillBuffer(0x0, 320*240*1);
	f.FillBuffer(0x80, 320*240/4);
	f.FillBuffer(0x80, 320*240/4);
	MyFile f2("222.yuv");
	f2.FillBuffer(0xFF, 320*240*1.5);
#endif
	
}
/*
启动录像操作
*/
int RecordMux::Start(const char* filePath,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle)
{
	
	int ret = OpenOutPut(filePath,pVideoInfo,pAudioInfo,pSubTitle);
	pVideoCap->StartRecord(AV_PIX_FMT_YUV420P,  pVideoInfo->width, pVideoInfo->height);
	pVideoCap2->StartRecord(AV_PIX_FMT_YUV420P, pVideoInfo->width, pVideoInfo->height);

	pRecFrame = alloc_picture(AV_PIX_FMT_YUV420P,pVideoInfo->width*2,pVideoInfo->height,16);

	pAudioCap->StartRecord();
	CreateThread( NULL, 0, RecordThreadProc, this, 0, NULL);
	return ret;
}
/*
停止录像操作
*/
int RecordMux::Stop()
{
	//如果还没有启动录像，就直接返回.
	if(false == bStartRecord)
	{
		return ERR_RECORD_OK;
	}
	bStartRecord = false;
	pVideoCap->StopRecord();
	pVideoCap2->StopRecord();
	pAudioCap->StopRecord();
	while(!recordThreadQuit)
	{
		av_log(NULL,AV_LOG_ERROR,"RecordStop wait quit\r\n");
		Sleep(10);
	}
	return 0;
}
/*
关闭所有设备.
*/
int RecordMux::Close()
{
	pAudioCap->Close();
	pVideoCap->Close();
	pVideoCap2->Close();

	return 0;
}
bool AudioFmtEqual(AVCodecContext *ctx1, AVCodecContext *ctx2)
{
	
	if (ctx1->sample_fmt != ctx2->sample_fmt 
					|| ctx1->channels != ctx2->channels 
					|| ctx1->sample_rate != ctx2->sample_rate)
	{
		return false;
	}
	return true;
}

AVFrame* RecordMux::MergeFrame(AVFrame* frame1, AVFrame* frame2)
{
	int offset = 0;
	int i = 0;
#if 0
	for( i = 0; i < frame1->height; i++)
	{
		memset(pRecFrame->data[0]+offset,0x0,frame1->width);
		offset+=frame1->width;
		memset(pRecFrame->data[0]+offset,0xFF,frame1->width);
		
		offset+=frame1->width;
	}
	offset = 0;

	for( i = 0; i < frame1->height/2; i++)
	{
		memset(pRecFrame->data[1]+offset,0x0,frame1->width/2);
		offset+=frame1->width/2;
		memset(pRecFrame->data[1]+offset,0x0,frame1->width/2);
		
		offset+=frame1->width/2;
	}
	offset = 0;

	for( i = 0; i < frame1->height/2; i++)
	{
		memset(pRecFrame->data[2]+offset,0x0,frame1->width/2);
		offset+=frame1->width/2;
		memset(pRecFrame->data[2]+offset,0x0,frame1->width/2);
		
		offset+=frame1->width/2;
	}
	offset = 0;


	
	return NULL;
#endif
	//file1.ReadYUV420P(frame1);
	//file2.ReadYUV420P(frame2);
	offset = 0;
	for( i = 0; i < frame1->height; i++)
	{
		memcpy(pRecFrame->data[0]+offset, frame1->data[0], frame1->width);
		offset+=frame1->width;
		memcpy(pRecFrame->data[0]+offset, frame2->data[0], frame2->width);
		offset+=frame2->width;
	}
	offset = 0;
	for( i = 0; i < frame1->height/2; i++)
	{
		memcpy(pRecFrame->data[1]+offset, frame1->data[1], frame1->width/2);
		offset+=frame1->width/2;
		memcpy(pRecFrame->data[1]+offset, frame2->data[1], frame2->width/2);
		offset+=frame1->width/2;
		
	}
	offset = 0;
	for( i = 0; i < frame1->height/2; i++)
	{
		memcpy(pRecFrame->data[2]+offset, frame1->data[2], frame1->width/2);
		offset+=frame1->width/2;
		memcpy(pRecFrame->data[2]+offset, frame2->data[2], frame2->width/2);
		offset+=frame1->width/2;
		
	}
	return pRecFrame;

}
void RecordMux::Run()
{
	AVFrame* pSecordFrame = NULL;
	AVFrame* ptmp = NULL;
	bStartRecord = true;
	cur_pts_v = cur_pts_a = 0; //复位音视频的pts
	VideoFrameIndex = AudioFrameIndex = 0; //复位音视频的帧序.
	MyFile file11("1.yuv");
	MyFile file22("2.yuv");
	MyFile file33("12.yuv");
	while(bStartRecord) //启动了录像标志，才进行录像，否则退出线程
	{

		if(av_compare_ts(cur_pts_v, pFmtContext->streams[VideoIndex]->codec->time_base, 
			cur_pts_a,pFmtContext->streams[AudioIndex]->codec->time_base) <= 0)
		{

			if( (pEncFrame = pVideoCap->GetSample()) != NULL )
			{
				int got_picture = 0;
				AVPacket pkt;

				pSecordFrame = pVideoCap2->GetLastSample();
				
				MergeFrame(pEncFrame,pSecordFrame);
				file11.WriteFrame(pEncFrame);
				file22.WriteFrame(pSecordFrame);
				file33.WriteFrame(pRecFrame);
				ptmp = pRecFrame;
				//pts = n * (（1 / timbase）/ fps); 计算pts,编码之前计算pts
				ptmp->pts = cur_pts_v++;// * ((pFormatCtx_Video->streams[0]->time_base.den / pFormatCtx_Video->streams[0]->time_base.num) / FPS);
				av_init_packet(&pkt);
				
				pkt.data = NULL;
				pkt.size = 0;
				//编码一帧视频.
				int ret = avcodec_encode_video2(pFmtContext->streams[VideoIndex]->codec, &pkt, ptmp, &got_picture);
				if(ret < 0)
				{
					//编码错误,不理会此帧
					continue;
				}
				
				if ( got_picture == 1 )
				{
 					pkt.stream_index = VideoIndex;
					//将编码后的包的Pts和dts，转换到输出文件中指定的时基 .在编码后就可以得出包的pts和dts.
				
					av_packet_rescale_ts(&pkt, pFmtContext->streams[VideoIndex]->codec->time_base, pFmtContext->streams[VideoIndex]->time_base);
				
					//写入一个packet.
					ret = av_interleaved_write_frame(pFmtContext, &pkt);

					av_free_packet(&pkt);
				}
				
			}
		}
		else
		{

			if(pAudioCap->SimpleSize() >= 
				(pFmtContext->streams[AudioIndex]->codec->frame_size > 0 ? pFmtContext->streams[AudioIndex]->codec->frame_size : 1024))
			{

				AVFrame *frame;
				AVFrame *frame2;
				frame = alloc_audio_frame(pAudioCap->GetCodecContext()->sample_fmt,\
					pFmtContext->streams[1]->codec->channel_layout,\
					pFmtContext->streams[1]->codec->sample_rate,\
					pFmtContext->streams[1]->codec->frame_size);
				frame2 = frame;

				pAudioCap->GetSample((void **)frame->data, 
					pFmtContext->streams[1]->codec->frame_size);


				if(!AudioFmtEqual(pFmtContext->streams[AudioIndex]->codec, pAudioCap->GetCodecContext()) )
				{
					int dst_nb_samples;
					//如果输入和输出的音频格式不一样 需要重采样，这里是一样的就没做
					dst_nb_samples = av_rescale_rnd(swr_get_delay(audio_swr_ctx, pFmtContext->streams[AudioIndex]->codec->sample_rate) + frame->nb_samples,
									pFmtContext->streams[AudioIndex]->codec->sample_rate, pFmtContext->streams[AudioIndex]->codec->sample_rate, AV_ROUND_UP);
					 
					int ret = swr_convert(audio_swr_ctx,
						pAudioFrame->data, dst_nb_samples,
						(const uint8_t **)frame->data, frame->nb_samples);
					frame = pAudioFrame;
				}
			
				AVPacket pkt_out;
				av_init_packet(&pkt_out);
				int got_picture = -1;
				pkt_out.data = NULL;
				pkt_out.size = 0;

				frame->pts = cur_pts_a;
				cur_pts_a+=pFmtContext->streams[AudioIndex]->codec->frame_size;

				if (avcodec_encode_audio2(pFmtContext->streams[AudioIndex]->codec, &pkt_out, frame, &got_picture) < 0)
				{
					av_log(NULL,AV_LOG_ERROR,"can not decoder a frame");
				}
				if(frame2)
					av_frame_free(&frame2);
				if (got_picture) 
				{
					pkt_out.stream_index = AudioIndex; //千万要记得加这句话，否则会导致没有音频流.
					av_packet_rescale_ts(&pkt_out, pFmtContext->streams[AudioIndex]->codec->time_base, pFmtContext->streams[AudioIndex]->time_base);
				
					int ret = av_interleaved_write_frame(pFmtContext, &pkt_out);
					if(ret == 0)
					{
						//av_log(NULL,AV_LOG_PANIC,"write audio ok\r\n");
					}
					else
					{
						av_log(NULL,AV_LOG_PANIC,"write audio failed\r\n");
					}
					
					av_free_packet(&pkt_out);
				}
				else
				{
					//av_log(NULL,AV_LOG_PANIC,"xxxxxxxxxxxxxwrite audio file failed\r\n");
				}
				
			}
		}
	}
	if(pEnc_yuv420p_buf)
	delete[] pEnc_yuv420p_buf;

	if(pEncFrame)
	{
		av_frame_free(&pEncFrame);
	}

	av_write_trailer(pFmtContext);

	avio_close(pFmtContext->pb);
	avformat_free_context(pFmtContext);

	
	recordThreadQuit = true;
	av_log(NULL,AV_LOG_INFO,"app  exit\r\n");
	
}

int RecordMux::OpenCamera(const char* psDevName,const char* psDevName2,const unsigned  int width,
													const unsigned  int height,
													const unsigned  int FrameRate,AVPixelFormat format, Video_Callback pCbFunc)
{
	int rt = 0;
	if(pDShowInputFmt == NULL)Init();
		
	rt = pVideoCap2->OpenPreview(psDevName2,pDShowInputFmt,width,height,FrameRate,AV_PIX_FMT_YUYV422,format, pCbFunc);
	if(rt != ERR_RECORD_OK) return rt;
	return pVideoCap->OpenPreview(psDevName,pDShowInputFmt,width,height,FrameRate, AV_PIX_FMT_YUV420P,format, pCbFunc);

}
int RecordMux::OpenAudio(const char * psDevName)
{
	if(pDShowInputFmt == NULL)Init();
	return pAudioCap->Open(psDevName,pDShowInputFmt);
}
int RecordMux::Init()
{
	file1.Open("111.yuv",AV_PIX_FMT_YUV420P,320,240,10);
	file2.Open("222.yuv",AV_PIX_FMT_YUV420P,320,240,10);
	pDShowInputFmt = av_find_input_format("dshow");
	return 0;
}