#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include "Recorder.h"


class VideoCap{
public:
	VideoCap();
	int OpenPreview(const char* psDevName,AVInputFormat *ifmt,const unsigned  int width,
													const unsigned  int height,
													const unsigned  int FrameRate,AVPixelFormat format, Video_Callback pCbFunc);
	bool SetCallBackAttr(int width, int height, AVPixelFormat format,Video_Callback pCbFunc);
	int Close();
	void Run( );
	AVRational GetVideoTimeBase();
	AVRational Get_aspect_ratio();
	int GetWidth();
	int GetHeight();
	AVPixelFormat GetFormat();

private:
	int OpenVideoCapture(AVFormatContext** pFmtCtx, AVCodecContext	** pCodecCtx,const char* psDevName,AVInputFormat *ifmt,const unsigned  int width,
													const unsigned  int height,
													const unsigned  int FrameRate);
	AVFormatContext* pFormatContext;
	AVCodecContext*  pCodecContext;
	bool bCapture;
	bool bQuit;
	Video_Callback   pCallback;
	SwsContext		 *sws_ctx;
	int VideoIndex;
	int prev_width,prev_height;
	AVPixelFormat prev_fmt;
};
#endif