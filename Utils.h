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
int dshow_try_open_devices(AVFormatContext	** ctx,const char* psDevName,AVInputFormat *ifmt,int width, int height, int fps);
#endif