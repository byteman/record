#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H
#include "Utils.h"
#include "audio_fifo.h"
int OpenAudioCapture(AVFormatContext** pFmtCtx, const char * psDevName, AVInputFormat *ifmt);

class AudioCap{
public:
	AudioCap();
	int Open(const char * psDevName, AVInputFormat *ifmt);
	int Close();
	void Run( );
	bool StartRecord();
	int  StopRecord();
	AVCodecContext* GetCodecContext();
	int GetSample(void **data, int nb_samples,DWORD timestamp);
	bool GetTimeStamp(DWORD &timeStamp);
	int SimpleSize();
	
	//查找与给定时间戳最匹配的视频帧，如果找不到就返回最后一帧.
	AVFrame* GetMatchFrame(DWORD timestamp);
private:
	AVFormatContext* pFormatContext;
	AVCodecContext*  pCodecContext;
	AVAudioFifo2		*fifo_audio;
	bool bCapture;
	bool bQuit;
	bool bStartRecord;
	SwsContext		 *sws_ctx;
	CRITICAL_SECTION section;
	Win32Event evt_ready;
	Win32Event evt_quit; 

};
#endif