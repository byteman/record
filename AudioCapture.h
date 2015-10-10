#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

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
	int GetSample(void **data, int nb_samples);
	int SimpleSize();
private:
	AVFormatContext* pFormatContext;
	AVCodecContext*  pCodecContext;
	AVAudioFifo		*fifo_audio;
	bool bCapture;
	bool bQuit;
	bool bStartRecord;
	SwsContext		 *sws_ctx;
	CRITICAL_SECTION section;

};
#endif