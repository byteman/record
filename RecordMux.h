#ifndef RECORD_MUX_H
#define RECORD_MUX_H
#include <vector>
class VideoCap;
class AudioCap;
class RecordMux
{
public:
	typedef std::vector<VideoCap*> VideoCapList; 
	RecordMux();
	~RecordMux();
	int Init();
	int GetNum();
	int choose_output(void);
	int OpenCamera(const char* psDevName,int index,const unsigned  int width,
													const unsigned  int height,
													unsigned  int &FrameRate,AVPixelFormat format, Video_Callback pCbFunc);
	int OpenCamera2(const char* psDevName,int index,const unsigned  int width,
													const unsigned  int height,
													unsigned  int &FrameRate,AVPixelFormat format, Video_Callback pCbFunc);

	int OpenAudio(const char * psDevName);
	int Start(const char* filePath,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle);
	int Stop();
	void Run();
	int Close();
	
	
private:
	bool StartCap();
	AVCodecContext* GetCodecCtx();
	int OpenOutPut(const char* outFileName,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle);
	AVFrame* MergeFrame(AVFrame* frame1, AVFrame* frame2);
	int GetBestFps();
	//VideoCap* pVideoCap[2];
	//VideoCap* pVideoCap2;
	VideoCapList pVideoCaps;
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