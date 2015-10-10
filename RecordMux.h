#ifndef RECORD_MUX_H
#define RECORD_MUX_H

class VideoCap;
class AudioCap;
class RecordMux
{
public:
	RecordMux();
	int Init();
	int OpenCamera(const char* psDevName,const char* psDevName2,const unsigned  int width,
													const unsigned  int height,
													const unsigned  int FrameRate,AVPixelFormat format, Video_Callback pCbFunc);

	int OpenAudio(const char * psDevName);
	int Start(const char* filePath,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle);
	int Stop();
	void Run();
	int Close();
	
private:
	int OpenOutPut(const char* outFileName,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle);
	AVFrame* MergeFrame(AVFrame* frame1, AVFrame* frame2);
	VideoCap* pVideoCap;
	VideoCap* pVideoCap2;
	AudioCap* pAudioCap;
	AVFormatContext *pFmtContext;
	int VideoIndex,AudioIndex;
	bool bStartRecord,bCapture;
	int64_t cur_pts_v,cur_pts_a;
	int VideoFrameIndex,AudioFrameIndex;

	CRITICAL_SECTION VideoSection;
	unsigned char* pEnc_yuv420p_buf;
	bool recordThreadQuit;
	AVInputFormat *pDShowInputFmt;
	AVFrame* pRecFrame;
	
};
#endif