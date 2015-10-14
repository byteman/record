#pragma once


#include <dshow.h>

#include <vector>
#include <string>

#pragma comment(lib, "strmiids")

using namespace std;

struct Cap_Info
{
	Cap_Info()
	{
		pixel_format = AV_PIX_FMT_NONE;
		codec_id	 = AV_CODEC_ID_NONE;
		max_fps = 0;
		min_fps = 0;
		name.clear();
	}
	std::string name;
	enum AVPixelFormat pixel_format;
    enum AVCodecID codec_id;
	SIZE max_size;
	SIZE min_size;
    int  max_fps;
	int  min_fps;
};
struct My_Info
{
	My_Info()
	{
		pixel_format = AV_PIX_FMT_NONE;
		width = height= 0;
		fps = 0;
	}
	
	enum AVPixelFormat pixel_format;
	int width;
	int height;
	int fps;
};
class CaptureDevices
{
public:
	CaptureDevices();
	~CaptureDevices();

	HRESULT Enumerate();

	HRESULT GetVideoDevices(vector<wstring> *videoDevices);
	HRESULT GetAudioDevices(vector<wstring> *audioDevices);

	std::wstring getLastError();
	std::vector<Cap_Info> & GetVideoCaps();
	bool GetBestCap(std::string video_name,enum AVPixelFormat pixel_format , int width, int height, int fps,My_Info & info);
	int  GetBestFps(std::string video_name,enum AVPixelFormat  pixel_format ,int width, int height,int fps);
	HRESULT ListCapablities(const char* name, int skip);
	HRESULT ListDevices(const char* name, int skip);
private:
	bool IsOKRect(Cap_Info& info,SIZE sz);
	bool IsOKFps(Cap_Info& info,int fps);
	bool FindRightRect(enum AVPixelFormat pixel_format ,SIZE sz);
	bool FindBestCap(enum AVPixelFormat  pixel_format ,SIZE sz,int fps);
	int  FindBestFps(enum AVPixelFormat  pixel_format ,SIZE sz,int fps);
	
	int dshow_cycle_pins(AVFormatContext *avctx, enum dshowDeviceType devtype,
                 IBaseFilter *device_filter, IPin **ppin,std::string);
	void dshow_cycle_formats(AVFormatContext *avctx, enum dshowDeviceType devtype,
                    IPin *pin, int *pformat_set,std::string str);
	std::wstring lastError;

	IEnumMoniker *enumMonikerVideo, *enumMonikerAudio;
	std::vector<Cap_Info> Video_Caps;
};

