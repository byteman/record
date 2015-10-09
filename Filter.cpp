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


#ifdef __cplusplus
};
#endif
#include "Filter.h"

//const char *filter_descr = "movie=my_logo.png[wm];[in][wm]overlay=5:5[out]";
//const char *filter_descr="drawtext=fontfile=simfang.ttf: fontcolor=red: fontsize=32: shadowcolor=black: text='hello': x=10:y=10";
const char *filter_descr="drawtext=fontfile=simfang.ttf:fontcolor=red:fontsize=32:shadowcolor=black:text='hello':x=10:y=10";
//const char *filter_descr="drawtext=fontfile=simfang.ttf: timecode='09\:57\:00\;00': r=30: x=(w-tw)/2: y=h-(2*lh): fontcolor=white: box=1: boxcolor=0x00000000@1";
//const char *filter_descr = "overlay=5:5";   


AVFilterGraph	*filter_graph = NULL;
AVFilterContext *buffersink_ctx = NULL;
AVFilterContext *buffersrc_ctx = NULL;

static void uninit_filter()
{
	avfilter_graph_free(&filter_graph);
	filter_graph = NULL;
}
static int init_filters(AVFormatContext* pFmtCtx, const char *filters_descr)  
{  
    char args[512];  
    int ret;  
    AVFilter *buffersrc  = avfilter_get_by_name("buffer");  
    AVFilter *buffersink = avfilter_get_by_name("ffbuffersink");  
    AVFilterInOut *outputs = avfilter_inout_alloc();  
    AVFilterInOut *inputs  = avfilter_inout_alloc();  
    enum PixelFormat pix_fmts[] = { AV_PIX_FMT_RGB24, PIX_FMT_NONE };  
    AVBufferSinkParams *buffersink_params;  
  
    filter_graph = avfilter_graph_alloc();  
  
    /* buffer video source: the decoded frames from the decoder will be inserted here. */  
	//pFormatCtx_Video->streams[0]->codec->pix_fmt
    _snprintf_s(args, sizeof(args),  
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",  
			pFmtCtx->streams[0]->codec->width, pFmtCtx->streams[0]->codec->height, pFmtCtx->streams[0]->codec->pix_fmt ,  
            pFmtCtx->streams[0]->codec->time_base.num, pFmtCtx->streams[0]->codec->time_base.den,  
            pFmtCtx->streams[0]->codec->sample_aspect_ratio.num, pFmtCtx->streams[0]->codec->sample_aspect_ratio.den);  
  
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",  
                                       args, NULL, filter_graph);  
    if (ret < 0) {  
        av_log(NULL,AV_LOG_ERROR,"Cannot create buffer source\n");  
         goto end;
    }  
  
    /* buffer video sink: to terminate the filter chain. */  
    //buffersink_params = av_buffersink_params_alloc();  
    //buffersink_params->pixel_fmts = pix_fmts;  
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",  
                                       NULL, NULL, filter_graph);  
	ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
         goto end;
    }
    //av_free(buffersink_params);  
    if (ret < 0) {  
        av_log(NULL,AV_LOG_ERROR,"Cannot create buffer sink\n");  
         goto end;
    }  
  
    /* Endpoints for the filter graph. */  
    outputs->name       = av_strdup("in");  
    outputs->filter_ctx = buffersrc_ctx;  
    outputs->pad_idx    = 0;  
    outputs->next       = NULL;  
  
    inputs->name       = av_strdup("out");  
    inputs->filter_ctx = buffersink_ctx;  
    inputs->pad_idx    = 0;  
    inputs->next       = NULL;  
  
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,  
                                    &inputs, &outputs, NULL)) < 0)  
         goto end;
  
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)  
        goto end;
end:
	if(inputs)
		avfilter_inout_free(&inputs);
	if(outputs)
		avfilter_inout_free(&outputs);
    return 0;  
}  


static int push_filter(AVFrame	*pFrame,AVFrame* pFilterFrame)
{
	int ret = 0;
	//AVFilterBufferRef *picref = NULL;
	//AVFrame *filt_frame = av_frame_alloc();

	pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);
	 /* push the decoded frame into the filtergraph */
	   /* push the decoded frame into the filtergraph */
    if (av_buffersrc_add_frame_flags(buffersrc_ctx, pFrame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
        return -1;
    }
#if 0
    if (av_buffersrc_add_frame(buffersrc_ctx, pFrame) < 0) {
        av_log(NULL,AV_LOG_ERROR,( "Error while feeding the filtergraph\n");
        return NULL;
    }
#endif
	 while (1) {
        ret = av_buffersink_get_frame(buffersink_ctx, pFilterFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
			return -2;
		}
        
        if (ret < 0)
		{
			return ret;	
		}

		return 0;


    }

}
