#ifndef RECORD_MUX_H
#define RECORD_MUX_H

class VideoCap;
class AudioCap;
class RecordMux
{
public:
	RecordMux();
	int Start(const char* filePath,VideoInfo* pVideoInfo, AudioInfo* pAudioInfo,SubTitleInfo* pSubTitle);
	int Stop();
	void Run();
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
};
#endif