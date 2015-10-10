#ifndef RECORD_MUX_H
#define RECORD_MUX_H

class VideoCap;
class AudioCap;
class RecordMux
{
public:
	RecordMux();
	int Init();
	int OpenCamera(const char* psDevName,const unsigned  int width,
													const unsigned  int height,
													const unsigned  int FrameRate,AVPixelFormat format, Video_Callback pCbFunc);

	int OpenAudio(const char * psDevName);
	int Start(const char* filePath,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle);
	int Stop();
	void Run();
	int Close();
	
private:
	int OpenOutPut(const char* outFileName,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle);
	VideoCap* pVideoCap;
	AudioCap* pAudioCap;
	AVFormatContext *pFmtContext;
	int VideoIndex,AudioIndex;
	bool bStartRecord,bCapture;
	int64_t cur_pts_v,cur_pts_a;
	int VideoFrameIndex,AudioFrameIndex;
	int frame_size,gYuv420FrameSize;
	CRITICAL_SECTION VideoSection;
	unsigned char* pEnc_yuv420p_buf;
	bool recordThreadQuit;
	AVInputFormat *pDShowInputFmt;
	
};
#endif