// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

extern int init_ffmpeg_env();
extern int CloseDevices();
extern void FreeAllRes();
static HMODULE hCrashDLL = 0;
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		hCrashDLL = LoadLibrary("CrashHelper.dll");
		init_ffmpeg_env();
		break;
	}
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		
		break;
	case DLL_PROCESS_DETACH:
		{
			if(hCrashDLL!=0)
				FreeLibrary(hCrashDLL);
			//FreeAllRes(); //如果加上了这句导致客户程序不能退出
		}
		break;
	}
	return TRUE;
}

