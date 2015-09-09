#pragma once

#include <windows.h>
#include <dshow.h>

#include <vector>
#include <string>

#pragma comment(lib, "strmiids")

using namespace std;

class CaptureDevices
{
public:
	CaptureDevices();
	~CaptureDevices();

	HRESULT Enumerate();

	HRESULT GetVideoDevices(vector<wstring> *videoDevices);
	HRESULT GetAudioDevices(vector<wstring> *audioDevices);

	std::wstring getLastError();

private:
	std::wstring lastError;

	IEnumMoniker *enumMonikerVideo, *enumMonikerAudio;
};

