
#include <stdio.h>
#include <Windows.h>
#include   <time.h>   
#include <iostream>
#include <string>

#ifdef	__cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"

#ifdef __cplusplus
};
#endif
#include "Utils.h"

#ifdef   WIN32     
#define   LOCALTIME_R(t)     localtime((t))     
#else     
#define   LOCALTIME_R(t)     localtime_r((t),   (struct   tm   *)&tmres)     
#endif  


std::string get_log_filename()
{
	struct   tm   *tmNow;     
    time_t   long_time;    
	char log_path[256] = {0,};
    time(&long_time   );                             /*   Get   time   as   long   integer.   */     
    tmNow   =   LOCALTIME_R(   &long_time   );   /*   Convert   to   local   time.   */    

    _snprintf(log_path,256,"record_%04d%02d%02d%02d%02d%02d.log",tmNow->tm_year+1900,   tmNow->tm_mon   +   1,       
        tmNow->tm_mday,   tmNow->tm_hour,   tmNow->tm_min,   tmNow->tm_sec);     
	return log_path;
}
std::string GetProgramDir(HMODULE handle)  
{   
    char exeFullPath[MAX_PATH]; // Full path
    std::string strPath = "";
 
    GetModuleFileName(handle,exeFullPath,MAX_PATH);
    strPath=(std::string)exeFullPath;    // Get full path of the file
    int pos = strPath.find_last_of('\\', strPath.length());
    return strPath.substr(0, pos);  // Return the directory without the file name
}  

std::string GBKToUTF8(const char* strGBK)
{ 
	std::string tmp;
    int len=MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strGBK, -1, NULL,0); 
    unsigned short * wszUtf8 = new unsigned short[len+1]; 
    memset(wszUtf8, 0, len * 2 + 2); 
    MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strGBK, -1, (LPWSTR)wszUtf8, len);
    len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, NULL, 0, NULL, NULL);
    char *szUtf8=new char[len + 1]; 
    memset(szUtf8, 0, len + 1); 
    WideCharToMultiByte (CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, (LPSTR)szUtf8, len, NULL,NULL);
	tmp = szUtf8;
	if(szUtf8) delete []szUtf8;
	if(wszUtf8) delete []wszUtf8;
    return tmp; 
}
/*
utf8转gbk
*/
std::string UTF8ToGBK(const char* str)
{
     std::string result;
     WCHAR *strSrc;
     TCHAR *szRes;

     //获得临时变量的大小
     int i = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
     strSrc = new WCHAR[i+1];
     MultiByteToWideChar(CP_UTF8, 0, str, -1, strSrc, i);

     //获得临时变量的大小
     i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
     szRes = new TCHAR[i+1];
     WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

     result = szRes;
     delete []strSrc;
     delete []szRes;

     return result;
}



static void YUV420p_to_RGB24(unsigned char *yuv420[3], unsigned char *rgb24, int width, int height) 
{
//  int begin = GetTickCount();
	int R,G,B,Y,U,V;
	int x,y;
	int nWidth = width>>1; //色度信号宽度
	for (y=0;y<height;y++)
	{
	for (x=0;x<width;x++)
	{
	Y = *(yuv420[0] + y*width + x);
	U = *(yuv420[1] + ((y>>1)*nWidth) + (x>>1));
	V = *(yuv420[2] + ((y>>1)*nWidth) + (x>>1));
	R = Y + 1.402*(V-128);
	G = Y - 0.34414*(U-128) - 0.71414*(V-128);
	B = Y + 1.772*(U-128);

	//防止越界
	if (R>255)R=255;
	if (R<0)R=0;
	if (G>255)G=255;
	if (G<0)G=0;
	if (B>255)B=255;
	if (B<0)B=0;
   
	*(rgb24 + ((height-y-1)*width + x)*3) = B;
	*(rgb24 + ((height-y-1)*width + x)*3 + 1) = G;
	*(rgb24 + ((height-y-1)*width + x)*3 + 2) = R;
  }
 }
}

int GBKToUTF8_V2(const char * lpGBKStr, char * lpUTF8Str,int nUTF8StrLen)
{
    wchar_t * lpUnicodeStr = NULL;
    int nRetLen = 0;

    if(!lpGBKStr)  //如果GBK字符串为NULL则出错退出
        return 0;

    nRetLen = ::MultiByteToWideChar(CP_ACP,0,(char *)lpGBKStr,-1,NULL,NULL);  //获取转换到Unicode编码后所需要的字符空间长度
    lpUnicodeStr = new WCHAR[nRetLen + 1];  //为Unicode字符串空间
    nRetLen = ::MultiByteToWideChar(CP_ACP,0,(char *)lpGBKStr,-1,lpUnicodeStr,nRetLen);  //转换到Unicode编码
    if(!nRetLen)  //转换失败则出错退出
        return 0;

    nRetLen = ::WideCharToMultiByte(CP_UTF8,0,lpUnicodeStr,-1,NULL,0,NULL,NULL);  //获取转换到UTF8编码后所需要的字符空间长度
    
    if(!lpUTF8Str)  //输出缓冲区为空则返回转换后需要的空间大小
    {
        if(lpUnicodeStr)       
		delete []lpUnicodeStr;
        return nRetLen;
    }
    
    if(nUTF8StrLen < nRetLen)  //如果输出缓冲区长度不够则退出
    {
        if(lpUnicodeStr)
            delete []lpUnicodeStr;
        return 0;
    }

    nRetLen = ::WideCharToMultiByte(CP_UTF8,0,lpUnicodeStr,-1,(char *)lpUTF8Str,nUTF8StrLen,NULL,NULL);  //转换到UTF8编码
    
    if(lpUnicodeStr)
        delete []lpUnicodeStr;
    
    return nRetLen;
}

static FILE *report_file;
static int report_file_level = AV_LOG_DEBUG;
#define va_copy(dst, src) ((dst) = (src))
static void log_callback_report(void *ptr, int level, const char *fmt, va_list vl)
{
    va_list vl2;
    char line[1024];
    static int print_prefix = 1;

    va_copy(vl2, vl);
    av_log_default_callback(ptr, level, fmt, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
    va_end(vl2);
    if (report_file_level >= level) {
        fputs(line, report_file);
        fflush(report_file);
    }
}
struct log_def{
	const char* desc;
	int level;
};
static struct log_def loglist[] = {
	{"quiet",AV_LOG_QUIET},
	{"debug",AV_LOG_DEBUG},
	{"verbose",AV_LOG_VERBOSE},
	{"info",AV_LOG_INFO},
	{"warning",AV_LOG_WARNING},
	{"error",AV_LOG_ERROR},
	{"fatal",AV_LOG_FATAL},
	{"panic",AV_LOG_PANIC},
	
};
#define ARRAY_SIZE(a,b) (sizeof(a)/sizeof(b))
static int parse_log_level(const char* logdesc)
{
	
	for(int i = 0; i < ARRAY_SIZE(loglist,struct log_def);i++)
	{
		if(!strcmp(logdesc, loglist[i].desc))
		{
			return loglist[i].level;
		}
	}
	return AV_LOG_QUIET;
}
int init_report_file(std::string log_config,std::string log_file )
{
	char buf[32] = {0,};

	int num = GetPrivateProfileString("log","enable","0", buf,sizeof(buf), log_config.c_str());
	if(num < 0) return 0;
	if( 0 != strcmp(buf,"1"))
	{
		return 0;
	}
	memset(buf,0,32);
	num = GetPrivateProfileString("log","level","quiet", buf,sizeof(buf), log_config.c_str());
	
	report_file_level = parse_log_level(buf);
	
	report_file = fopen(log_file.c_str(), "w");
    if (!report_file) {
        int ret = AVERROR(errno);
        av_log(NULL, AV_LOG_ERROR, "Failed to open report \"%s\": %s\n",
               log_file.c_str(), strerror(errno));
        return 1;
    }
	av_log_set_callback(log_callback_report);
	av_log(NULL,AV_LOG_ERROR,"录像模块加载成功");
	return 0;
}
std::string ws2s(const std::wstring& ws)
{
    std::string curLocale = setlocale(LC_ALL, NULL);        // curLocale = "C";
    setlocale(LC_ALL, "chs");
    const wchar_t* _Source = ws.c_str();
    size_t _Dsize = 2 * ws.size() + 1;
    char *_Dest = new char[_Dsize];
    memset(_Dest,0,_Dsize);
    wcstombs(_Dest,_Source,_Dsize);
    std::string result = _Dest;
    delete []_Dest;
    setlocale(LC_ALL, curLocale.c_str());
    return result;
}

std::wstring s2ws(const std::string& s)
{
    setlocale(LC_ALL, "chs"); 
    const char* _Source = s.c_str();
    size_t _Dsize = s.size() + 1;
    wchar_t *_Dest = new wchar_t[_Dsize];
    wmemset(_Dest, 0, _Dsize);
    mbstowcs(_Dest,_Source,_Dsize);
    std::wstring result = _Dest;
    delete []_Dest;
    setlocale(LC_ALL, "C");
    return result;
}
AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height, int algin)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, algin);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}
AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        av_log(NULL,AV_LOG_ERROR, "Error allocating an audio frame\n");
        return NULL;
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            av_log(NULL,AV_LOG_ERROR, "Error allocating an audio buffer\n");
            return NULL;
        }
    }

    return frame;
}
std::string getDevicePath(const char* pDevType, const char* pDevName)
{
	char tmpbuf[256] = {0,};
	_snprintf(tmpbuf,256,"%s=%s",pDevType,pDevName);
	return GBKToUTF8(tmpbuf);
}

void dshow_dump_params(AVFormatContext	** ctx,const char* psDevName,AVInputFormat *ifmt)
{
	AVDictionary *options = NULL;
	av_log(NULL,AV_LOG_ERROR,"detect camera and audio device \r\n");
	if(av_opt_find(&(ifmt->priv_class),"list_options",NULL,0,AV_OPT_SEARCH_FAKE_OBJ))
	{
		av_dict_set_int(&options,"list_options",1,NULL);
	}
	if(avformat_open_input(ctx, psDevName, ifmt, &options)!=0)
	{
		
		
	}
	if(options!=NULL)
		av_dict_free(&options);
}
void dshow_dump_devices(AVFormatContext	** ctx,const char* psDevName,AVInputFormat *ifmt)
{
	AVDictionary *options = NULL;
	av_log(NULL,AV_LOG_ERROR,"detect camera and audio device \r\n");
	if(av_opt_find(&(ifmt->priv_class),"list_options",NULL,0,AV_OPT_SEARCH_FAKE_OBJ))
	{
		av_dict_set_int(&options,"list_devices",1,NULL);
	}
	if(avformat_open_input(ctx, psDevName, ifmt, &options)!=0)
	{
		
		
	}
	if(options!=NULL)
		av_dict_free(&options);
}
static int dshow_try_open_devices2(AVFormatContext	** ctx,const char* psDevName,AVInputFormat *ifmt,int width, int height, int fps,const char* fmt)
{
	AVDictionary *options = NULL;
	char buf[16] = {0,};
	_snprintf_s(buf,16,"%d",fps);
	
	av_dict_set(&options, "framerate", buf, NULL);
	memset(buf,0,16);
	_snprintf_s(buf,16,"%d",width*height*3*fps);
	av_dict_set(&options, "rtbufsize", buf, NULL);
	memset(buf,0,16);
	_snprintf_s(buf,16,"%dx%d",width,height);
	av_dict_set(&options, "video_size", buf, NULL);
	av_dict_set(&options, "pixel_format", fmt, NULL);
	if(avformat_open_input(ctx, psDevName, ifmt, &options)!=0)
		//if(avformat_open_input(&pFormatCtx_Video, psDevName, ifmt, NULL)!=0)
	{
		av_log(NULL,AV_LOG_ERROR,"Couldn't open input stream.（无法打开视频输入流）\n");
		return -1;
	}

	if(options!=NULL)
		av_dict_free(&options);
	return 0;
}
int dshow_try_open_devices(AVFormatContext	** ctx,const char* psDevName,AVInputFormat *ifmt,int width, int height, int fps,const char* fmt)
{
	int fps_arr[5] = {10,15,20,25,30};
	
	if(dshow_try_open_devices2(ctx,psDevName,ifmt,width,height,fps,fmt) == 0) return fps;
	av_log(NULL,AV_LOG_ERROR,"%d orig fps can not open\r\n",fps);
	for(int i = 10; i <= 30; i++)
	{
		
		if(dshow_try_open_devices2(ctx,psDevName,ifmt,width,height,i,fmt) == 0)
		{
			av_log(NULL,AV_LOG_ERROR,"%d fps open ok\r\n",i);
			return i;
		}
		av_log(NULL,AV_LOG_ERROR,"%d fps can not open\r\n",i);
	}
	return 0;
}

MyFile::MyFile(const char* file)
{
	Open(file);
}
MyFile::~MyFile()
{
	Close();
}
bool MyFile::Open(const char* file)
{
	_file = file;
	_fp = fopen(file,"wb+");
	if(_fp==NULL) return false;

	return true;
}
bool MyFile::Close()
{
	if(_fp)
	{
		fclose(_fp);
	}
	return true;
}
int  MyFile::FillBuffer(unsigned char val, int size)
{
	int rt = 0;
	for(int i = 0; i < size; i++)
	{
		rt += fwrite(&val,1,1,_fp);
	}
	return rt;
}
int  MyFile::FillTestBuffer()
{
	int i = 0;
	for(i = 0 ; i < 120; i++)
		FillBuffer(0x23,320);
	for(i = 0 ; i < 120; i++)
		FillBuffer(0x80,320);

	for(i = 0 ; i < 60; i++)
		FillBuffer(0x23,160);
	for(i = 0 ; i < 60; i++)
		FillBuffer(0x80,160);

	for(i = 0 ; i < 60; i++)
		FillBuffer(0x23,160);
	for(i = 0 ; i < 60; i++)
		FillBuffer(0x80,160);
	return 0;


}
int MyFile::WriteFrame(AVFrame* frame)
{
	int ret = 0;
	if(frame==NULL) return 0;
	switch(frame->format)
	{
		case AV_PIX_FMT_YUV420P:
			ret = WriteYUV420P(frame);
			break;
		case AV_PIX_FMT_RGB24:
		case AV_PIX_FMT_BGR24:
			ret = WriteRGB24(frame);
			break;
	}
	return ret;
}
int  MyFile::WriteBuffer(void* buffer, int size)
{
	return fwrite(buffer,size,1,_fp);
}
int  MyFile::WriteRGB24(AVFrame* frame)
{
	return WriteBuffer(frame->data[0], frame->width*frame->height);
}
int  MyFile::WriteYUV420P(AVFrame* frame)
{
	int y_size = frame->width*frame->height;
	int ret = 0;
	ret += WriteBuffer(frame->data[0], y_size);
	ret += WriteBuffer(frame->data[1], y_size/4);
	ret += WriteBuffer(frame->data[2], y_size/4);

	return ret;

}

bool MyFile::Open(const char* file,AVPixelFormat format, int width, int height,int fps)
{
	isReadOnly = true;
	_width = width;
	_height = height;

	_frameSize = avpicture_get_size(format, width, height);

	_fp = fopen(file,"rb");
	if(_fp == NULL) return false;
	

	_frame = alloc_picture(format,width,height,16);
	if(_frame == NULL) return false;
	
	_format = format;
	ReadBuffer(_buffer,320*240*1.5);
	return (_frame!=NULL); 
}
AVFrame*  MyFile::ReadFrame()
{
	bool rt = false;
	if(!isReadOnly) return NULL;

	switch(_format)
	{
		case AV_PIX_FMT_YUV420P:
			rt = ReadYUV420P(_frame);
			break;
		case AV_PIX_FMT_RGB24:
		case AV_PIX_FMT_BGR24:
			rt = ReadRGB24(_frame);
			break;
		default:
			return NULL;
			
	}
	if(!rt) return NULL;
	return _frame;
}

int  MyFile::ReadBuffer(void* buffer, int size)
{
	return fread(buffer,size,1,_fp);
}
bool  MyFile::ReadRGB24(AVFrame* frame)
{
	int ret = ReadBuffer(frame->data[0],frame->width*frame->height);
	return (ret == _frameSize);
}
bool MyFile::ReadYUV420P(AVFrame* frame)
{
	
	int ret = 0;

	memcpy(frame->data[0],_buffer,frame->width*frame->height);
	memcpy(frame->data[1],_buffer+frame->width*frame->height,frame->width*frame->height/4);
	memcpy(frame->data[2],_buffer+(frame->width*frame->height)+(frame->width*frame->height)/4,frame->width*frame->height/4);
	return true;



	fseek(_fp,0,SEEK_CUR);
	ret += ReadBuffer(frame->data[0],frame->width*frame->height);
	ret += ReadBuffer(frame->data[1],frame->width*frame->height/4);
	ret += ReadBuffer(frame->data[2],frame->width*frame->height/4);

	return (ret == _frameSize);

}