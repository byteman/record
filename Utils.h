#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <Windows.h>

std::string get_log_filename();
std::string GetProgramDir(HMODULE handle)  ;
std::string GBKToUTF8(const char* strGBK);
int init_report_file(std::string log_config,std::string log_file );
int GBKToUTF8_V2(const char * lpGBKStr, char * lpUTF8Str,int nUTF8StrLen);
AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height, int algin);
AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples);
std::string ws2s(const std::wstring& ws);
std::wstring s2ws(const std::string& s);
std::string getDevicePath(const char* pDevType, const char* pDevName);

bool resetDevciesString(int num);

void dshow_dump_params(AVFormatContext	** ctx,const char* psDevName,AVInputFormat *ifmt);
void dshow_dump_devices(AVFormatContext	** ctx,const char* psDevName,AVInputFormat *ifmt);
int dshow_try_open_devices(AVFormatContext	** ctx,const char* psDevName,AVInputFormat *ifmt,int width, int height, int fps,const char* fmt);


class MyFile
{
public:
	MyFile():
		_fp(NULL)
	{
		_file = "";
		isReadOnly = false;
		_frame = NULL;
		_frameSize = 0;
		_format = AV_PIX_FMT_NONE;
	}
	~MyFile();
	MyFile(const char* file);
	bool Open(const char* file);
	bool Open(const char* file,AVPixelFormat format, int width, int height,int fps);
	bool Close();
	int  FillBuffer(unsigned char val, int size);
	int	 WriteFrame(AVFrame* frame);
	int  WriteBuffer(void* buffer, int size);
	int  WriteRGB24(AVFrame* frame);
	int  WriteYUV420P(AVFrame* frame);
	AVFrame*  ReadFrame();
	int  ReadBuffer(void* buffer, int size);
	bool  ReadRGB24(AVFrame* frame);
	bool  ReadYUV420P(AVFrame* frame);
	
private:
	std::string _file;
	FILE* _fp;
	unsigned char *_buffer;
	int _width,_height,_fps;
	bool isReadOnly;
	AVFrame* _frame;
	int _frameSize;
	AVPixelFormat _format;
};
#endif