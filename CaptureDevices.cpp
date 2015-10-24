#include "stdafx.h"
#include <atlbase.h>



#ifdef	__cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/audio_fifo.h"
#include "libswresample/swresample.h"
#include "libavfilter/avfiltergraph.h" 
#include "libavfilter/buffersink.h"  
#include "libavfilter/buffersrc.h" 
#include "libavutil/pixdesc.h"

#ifdef __cplusplus
};
#endif
#include "Utils.h"
#include "CaptureDevices.h"

#define RLS(x)	if (x) { x->Release(); x = NULL; }
#define ARRAY_NUM(arr)	(sizeof(arr)/sizeof(arr[0]))



CaptureDevices::CaptureDevices():
	enumMonikerVideo(NULL),
	enumMonikerAudio(NULL)
{
	
	CoInitialize(0);
	Enumerate();
}


CaptureDevices::~CaptureDevices()
{
	if(enumMonikerAudio)
		enumMonikerAudio->Release();
	enumMonikerAudio = NULL;
	if(enumMonikerVideo)
		enumMonikerVideo->Release();
	enumMonikerVideo = NULL;
	CoUninitialize();
}

HRESULT CaptureDevices::Enumerate()
{
	HRESULT hr = S_OK;

	ICreateDevEnum *enumDev;

	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&enumDev));

	if(FAILED(hr))
	{
		lastError = L"Could not create device enumerator";
		return hr;
	}

	hr = enumDev->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enumMonikerVideo, NULL);

	if (FAILED(hr))
	{
		printf("No video capture devices available");
	}

	hr = enumDev->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &enumMonikerAudio, NULL);

	if (FAILED(hr))
	{
		printf("No audio capture devices available");
	}
	
	enumDev->Release();

	return hr;
}

HRESULT CaptureDevices::GetVideoDevices(vector<wstring> *videoDevices)
{
	if (!enumMonikerVideo)
		return E_FAIL;

	IMoniker *pMoniker = NULL;
	wstring name;

	while (enumMonikerVideo->Next(1, &pMoniker, NULL) == S_OK)
	{
		IPropertyBag *pPropBag;
		HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if (FAILED(hr))
		{
			pMoniker->Release();
			continue;  
		} 

		VARIANT var;
		VariantInit(&var);

		hr = pPropBag->Read(L"FriendlyName", &var, 0);
		if (SUCCEEDED(hr))
		{
			name = var.bstrVal;
			VariantClear(&var); 
		}
		

		pPropBag->Release();
		pMoniker->Release();

		if (!name.empty())
			videoDevices->push_back(name);
	}
	return 0;
}

HRESULT CaptureDevices::GetAudioDevices(vector<wstring> *audioDevices)
{
	if (!enumMonikerAudio)
		return E_FAIL;

	IMoniker *pMoniker = NULL;
	wstring name;

	while (enumMonikerAudio->Next(1, &pMoniker, NULL) == S_OK)
	{
		IPropertyBag *pPropBag;
		HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if (FAILED(hr))
		{
			pMoniker->Release();
			continue;  
		} 

		VARIANT var;
		VariantInit(&var);

		hr = pPropBag->Read(L"FriendlyName", &var, 0);
		if (SUCCEEDED(hr))
		{
			name = var.bstrVal;
			VariantClear(&var); 
		}

		pPropBag->Release();
		pMoniker->Release();

		if (!name.empty())
			audioDevices->push_back(name);
	}
}
static char *dup_wchar_to_utf8(wchar_t *w)
{
    char *s = NULL;
    int l = WideCharToMultiByte(CP_UTF8, 0, w, -1, 0, 0, 0, 0);
    s = (char *)av_malloc(l);
    if (s)
        WideCharToMultiByte(CP_UTF8, 0, w, -1, s, l, 0, 0);
    return s;
}
HRESULT CaptureDevices::ListDevices(const char* name, int skip)
{
	IBaseFilter *device_filter = NULL;
    IEnumMoniker *classenum = NULL;
	ICreateDevEnum *devenum = NULL;
    IMoniker *m = NULL;
	int r;
	
	r = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(&devenum));

	r = devenum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                             (IEnumMoniker **) &classenum, 0);
    if (r != S_OK) {
      
        return AVERROR(EIO);
    }
	while (!device_filter && classenum->Next(1, &m, NULL) == S_OK) {
        IPropertyBag *bag = NULL;
		char *buf = NULL;
        VARIANT var;
		
		
		r = m->BindToStorage(0, 0, IID_PPV_ARGS(&bag));

        if (r != S_OK)
		{
			goto fail1;
		}

	
		var.vt = VT_BSTR;
		r = bag->Read(L"FriendlyName", &var, 0);
		if (r != S_OK)
		{
			goto fail1;
		}
		if(ws2s(var.bstrVal) != std::string(name))
		//buf = dup_wchar_to_utf8(var.bstrVal);
		//if(strcmp(buf,name))
		{
			goto fail1;
		}
		  //获取到一个BaseFilter
		if(!skip--)
			m->BindToObject(0, 0, IID_PPV_ARGS(&device_filter));
      
fail1:
        if (bag)
            bag->Release();
		if(m)
			m->Release();
		
    }
	classenum->Release();
	if(device_filter!=NULL)
		dshow_cycle_pins(NULL,(enum dshowDeviceType)0,device_filter,NULL,name);
	if(device_filter)
		device_filter->Release();
	devenum->Release();
 
}
HRESULT CaptureDevices::ListCapablities(const char* name, int skip)
{
	

	
	 ListDevices(name,skip);
	
	 return 0;
}

int CaptureDevices::dshow_cycle_pins(AVFormatContext *avctx, enum dshowDeviceType devtype,
                 IBaseFilter *device_filter, IPin **ppin,std::string str)
{
   
    IEnumPins *pins = 0;
    IPin *device_pin = NULL;
    IPin *pin;
    int r;
	
    const char *devtypename = "video";

    int set_format = 0; //是否需要按指定设置格式，否则只是列举格式
    int format_set = 0; //设置的结果，是否设置成功.

    r = device_filter->EnumPins(&pins);
    if (r != S_OK) {
        av_log(avctx, AV_LOG_ERROR, "Could not enumerate pins.\n");
        return AVERROR(EIO);
    }

    if (!ppin) {
        av_log(avctx, AV_LOG_INFO, "DirectShow %s device options\n",
               devtypename);
    }
    while (!device_pin && pins->Next(1, &pin, NULL) == S_OK) {
        IKsPropertySet *p = NULL;
        IEnumMediaTypes *types = NULL;
        PIN_INFO info = {0};
        AM_MEDIA_TYPE *type;
        GUID category;
        DWORD r2;

        pin->QueryPinInfo(&info);
        info.pFilter->Release();

        if (info.dir != PINDIR_OUTPUT)
            goto next;
		
        if(pin->QueryInterface(IID_IKsPropertySet, (void **) &p) != S_OK)
            goto next;
		
        if (p->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY,
                               NULL, 0, &category, sizeof(GUID), &r2) != S_OK)
            goto next;
        if (!IsEqualGUID(category, PIN_CATEGORY_CAPTURE))
            goto next;
#if 1
        if (!ppin) {
            char *buf = dup_wchar_to_utf8(info.achName);
            av_log(avctx, AV_LOG_INFO, " Pin \"%s\"\n", buf);
            av_free(buf);
            dshow_cycle_formats(avctx, devtype, pin, NULL,str);
            goto next;
        }
        if (set_format) {
            dshow_cycle_formats(avctx, devtype, pin, &format_set,str);
            if (!format_set) {
                goto next;
            }
        }


        if (pin->EnumMediaTypes(&types) != S_OK)
            goto next;

        types->Reset();
        while (!device_pin && types->Next(1, &type, NULL) == S_OK) {
            if (IsEqualGUID(type->majortype, MEDIATYPE_Video)) {
                device_pin = pin;
                goto next;
            }
            CoTaskMemFree(type);
        }
#endif
next:
        if (types)
            types->Release();
        if (p)
            p->Release();
        if (device_pin != pin)
            pin->Release();
    }

    pins->Release();

    if (ppin) {
        
        *ppin = device_pin;
    }

    return 0;
}
typedef struct PixelFormatTag {
    enum AVPixelFormat pix_fmt;
    unsigned int fourcc;
} PixelFormatTag;

enum AVPixelFormat avpriv_find_pix_fmt(const PixelFormatTag *tags,
                                       unsigned int fourcc)
{
    while (tags->pix_fmt >= 0) {
        if (tags->fourcc == fourcc)
            return tags->pix_fmt;
        tags++;
    }
    return AV_PIX_FMT_NONE;
}
const PixelFormatTag ff_raw_pix_fmt_tags[] = {
    { AV_PIX_FMT_YUV420P, MKTAG('I', '4', '2', '0') }, /* Planar formats */
    { AV_PIX_FMT_YUV420P, MKTAG('I', 'Y', 'U', 'V') },
    { AV_PIX_FMT_YUV420P, MKTAG('Y', 'V', '1', '2') },
    { AV_PIX_FMT_YUV410P, MKTAG('Y', 'U', 'V', '9') },
    { AV_PIX_FMT_YUV410P, MKTAG('Y', 'V', 'U', '9') },
    { AV_PIX_FMT_YUV411P, MKTAG('Y', '4', '1', 'B') },
    { AV_PIX_FMT_YUV422P, MKTAG('Y', '4', '2', 'B') },
    { AV_PIX_FMT_YUV422P, MKTAG('P', '4', '2', '2') },
    { AV_PIX_FMT_YUV422P, MKTAG('Y', 'V', '1', '6') },
    /* yuvjXXX formats are deprecated hacks specific to libav*,
       they are identical to yuvXXX  */
    { AV_PIX_FMT_YUVJ420P, MKTAG('I', '4', '2', '0') }, /* Planar formats */
    { AV_PIX_FMT_YUVJ420P, MKTAG('I', 'Y', 'U', 'V') },
    { AV_PIX_FMT_YUVJ420P, MKTAG('Y', 'V', '1', '2') },
    { AV_PIX_FMT_YUVJ422P, MKTAG('Y', '4', '2', 'B') },
    { AV_PIX_FMT_YUVJ422P, MKTAG('P', '4', '2', '2') },
    { AV_PIX_FMT_GRAY8,    MKTAG('Y', '8', '0', '0') },
    { AV_PIX_FMT_GRAY8,    MKTAG('Y', '8', ' ', ' ') },

    { AV_PIX_FMT_YUYV422, MKTAG('Y', 'U', 'Y', '2') }, /* Packed formats */
    { AV_PIX_FMT_YUYV422, MKTAG('Y', '4', '2', '2') },
    { AV_PIX_FMT_YUYV422, MKTAG('V', '4', '2', '2') },
    { AV_PIX_FMT_YUYV422, MKTAG('V', 'Y', 'U', 'Y') },
    { AV_PIX_FMT_YUYV422, MKTAG('Y', 'U', 'N', 'V') },
    { AV_PIX_FMT_YVYU422, MKTAG('Y', 'V', 'Y', 'U') }, /* Philips */
    { AV_PIX_FMT_UYVY422, MKTAG('U', 'Y', 'V', 'Y') },
    { AV_PIX_FMT_UYVY422, MKTAG('H', 'D', 'Y', 'C') },
    { AV_PIX_FMT_UYVY422, MKTAG('U', 'Y', 'N', 'V') },
    { AV_PIX_FMT_UYVY422, MKTAG('U', 'Y', 'N', 'Y') },
    { AV_PIX_FMT_UYVY422, MKTAG('u', 'y', 'v', '1') },
    { AV_PIX_FMT_UYVY422, MKTAG('2', 'V', 'u', '1') },
    { AV_PIX_FMT_UYVY422, MKTAG('A', 'V', 'R', 'n') }, /* Avid AVI Codec 1:1 */
    { AV_PIX_FMT_UYVY422, MKTAG('A', 'V', '1', 'x') }, /* Avid 1:1x */
    { AV_PIX_FMT_UYVY422, MKTAG('A', 'V', 'u', 'p') },
    { AV_PIX_FMT_UYVY422, MKTAG('V', 'D', 'T', 'Z') }, /* SoftLab-NSK VideoTizer */
    { AV_PIX_FMT_UYVY422, MKTAG('a', 'u', 'v', '2') },
    { AV_PIX_FMT_UYVY422, MKTAG('c', 'y', 'u', 'v') }, /* CYUV is also Creative YUV */
    { AV_PIX_FMT_UYYVYY411, MKTAG('Y', '4', '1', '1') },
    { AV_PIX_FMT_GRAY8,   MKTAG('G', 'R', 'E', 'Y') },
    { AV_PIX_FMT_NV12,    MKTAG('N', 'V', '1', '2') },
    { AV_PIX_FMT_NV21,    MKTAG('N', 'V', '2', '1') },

    /* nut */
    { AV_PIX_FMT_RGB555LE, MKTAG('R', 'G', 'B', 15) },
    { AV_PIX_FMT_BGR555LE, MKTAG('B', 'G', 'R', 15) },
    { AV_PIX_FMT_RGB565LE, MKTAG('R', 'G', 'B', 16) },
    { AV_PIX_FMT_BGR565LE, MKTAG('B', 'G', 'R', 16) },
    { AV_PIX_FMT_RGB555BE, MKTAG(15 , 'B', 'G', 'R') },
    { AV_PIX_FMT_BGR555BE, MKTAG(15 , 'R', 'G', 'B') },
    { AV_PIX_FMT_RGB565BE, MKTAG(16 , 'B', 'G', 'R') },
    { AV_PIX_FMT_BGR565BE, MKTAG(16 , 'R', 'G', 'B') },
    { AV_PIX_FMT_RGB444LE, MKTAG('R', 'G', 'B', 12) },
    { AV_PIX_FMT_BGR444LE, MKTAG('B', 'G', 'R', 12) },
    { AV_PIX_FMT_RGB444BE, MKTAG(12 , 'B', 'G', 'R') },
    { AV_PIX_FMT_BGR444BE, MKTAG(12 , 'R', 'G', 'B') },
    { AV_PIX_FMT_RGBA64LE, MKTAG('R', 'B', 'A', 64 ) },
    { AV_PIX_FMT_BGRA64LE, MKTAG('B', 'R', 'A', 64 ) },
    { AV_PIX_FMT_RGBA64BE, MKTAG(64 , 'R', 'B', 'A') },
    { AV_PIX_FMT_BGRA64BE, MKTAG(64 , 'B', 'R', 'A') },
    { AV_PIX_FMT_RGBA,     MKTAG('R', 'G', 'B', 'A') },
    { AV_PIX_FMT_RGB0,     MKTAG('R', 'G', 'B',  0 ) },
    { AV_PIX_FMT_BGRA,     MKTAG('B', 'G', 'R', 'A') },
    { AV_PIX_FMT_BGR0,     MKTAG('B', 'G', 'R',  0 ) },
    { AV_PIX_FMT_ABGR,     MKTAG('A', 'B', 'G', 'R') },
    { AV_PIX_FMT_0BGR,     MKTAG( 0 , 'B', 'G', 'R') },
    { AV_PIX_FMT_ARGB,     MKTAG('A', 'R', 'G', 'B') },
    { AV_PIX_FMT_0RGB,     MKTAG( 0 , 'R', 'G', 'B') },
    { AV_PIX_FMT_RGB24,    MKTAG('R', 'G', 'B', 24 ) },
    { AV_PIX_FMT_BGR24,    MKTAG('B', 'G', 'R', 24 ) },
    { AV_PIX_FMT_YUV411P,  MKTAG('4', '1', '1', 'P') },
    { AV_PIX_FMT_YUV422P,  MKTAG('4', '2', '2', 'P') },
    { AV_PIX_FMT_YUVJ422P, MKTAG('4', '2', '2', 'P') },
    { AV_PIX_FMT_YUV440P,  MKTAG('4', '4', '0', 'P') },
    { AV_PIX_FMT_YUVJ440P, MKTAG('4', '4', '0', 'P') },
    { AV_PIX_FMT_YUV444P,  MKTAG('4', '4', '4', 'P') },
    { AV_PIX_FMT_YUVJ444P, MKTAG('4', '4', '4', 'P') },
    { AV_PIX_FMT_MONOWHITE,MKTAG('B', '1', 'W', '0') },
    { AV_PIX_FMT_MONOBLACK,MKTAG('B', '0', 'W', '1') },
    { AV_PIX_FMT_BGR8,     MKTAG('B', 'G', 'R',  8 ) },
    { AV_PIX_FMT_RGB8,     MKTAG('R', 'G', 'B',  8 ) },
    { AV_PIX_FMT_BGR4,     MKTAG('B', 'G', 'R',  4 ) },
    { AV_PIX_FMT_RGB4,     MKTAG('R', 'G', 'B',  4 ) },
    { AV_PIX_FMT_RGB4_BYTE,MKTAG('B', '4', 'B', 'Y') },
    { AV_PIX_FMT_BGR4_BYTE,MKTAG('R', '4', 'B', 'Y') },
    { AV_PIX_FMT_RGB48LE,  MKTAG('R', 'G', 'B', 48 ) },
    { AV_PIX_FMT_RGB48BE,  MKTAG( 48, 'R', 'G', 'B') },
    { AV_PIX_FMT_BGR48LE,  MKTAG('B', 'G', 'R', 48 ) },
    { AV_PIX_FMT_BGR48BE,  MKTAG( 48, 'B', 'G', 'R') },
    { AV_PIX_FMT_GRAY16LE,    MKTAG('Y', '1',  0 , 16 ) },
    { AV_PIX_FMT_GRAY16BE,    MKTAG(16 ,  0 , '1', 'Y') },
    { AV_PIX_FMT_YUV420P10LE, MKTAG('Y', '3', 11 , 10 ) },
    { AV_PIX_FMT_YUV420P10BE, MKTAG(10 , 11 , '3', 'Y') },
    { AV_PIX_FMT_YUV422P10LE, MKTAG('Y', '3', 10 , 10 ) },
    { AV_PIX_FMT_YUV422P10BE, MKTAG(10 , 10 , '3', 'Y') },
    { AV_PIX_FMT_YUV444P10LE, MKTAG('Y', '3',  0 , 10 ) },
    { AV_PIX_FMT_YUV444P10BE, MKTAG(10 ,  0 , '3', 'Y') },
    { AV_PIX_FMT_YUV420P12LE, MKTAG('Y', '3', 11 , 12 ) },
    { AV_PIX_FMT_YUV420P12BE, MKTAG(12 , 11 , '3', 'Y') },
    { AV_PIX_FMT_YUV422P12LE, MKTAG('Y', '3', 10 , 12 ) },
    { AV_PIX_FMT_YUV422P12BE, MKTAG(12 , 10 , '3', 'Y') },
    { AV_PIX_FMT_YUV444P12LE, MKTAG('Y', '3',  0 , 12 ) },
    { AV_PIX_FMT_YUV444P12BE, MKTAG(12 ,  0 , '3', 'Y') },
    { AV_PIX_FMT_YUV420P14LE, MKTAG('Y', '3', 11 , 14 ) },
    { AV_PIX_FMT_YUV420P14BE, MKTAG(14 , 11 , '3', 'Y') },
    { AV_PIX_FMT_YUV422P14LE, MKTAG('Y', '3', 10 , 14 ) },
    { AV_PIX_FMT_YUV422P14BE, MKTAG(14 , 10 , '3', 'Y') },
    { AV_PIX_FMT_YUV444P14LE, MKTAG('Y', '3',  0 , 14 ) },
    { AV_PIX_FMT_YUV444P14BE, MKTAG(14 ,  0 , '3', 'Y') },
    { AV_PIX_FMT_YUV420P16LE, MKTAG('Y', '3', 11 , 16 ) },
    { AV_PIX_FMT_YUV420P16BE, MKTAG(16 , 11 , '3', 'Y') },
    { AV_PIX_FMT_YUV422P16LE, MKTAG('Y', '3', 10 , 16 ) },
    { AV_PIX_FMT_YUV422P16BE, MKTAG(16 , 10 , '3', 'Y') },
    { AV_PIX_FMT_YUV444P16LE, MKTAG('Y', '3',  0 , 16 ) },
    { AV_PIX_FMT_YUV444P16BE, MKTAG(16 ,  0 , '3', 'Y') },
    { AV_PIX_FMT_YUVA420P,    MKTAG('Y', '4', 11 ,  8 ) },
    { AV_PIX_FMT_YUVA422P,    MKTAG('Y', '4', 10 ,  8 ) },
    { AV_PIX_FMT_YUVA444P,    MKTAG('Y', '4',  0 ,  8 ) },
    { AV_PIX_FMT_YA8,         MKTAG('Y', '2',  0 ,  8 ) },

    { AV_PIX_FMT_YUVA420P9LE,  MKTAG('Y', '4', 11 ,  9 ) },
    { AV_PIX_FMT_YUVA420P9BE,  MKTAG( 9 , 11 , '4', 'Y') },
    { AV_PIX_FMT_YUVA422P9LE,  MKTAG('Y', '4', 10 ,  9 ) },
    { AV_PIX_FMT_YUVA422P9BE,  MKTAG( 9 , 10 , '4', 'Y') },
    { AV_PIX_FMT_YUVA444P9LE,  MKTAG('Y', '4',  0 ,  9 ) },
    { AV_PIX_FMT_YUVA444P9BE,  MKTAG( 9 ,  0 , '4', 'Y') },
    { AV_PIX_FMT_YUVA420P10LE, MKTAG('Y', '4', 11 , 10 ) },
    { AV_PIX_FMT_YUVA420P10BE, MKTAG(10 , 11 , '4', 'Y') },
    { AV_PIX_FMT_YUVA422P10LE, MKTAG('Y', '4', 10 , 10 ) },
    { AV_PIX_FMT_YUVA422P10BE, MKTAG(10 , 10 , '4', 'Y') },
    { AV_PIX_FMT_YUVA444P10LE, MKTAG('Y', '4',  0 , 10 ) },
    { AV_PIX_FMT_YUVA444P10BE, MKTAG(10 ,  0 , '4', 'Y') },
    { AV_PIX_FMT_YUVA420P16LE, MKTAG('Y', '4', 11 , 16 ) },
    { AV_PIX_FMT_YUVA420P16BE, MKTAG(16 , 11 , '4', 'Y') },
    { AV_PIX_FMT_YUVA422P16LE, MKTAG('Y', '4', 10 , 16 ) },
    { AV_PIX_FMT_YUVA422P16BE, MKTAG(16 , 10 , '4', 'Y') },
    { AV_PIX_FMT_YUVA444P16LE, MKTAG('Y', '4',  0 , 16 ) },
    { AV_PIX_FMT_YUVA444P16BE, MKTAG(16 ,  0 , '4', 'Y') },

    { AV_PIX_FMT_GBRP,         MKTAG('G', '3', 00 ,  8 ) },
    { AV_PIX_FMT_GBRP9LE,      MKTAG('G', '3', 00 ,  9 ) },
    { AV_PIX_FMT_GBRP9BE,      MKTAG( 9 , 00 , '3', 'G') },
    { AV_PIX_FMT_GBRP10LE,     MKTAG('G', '3', 00 , 10 ) },
    { AV_PIX_FMT_GBRP10BE,     MKTAG(10 , 00 , '3', 'G') },
    { AV_PIX_FMT_GBRP12LE,     MKTAG('G', '3', 00 , 12 ) },
    { AV_PIX_FMT_GBRP12BE,     MKTAG(12 , 00 , '3', 'G') },
    { AV_PIX_FMT_GBRP14LE,     MKTAG('G', '3', 00 , 14 ) },
    { AV_PIX_FMT_GBRP14BE,     MKTAG(14 , 00 , '3', 'G') },
    { AV_PIX_FMT_GBRP16LE,     MKTAG('G', '3', 00 , 16 ) },
    { AV_PIX_FMT_GBRP16BE,     MKTAG(16 , 00 , '3', 'G') },

    { AV_PIX_FMT_XYZ12LE,      MKTAG('X', 'Y', 'Z' , 36 ) },
    { AV_PIX_FMT_XYZ12BE,      MKTAG(36 , 'Z' , 'Y', 'X') },

    { AV_PIX_FMT_BAYER_BGGR8,    MKTAG(0xBA, 'B', 'G', 8   ) },
    { AV_PIX_FMT_BAYER_BGGR16LE, MKTAG(0xBA, 'B', 'G', 16  ) },
    { AV_PIX_FMT_BAYER_BGGR16BE, MKTAG(16  , 'G', 'B', 0xBA) },
    { AV_PIX_FMT_BAYER_RGGB8,    MKTAG(0xBA, 'R', 'G', 8   ) },
    { AV_PIX_FMT_BAYER_RGGB16LE, MKTAG(0xBA, 'R', 'G', 16  ) },
    { AV_PIX_FMT_BAYER_RGGB16BE, MKTAG(16  , 'G', 'R', 0xBA) },
    { AV_PIX_FMT_BAYER_GBRG8,    MKTAG(0xBA, 'G', 'B', 8   ) },
    { AV_PIX_FMT_BAYER_GBRG16LE, MKTAG(0xBA, 'G', 'B', 16  ) },
    { AV_PIX_FMT_BAYER_GBRG16BE, MKTAG(16,   'B', 'G', 0xBA) },
    { AV_PIX_FMT_BAYER_GRBG8,    MKTAG(0xBA, 'G', 'R', 8   ) },
    { AV_PIX_FMT_BAYER_GRBG16LE, MKTAG(0xBA, 'G', 'R', 16  ) },
    { AV_PIX_FMT_BAYER_GRBG16BE, MKTAG(16,   'R', 'G', 0xBA) },

    /* quicktime */
    { AV_PIX_FMT_YUV420P, MKTAG('R', '4', '2', '0') }, /* Radius DV YUV PAL */
    { AV_PIX_FMT_YUV411P, MKTAG('R', '4', '1', '1') }, /* Radius DV YUV NTSC */
    { AV_PIX_FMT_UYVY422, MKTAG('2', 'v', 'u', 'y') },
    { AV_PIX_FMT_UYVY422, MKTAG('2', 'V', 'u', 'y') },
    { AV_PIX_FMT_UYVY422, MKTAG('A', 'V', 'U', 'I') }, /* FIXME merge both fields */
    { AV_PIX_FMT_UYVY422, MKTAG('b', 'x', 'y', 'v') },
    { AV_PIX_FMT_YUYV422, MKTAG('y', 'u', 'v', '2') },
    { AV_PIX_FMT_YUYV422, MKTAG('y', 'u', 'v', 's') },
    { AV_PIX_FMT_YUYV422, MKTAG('D', 'V', 'O', 'O') }, /* Digital Voodoo SD 8 Bit */
    { AV_PIX_FMT_RGB555LE,MKTAG('L', '5', '5', '5') },
    { AV_PIX_FMT_RGB565LE,MKTAG('L', '5', '6', '5') },
    { AV_PIX_FMT_RGB565BE,MKTAG('B', '5', '6', '5') },
    { AV_PIX_FMT_BGR24,   MKTAG('2', '4', 'B', 'G') },
    { AV_PIX_FMT_BGR24,   MKTAG('b', 'x', 'b', 'g') },
    { AV_PIX_FMT_BGRA,    MKTAG('B', 'G', 'R', 'A') },
    { AV_PIX_FMT_RGBA,    MKTAG('R', 'G', 'B', 'A') },
    { AV_PIX_FMT_RGB24,   MKTAG('b', 'x', 'r', 'g') },
    { AV_PIX_FMT_ABGR,    MKTAG('A', 'B', 'G', 'R') },
    { AV_PIX_FMT_GRAY16BE,MKTAG('b', '1', '6', 'g') },
    { AV_PIX_FMT_RGB48BE, MKTAG('b', '4', '8', 'r') },

    /* special */
    { AV_PIX_FMT_RGB565LE,MKTAG( 3 ,  0 ,  0 ,  0 ) }, /* flipped RGB565LE */
    { AV_PIX_FMT_YUV444P, MKTAG('Y', 'V', '2', '4') }, /* YUV444P, swapped UV */

    { AV_PIX_FMT_NONE, 0 },
};

const struct PixelFormatTag *avpriv_get_raw_pix_fmt_tags(void)
{
    return ff_raw_pix_fmt_tags;
}

static enum AVPixelFormat dshow_pixfmt(DWORD biCompression, WORD biBitCount)
{
    switch(biCompression) {
    case BI_BITFIELDS:
    case BI_RGB:
        switch(biBitCount) { /* 1-8 are untested */
            case 1:
                return AV_PIX_FMT_MONOWHITE;
            case 4:
                return AV_PIX_FMT_RGB4;
            case 8:
                return AV_PIX_FMT_RGB8;
            case 16:
                return AV_PIX_FMT_RGB555;
            case 24:
                return AV_PIX_FMT_BGR24;
            case 32:
                return AV_PIX_FMT_0RGB32;
        }
    }
    return avpriv_find_pix_fmt(avpriv_get_raw_pix_fmt_tags(), biCompression); // all others
}

void CaptureDevices::dshow_cycle_formats(AVFormatContext *avctx, enum dshowDeviceType devtype,
                    IPin *pin, int *pformat_set,std::string str)
{
    //struct dshow_ctx *ctx = avctx->priv_data;
    IAMStreamConfig *config = NULL;
    AM_MEDIA_TYPE *type = NULL;
    int format_set = 0;
    void *caps = NULL;
    int i, n, size;
	Video_Caps.clear();
    if (pin->QueryInterface(IID_IAMStreamConfig, (void **) &config) != S_OK)
        return;
    if (config->GetNumberOfCapabilities(&n, &size) != S_OK)
        goto end;

    caps = av_malloc(size);
    if (!caps)
        goto end;

    for (i = 0; i < n && !format_set; i++) {
        config->GetStreamCaps(i, &type, (BYTE *) caps);
		
	#if DSHOWDEBUG
			ff_print_AM_MEDIA_TYPE(type);
	#endif

       
		VIDEO_STREAM_CONFIG_CAPS *vcaps = (VIDEO_STREAM_CONFIG_CAPS *)caps;
		BITMAPINFOHEADER *bih;
		int64_t *fr;
		Cap_Info info;
		const AVCodecTag *const tags[] = { avformat_get_riff_video_tags(), NULL };
	#if DSHOWDEBUG
		ff_print_VIDEO_STREAM_CONFIG_CAPS(vcaps);
	#endif
		if (IsEqualGUID(type->formattype, FORMAT_VideoInfo)) {
			VIDEOINFOHEADER *v = (VIDEOINFOHEADER *) type->pbFormat;
			fr = &v->AvgTimePerFrame;
			bih = &v->bmiHeader;
		} 
		/*
		else if (IsEqualGUID(type->formattype, FORMAT_VideoInfo2)) {
			VIDEOINFOHEADER2 *v = (VIDEOINFOHEADER2 *) type->pbFormat;
			fr = &v->AvgTimePerFrame;
			bih = &v->bmiHeader;
		} 
		*/
		else {
			goto next;
		}
		if (!pformat_set) {
			enum AVPixelFormat pix_fmt = dshow_pixfmt(bih->biCompression, bih->biBitCount);
			if (pix_fmt == AV_PIX_FMT_NONE) {
				enum AVCodecID codec_id = av_codec_get_id(tags, bih->biCompression);
				info.codec_id = codec_id;
				AVCodec *codec = avcodec_find_decoder(codec_id);
				if (codec_id == AV_CODEC_ID_NONE || !codec) {
					av_log(avctx, AV_LOG_INFO, "  unknown compression type 0x%X", (int) bih->biCompression);
				} else {
					av_log(avctx, AV_LOG_INFO, "  vcodec=%s", codec->name);
				}
			} else {
				av_log(avctx, AV_LOG_INFO, "  pixel_format=%s", av_get_pix_fmt_name(pix_fmt));
			}
			av_log(avctx, AV_LOG_INFO, "  min s=%ldx%ld fps=%g max s=%ldx%ld fps=%g\n",
					vcaps->MinOutputSize.cx, vcaps->MinOutputSize.cy,
					1e7 / vcaps->MaxFrameInterval,
					vcaps->MaxOutputSize.cx, vcaps->MaxOutputSize.cy,
					1e7 / vcaps->MinFrameInterval);
			info.name = str;
			info.pixel_format = pix_fmt;
			info.max_fps = 1e7 / vcaps->MinFrameInterval;
			info.min_fps = (1e7 / vcaps->MaxFrameInterval)+1;
			info.max_size.cx = vcaps->MinOutputSize.cx;
			info.max_size.cy = vcaps->MinOutputSize.cy;
			info.min_size.cx = vcaps->MinOutputSize.cx;
			info.min_size.cy = vcaps->MinOutputSize.cy;
			Video_Caps.push_back(info);
			continue;
		}
    
        
        if (config->SetFormat(type) != S_OK)
            goto next;
        format_set = 1;
next:
        if (type->pbFormat)
            CoTaskMemFree(type->pbFormat);
        CoTaskMemFree(type);
    }
end:
    config->Release();
    if (caps)
        av_free(caps);
    if (pformat_set)
        *pformat_set = format_set;
}
std::vector<Cap_Info> & CaptureDevices::GetVideoCaps()
{
	return Video_Caps;
}
bool CaptureDevices::IsOKRect(Cap_Info& info,SIZE sz)
{
	return (info.min_size.cx <= sz.cx)&&(info.min_size.cy <= sz.cy)&&(info.max_size.cx >= sz.cx)&&(info.max_size.cy >= sz.cy);
}
bool CaptureDevices::FindRightRect(enum AVPixelFormat pixel_format ,SIZE sz)
{
	bool find = false;
	for(int i = 0 ; i < Video_Caps.size(); i++)
	{
		if(Video_Caps[i].pixel_format == pixel_format)
		{
			if(!IsOKRect(Video_Caps[i],sz))
			{
				continue;
			}
			find = true;
			break;
		}
	}
	return find;
}
int  CaptureDevices::GetBestFps(std::string video_name,enum AVPixelFormat  pixel_format ,int width, int height,int fps)
{
	int ret = 10;
	for(int i = 0 ; i < Video_Caps.size(); i++)
	{
		if(Video_Caps[i].pixel_format == AV_PIX_FMT_NONE)
		{
			continue;
		}
		SIZE sz;
		sz.cx = width;
		sz.cy = height;
		ret = FindBestCap(Video_Caps[i].pixel_format,sz,fps);
		if( ret != 0)
		{
			return ret;
		}
		

	}
	return ret;
}
bool CaptureDevices::GetBestCap(std::string video_name,enum AVPixelFormat pixel_format , int width, int height, int fps,My_Info & info)
{
	bool find = false;
	for(int i = 0 ; i < Video_Caps.size(); i++)
	{
		if(video_name != Video_Caps[i].name)
		{
			continue;
		}
		if(Video_Caps[i].pixel_format == AV_PIX_FMT_NONE)
		{
			continue;
		}
		SIZE sz;
		sz.cx = width;
		sz.cy = height;
		if(!FindBestCap(Video_Caps[i].pixel_format,sz,fps))
		{
			//找不到
			int f = FindBestFps(Video_Caps[i].pixel_format,sz,fps);
			if(0!=f)
			{
				info.fps = f;
				info.height = height;
				info.width  = width;
				info.pixel_format = Video_Caps[i].pixel_format;
				find = true;
			}
		}
		else
		{
			info.fps = fps;
			info.height = height;
			info.width  = width;
			info.pixel_format = Video_Caps[i].pixel_format;
			find = true;
			break;
		}

	}

	return find;
}
bool CaptureDevices::IsOKFps(Cap_Info& info,int fps)
{
	return (fps >= info.min_fps && fps <= info.max_fps);
}
bool CaptureDevices::FindBestCap(enum AVPixelFormat  pixel_format ,SIZE sz,int fps)
{
	bool find = false;
	for(int i = 0 ; i < Video_Caps.size(); i++)
	{
		if( (Video_Caps[i].pixel_format != pixel_format) )
		{
			continue;
		}
		
		if(IsOKRect(Video_Caps[i],sz) && IsOKFps(Video_Caps[i],fps) )
		{
			find = true;
			break;
		}

	}

	return find;
}
int CaptureDevices::FindBestFps(enum AVPixelFormat  pixel_format ,SIZE sz,int fps)
{
	int find = 0;
	for(int i = 0 ; i < Video_Caps.size(); i++)
	{
		if( (Video_Caps[i].pixel_format != pixel_format) )
		{
			continue;
		}
		
		if(IsOKRect(Video_Caps[i],sz) )
		{
			find = Video_Caps[i].min_fps;
			break;
		}

	}

	return find;
}