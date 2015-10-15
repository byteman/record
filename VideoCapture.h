#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include "Recorder.h"
#include "Utils.h"
#include "Poco\Event.h"

class VideoCap{
public:
	VideoCap(int chan);
	int OpenPreview(const char* psDevName,int index,AVInputFormat *ifmt,const unsigned  int width,
													const unsigned  int height,
													unsigned  int &FrameRate,AVPixelFormat cap_format,AVPixelFormat format, Video_Callback pCbFunc);
	bool SetCallBackAttr(int width, int height, AVPixelFormat format,Video_Callback pCbFunc);
	int Close();
	void Run( );
	AVRational GetVideoTimeBase();
	AVRational Get_aspect_ratio();
	int GetWidth();
	int GetHeight();
	AVPixelFormat GetFormat();
	AVFrame* GetSample();
	AVFrame* GetLastSample();
	int StartRecord(AVPixelFormat format, int width, int height);
	int StopRecord();
	int Start();
	int Stop();
private:

	int OpenVideoCapture(AVFormatContext** pFmtCtx, AVCodecContext	** pCodecCtx,const char* psDevName,int index,AVInputFormat *ifmt,const unsigned  int width,
													const unsigned  int height,
													unsigned  int &FrameRate,const char* fmt);
	AVFormatContext* pFormatContext;
	AVCodecContext*  pCodecContext;
	bool bCapture;
	bool bQuit;
	bool bStartRecord;
	Video_Callback   pCallback;
	SwsContext		 *sws_ctx;
	SwsContext		 *record_sws_ctx;
	int VideoIndex;
	int _width,_height;
	AVPixelFormat _fmt;


	AVFifoBuffer	*fifo_video;
	CRITICAL_SECTION section;
	AVFrame* pRecFrame;
	AVFrame* pRecFrame2;
	int y_size;
	int nSampleSize;
	int nRGB24Size;
	int channel;
	//Poco::Event evt_ready;
	//Poco::Event evt_quit;
	Win32Event evt_ready;
	Win32Event evt_quit;
};
#endif