#include "stdafx.h"
#include "CaptureDevices.h"


CaptureDevices::CaptureDevices()
{
	CoInitialize(0);
	Enumerate();
}


CaptureDevices::~CaptureDevices()
{
	enumMonikerAudio->Release();
	enumMonikerVideo->Release();
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
