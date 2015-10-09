#ifdef	__cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#ifdef __cplusplus
};
#endif
#include "RateCtrl.h"
/*
 固定码率设置,码流大小控制的比较准确.
 CBR (Constant Bit Rate)
There is no native CBR mode, but you can "simulate" a constant bit rate setting by tuning the parameters of ABR, like
ffmpeg -i input -c:v libx264 -b:v 4000k -minrate 4000k -maxrate 4000k -bufsize 1835k out.m2v
in this example, -bufsize is the "rate control buffer" so it will enforce your requested "average" (4000k in this case) across each 1835k worth of video. So basically it is assumed that the receiver/end player will buffer that much data so it's ok to fluctuate within that much.
Of course, if it's all just empty/black frames then it will still serve less than that many bits/s (but it will raise the quality level as much as it can, up to the crf level).

只设置bit_rate是平均码率，不一定能控制住

c->bit_rate = 400000;
c->rc_max_rate = 400000;
c->rc_min_rate = 400000;
*/
void CBR_Set(AVCodecContext *c, long br)
{
	c->bit_rate = br;
	c->rc_min_rate =br;
	c->rc_max_rate = br;
	c->bit_rate_tolerance = br;
	c->rc_buffer_size=br;
	c->rc_initial_buffer_occupancy = c->rc_buffer_size*3/4;
	//c->rc_buffer_aggressivity= (float)1.0;
	//c->rc_initial_cplx= 0.5;
}
/*
 可变码率设置,码流大小控制的不太准确.
 ffmpeg.x264 doc : https://trac.ffmpeg.org/wiki/Encode/H.264
*/
void VBR_Set(AVCodecContext *c, long br, long max, long min)
{
	c->flags |= CODEC_FLAG_QSCALE;
    c->rc_min_rate  = min;
    c->rc_max_rate  = max;
    c->bit_rate		= br; //这个码率是评价

	/*
	c->flags |= CODEC_FLAG_QSCALE;
	c->rc_min_rate =min;
	c->rc_max_rate = max; 
	c->bit_rate = br;
	*/
}