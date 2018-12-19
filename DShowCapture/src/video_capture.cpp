#include "video_capture.h"
#include <afx.h>  

CVideoCapture::CVideoCapture()
{
	m_pVideoFilter = NULL;
	m_pAudioFilter = NULL;
	m_pVideoGrabber = NULL;
	m_pAudioGrabber = NULL;
}

CVideoCapture::~CVideoCapture()
{

}

bool CVideoCapture::InitCapture(CString VidDevName, CString AudDevName)
{
	//选择设备，并获取Video Capture Filter
	CreateVideoFilter(VidDevName, &m_pVideoFilter);
	if (m_pVideoFilter == NULL)
	{
		return false;
	}
	//选择设备，并获取Audio Capture Filter
	CreateAudioFilter(AudDevName, &m_pAudioFilter);
	if (m_pAudioFilter == NULL)
	{
		return false;
	}

	//创建采集过滤器图表
	HRESULT hr = InitCaptureGraphBuilder(&m_pGraphBuilder, &m_pCapture);
	if (S_OK != hr || m_pGraphBuilder == NULL)
	{
		return false;
	}

	//创建视频捕捉实例 
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pVideoGrabberFilter);
	if (m_pVideoGrabberFilter == NULL)
	{
		return false;
	}

	//创建音频捕捉实例
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&m_pAudioGrabberFilter);
	if (m_pAudioGrabberFilter == NULL)
	{
		return false;
	}

	//将获取到的采集过滤器加入图表
	hr = m_pGraphBuilder->AddFilter(m_pVideoFilter, L"Video Capture Filter");
	if (S_OK != hr)
	{
		return false;
	}

	//关闭自动曝光  
	IAMCameraControl *m_pCtrl = NULL;
	long nValue = -5, nFlag = 1;
	m_pVideoFilter->QueryInterface(IID_IAMCameraControl, (void **)&m_pCtrl);
	if (m_pCtrl)
	{
		m_pCtrl->Get(CameraControl_Exposure, &nValue, &nFlag);
		m_pCtrl->Set(CameraControl_Exposure, nValue, CameraControl_Flags_Manual);
	}

	hr = m_pGraphBuilder->AddFilter(m_pAudioFilter, L"Audio Capture Filter");
	if (S_OK != hr)
	{
		return false;
	}

	//将视频捕捉过滤器加入图表
	hr = m_pGraphBuilder->AddFilter(m_pVideoGrabberFilter, L"Video Grabber");
	if (S_OK != hr)
	{
		return false;
	}

	//将音频捕捉过滤器加入图表
	hr = m_pGraphBuilder->AddFilter(m_pAudioGrabberFilter, L"Audio Grabber");
	if (S_OK != hr)
	{
		return false;
	}

	return true;
}

bool CVideoCapture::StartCapture(int index, HWND hwnd)
{
	IAMStreamConfig *pConfig = NULL;
	m_pCapture->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
		m_pVideoFilter, IID_IAMStreamConfig, (void **)&pConfig);

	AM_MEDIA_TYPE *pmt = NULL;
	VIDEO_STREAM_CONFIG_CAPS scc;
	pConfig->GetStreamCaps(index, &pmt, (BYTE*)&scc); //nResolutionIndex就是选择的分辨率序号

	//这里仅以采集源中的两种做例子（YUY2和RGB），一般的摄像头都是支持YUY2的

	if (pmt->subtype == MEDIASUBTYPE_YUY2)
	{
		pmt->majortype = MEDIATYPE_Video;
		pmt->subtype = MEDIASUBTYPE_YUY2;
		pmt->formattype = FORMAT_VideoInfo;

		pConfig->SetFormat(pmt);
		TRACE("Support Type: YUY2\n");
	}
	else  //如果不是YUY2，则默认为RGB24，需要摄像头支持RGB24，否则只能针对支持的类型做处理
	{
	//	m_nSourceVideoType = 0;

		pmt->majortype = MEDIATYPE_Video;
		pmt->subtype = MEDIASUBTYPE_RGB24;  //抓取RGB24
		pmt->formattype = FORMAT_VideoInfo;

		pConfig->SetFormat(pmt);
		TRACE("Support Type: RGB24\n");
	}

	m_pVideoGrabberFilter->QueryInterface(IID_ISampleGrabber, (void **)&m_pVideoGrabber);
	HRESULT hr = m_pVideoGrabber->SetMediaType(pmt);
	if (FAILED(hr))
	{
		//AfxMessageBox(_T("Fail to set media type!"));
		return false;
	}
	//是否缓存数据，缓存的话，可以给后面做其他处理，不缓存的话，图像处理就放在回调中
	m_pVideoGrabber->SetBufferSamples(FALSE);
	m_pVideoGrabber->SetOneShot(FALSE);
	m_pVideoGrabber->SetCallback(&m_videoCB, 1);
	m_videoCB.m_nMediaType = 0;

	//设置音频抓取数据
	m_pAudioGrabberFilter->QueryInterface(IID_ISampleGrabber, (void **)&m_pAudioGrabber);

	//获取音频采集源的相关参数
	IAMStreamConfig *pAudioConfig = NULL;
	m_pCapture->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio,
		m_pAudioFilter, IID_IAMStreamConfig, (void **)&pAudioConfig);

	AM_MEDIA_TYPE *audiPmt = NULL;
	AUDIO_STREAM_CONFIG_CAPS ascc;
	pAudioConfig->GetStreamCaps(0, &audiPmt, (BYTE*)&ascc);

	WAVEFORMATEX *pVih = (WAVEFORMATEX*)audiPmt->pbFormat;
	//pVih->nSamplesPerSec = 4096;
	//这里获取到音频采样率、通道数
	//m_audioCB.m_nChannels = pVih->nChannels;
	//m_audioCB.m_nSamplesPerSec = pVih->nSamplesPerSec;
	//m_audioCB.m_wBitsPerSample = pVih->wBitsPerSample;

	//-----------
//	m_nChannels = pVih->nChannels;
//	m_nSamplesPerSec = pVih->nSamplesPerSec;
//	m_wBitsPerSample = pVih->wBitsPerSample;
	//----------------

	//audiPmt->cbFormat = sizeof(WAVEFORMATEX);
	//audiPmt->pbFormat = (BYTE*)pVih;
	audiPmt->majortype = MEDIATYPE_Audio;//MEDIATYPE_Video
	audiPmt->subtype = MEDIASUBTYPE_PCM;//MEDIASUBTYPE_RGB24
	audiPmt->formattype = FORMAT_WaveFormatEx;//视频采集时没设置这一项

	pAudioConfig->SetFormat(audiPmt);
	hr = m_pAudioGrabber->SetMediaType(audiPmt);
	if (FAILED(hr))
	{
		AfxMessageBox(_T("Fail to set audio media type!"));
		return false;
	}

	m_pAudioGrabber->SetBufferSamples(FALSE);
	m_pAudioGrabber->SetOneShot(FALSE);
	m_pAudioGrabber->SetCallback(&m_audioCB, 1);
	m_audioCB.m_nMediaType = 1;

	hr = m_pCapture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pVideoFilter, m_pVideoGrabberFilter, NULL);
	if (FAILED(hr))
	{
	//	AfxMessageBox(_T("Video RenderStream failed"));
		return false;
	}
	
	hr = m_pCapture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, m_pAudioFilter, m_pAudioGrabberFilter, NULL);
	if (FAILED(hr))
	{
		return false;
	}
	
	FreeMediaType(pmt);
	FreeMediaType(audiPmt);

	//m_pVW->put_Visible(OATRUE);

	//如果不想要视频预览，则请注释上方显示的代码，使用下面这样的设置
	m_pVW->put_AutoShow(OAFALSE);
	m_pVW->put_Visible(OAFALSE);

	m_pMediaControl->Run();

	hr = m_pVW->put_Owner((OAHWND)hwnd);
	if (FAILED(hr))
		return false;

	hr = m_pVW->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
	if (FAILED(hr))
		return false;


	long nFrameWidth = 0, nFrameHeight = 0;
	m_pVW->get_Width(&nFrameWidth);
	m_pVW->get_Height(&nFrameHeight);

	//图像显示位置,下面的写法是为了让视频显示在当前picture控件的正中央
	if (m_pVW)
	{
		CRect rc;
		::GetClientRect(hwnd, &rc);
		double fWndWidth = rc.Width();
		double fWndHeight = rc.Height();
		double fWndScale = fWndWidth / fWndHeight;

		double fFrameWidth = nFrameWidth;
		double fFrameHeight = nFrameHeight;
		double fFrameScale = fFrameWidth / fFrameHeight;

		int nShowWidth = fWndWidth;
		int nShowHeight = fWndHeight;
		int xPos = 0, yPos = 0;
		if (fWndScale >= fFrameScale)  //控件窗口宽高比例比视频的宽高大
		{
			if (fWndHeight <= fFrameHeight)
			{
				nShowHeight = fWndHeight;
				nShowWidth = nFrameWidth*fWndHeight / nFrameHeight;
			}
			else
			{
				nShowHeight = fFrameHeight;
				nShowWidth = nFrameWidth;
			}
		}
		else
		{
			if (fWndWidth <= fFrameWidth)
			{
				nShowWidth = fWndWidth;
				nShowHeight = fWndWidth*nFrameHeight / nFrameWidth;
			}
			else
			{
				nShowHeight = fFrameHeight;
				nShowWidth = nFrameWidth;
			}
		}

		xPos = (fWndWidth - nShowWidth) / 2;
		yPos = (fWndHeight - nShowHeight) / 2;

		m_pVW->SetWindowPosition(xPos, yPos, nShowWidth, nShowHeight);

		m_pVW->put_Visible(OATRUE);
	}

	return true;
}

bool CVideoCapture::StopCapture()
{
	return true;
}

void CVideoCapture::ListVideoCaptureDevices(ASImgDeviceInfoArray &VidDevInfo)
{
	int count = 0;

	ImgDeviceInfo sDevice;
	// enumerate all video capture devices
	ICreateDevEnum *pCreateDevEnum = NULL;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void**)&pCreateDevEnum);

	IEnumMoniker *pEm = NULL;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
//	if (hr != NOERROR)
//		return ;

	ULONG cFetched;
	IMoniker *pM = NULL;
	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
	{
		sDevice.nDeviceIndex = count;

		LPOLESTR strPidvid = NULL;
		hr = pM->GetDisplayName(0, 0, &strPidvid);  //获取ID信息
		if (SUCCEEDED(hr))
		{
			//这里获取了一下设备的ID
			USES_CONVERSION; //OLE2T
			CString sPidvid = strPidvid;
			string str = T2A(sPidvid);
			string result;
			static const regex re("(vid_[0-9a-f]{4}&pid_[0-9a-f]{4})", regex::icase);
			smatch match;
			if (regex_search(str, match, re) && match.size() > 1)
			{
				result = match.str(1);
			}
			else
			{
				result = string("");
			}
			CString strPid(result.c_str());
			strPid.MakeUpper(); //全部大写
			sDevice.strDevicePidVid = strPid;

			IPropertyBag *pBag = 0;
			hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
			if (SUCCEEDED(hr))
			{
				VARIANT var;
				var.vt = VT_BSTR;
				hr = pBag->Read(L"FriendlyName", &var, NULL); //还有其他属性,像描述信息等等...
				if (hr == NOERROR)
				{
					//获取设备名称	
					char camera_name[1024];
					WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, camera_name, sizeof(camera_name), "", NULL);

					CString str(camera_name);
					sDevice.strDeviceName = str;
					VidDevInfo.Add(sDevice);

					SysFreeString(var.bstrVal);
				}
				pBag->Release();
			}
		}
		pM->Release();

	}

	pEm->Release();
	pCreateDevEnum->Release();

}

void CVideoCapture::ListAudioCaptureDevices(ASImgDeviceInfoArray &AudDevInfo)
{
	ImgDeviceInfo sDevice;
	int count = 0;
	// enumerate all Audio capture devices
	ICreateDevEnum *pCreateDevEnum = NULL;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void**)&pCreateDevEnum);

	IEnumMoniker *pEm = NULL;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEm, 0);
//	if (hr != NOERROR)
//		return 0;

	ULONG cFetched;
	IMoniker *pM = NULL;
	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
	{
		sDevice.nDeviceIndex = count;

		LPOLESTR strPidvid = NULL;
		hr = pM->GetDisplayName(0, 0, &strPidvid);  //获取ID信息
		if (SUCCEEDED(hr))
		{
			USES_CONVERSION; //OLE2T
			CString sPidvid = strPidvid;
			string str = T2A(sPidvid);
			string result;
			static const regex re("(vid_[0-9a-f]{4}&pid_[0-9a-f]{4})", regex::icase);
			smatch match;
			if (regex_search(str, match, re) && match.size() > 1)
			{
				result = match.str(1);
			}
			else
			{
				result = string("");
			}
			CString strPid(result.c_str());
			strPid.MakeUpper(); //全部大写
			sDevice.strDevicePidVid = strPid;

			IPropertyBag *pBag = 0;
			hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
			if (SUCCEEDED(hr))
			{
				VARIANT var;
				var.vt = VT_BSTR;
				hr = pBag->Read(L"FriendlyName", &var, NULL); //还有其他属性,像描述信息等等...
				if (hr == NOERROR)
				{
					//获取设备名称	
					char camera_name[1024];
					WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, camera_name, sizeof(camera_name), "", NULL);
					CString str(camera_name);
					sDevice.strDeviceName = str;
					AudDevInfo.Add(sDevice);

					SysFreeString(var.bstrVal);
				}
				pBag->Release();
			}
		}
		pM->Release();
		count++;
	}

	pEm->Release();
	pCreateDevEnum->Release();
}

void CVideoCapture::GetVideoResolution(ASCamResolutionInfoArray &VidResolution)
{
	if (m_pCapture)
	{
		IAMStreamConfig *pConfig = NULL;
		//&MEDIATYPE_Video,如果包括其他媒体类型，第二个参数设置为0
		HRESULT hr = m_pCapture->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
			m_pVideoFilter, IID_IAMStreamConfig, (void **)&pConfig);

		int iCount = 0, iSize = 0;
		hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);
		// Check the size to make sure we pass in the correct structure.
		if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
		{
			// Use the video capabilities structure.
			for (int iFormat = 0; iFormat < iCount; iFormat++)
			{
				VIDEO_STREAM_CONFIG_CAPS scc;
				AM_MEDIA_TYPE *pmtConfig = NULL;
				hr = pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
				if (SUCCEEDED(hr))
				{
					//(pmtConfig->subtype == MEDIASUBTYPE_RGB24) &&
					if ((pmtConfig->majortype == MEDIATYPE_Video) &&
						(pmtConfig->formattype == FORMAT_VideoInfo) &&
						(pmtConfig->cbFormat >= sizeof (VIDEOINFOHEADER)) &&
						(pmtConfig->pbFormat != NULL))
					{
						VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmtConfig->pbFormat;
						// pVih contains the detailed format information.
						LONG lWidth = pVih->bmiHeader.biWidth;
						LONG lHeight = pVih->bmiHeader.biHeight;
						BOOL bFind = FALSE;
						//是否已经存在这个分辨率，不存在就加入array
						for (int n = 0; n < VidResolution.GetSize(); n++)
						{
							CamResolutionInfo sInfo = VidResolution.GetAt(n);
							if (sInfo.nWidth == lWidth && sInfo.nHeight == lHeight)
							{
								bFind = TRUE;
								break;
							}
						}
						if (!bFind)
						{
							CamResolutionInfo camInfo;
							camInfo.nResolutionIndex = iFormat;
							camInfo.nWidth = lWidth;
							camInfo.nHeight = lHeight;
						

							CString strSubType = _T("");
							if (MEDIASUBTYPE_RGB24 == pmtConfig->subtype)
							{
								strSubType = _T("RGB24");
							}
							else if (MEDIASUBTYPE_RGB555 == pmtConfig->subtype)
							{
								strSubType = _T("RGB555");
							}
							else if (MEDIASUBTYPE_RGB32 == pmtConfig->subtype)
							{
								strSubType = _T("RGB32");
							}
							else if (MEDIASUBTYPE_RGB565 == pmtConfig->subtype)
							{
								strSubType = _T("RGB565");
							}
							else if (MEDIASUBTYPE_RGB8 == pmtConfig->subtype)
							{
								strSubType = _T("RGB8");
							}
							else if (MEDIASUBTYPE_IJPG == pmtConfig->subtype)
							{
								strSubType = _T("IJPG");
							}
							else if (MEDIASUBTYPE_YUY2 == pmtConfig->subtype)
							{
								strSubType = _T("YUY2");
							}
							else if (MEDIASUBTYPE_YUYV == pmtConfig->subtype)
							{
								strSubType = _T("YUYV");
							}
							else if (MEDIASUBTYPE_H264 == pmtConfig->subtype)
							{
								strSubType = _T("H264");
							}
							else if (MEDIASUBTYPE_MJPG == pmtConfig->subtype)
							{
								strSubType = _T("MJPG");
							}
							else if (MEDIASUBTYPE_Y41P == pmtConfig->subtype)
							{
								strSubType = _T("Y41P");
							}
							else
							{
								strSubType = _T("其他");
							}

							camInfo.strSubType = strSubType;
							VidResolution.Add(camInfo);

						}
					}

					// Delete the media type when you are done.
					FreeMediaType(pmtConfig);
				}
			}
		}

	}
}

void CVideoCapture::CreateVideoFilter(CString strSelectedDevice, IBaseFilter **pBaseFilter)
{
	ICreateDevEnum *pCreateDevEnum = NULL;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void**)&pCreateDevEnum);

	IEnumMoniker *pEm = NULL;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	if (hr != NOERROR)
		return;

	ULONG cFetched;
	IMoniker *pM = NULL;
	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
	{
		IPropertyBag *pBag = 0;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if (SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL); //还有其他属性,像描述信息等等...
			if (hr == NOERROR)
			{
				//获取设备名称	
				char camera_name[1024];
				WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, camera_name, sizeof(camera_name), "", NULL);
				CString str(camera_name);
				SysFreeString(var.bstrVal);
				if (strSelectedDevice == str)
				{
					hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)pBaseFilter);
					if (FAILED(hr))
					{
						OutputDebugString(_T("BindToObject Failed"));
					}
				}
			}
			pBag->Release();
		}
		pM->Release();
	}

	pEm->Release();
	pCreateDevEnum->Release();
}

void CVideoCapture::CreateAudioFilter(CString strSelectedDevice, IBaseFilter **pBaseFilter)
{
	ICreateDevEnum *pCreateDevEnum = NULL;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void**)&pCreateDevEnum);

	IEnumMoniker *pEm = NULL;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEm, 0);
	if (hr != NOERROR)
		return;

	ULONG cFetched;
	IMoniker *pM = NULL;
	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
	{
		IPropertyBag *pBag = 0;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if (SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL); //还有其他属性,像描述信息等等...
			if (hr == NOERROR)
			{
				//获取设备名称	
				char camera_name[1024];
				WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, camera_name, sizeof(camera_name), "", NULL);
				CString str(camera_name);
				SysFreeString(var.bstrVal);
				if (strSelectedDevice == str)
				{
					hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)pBaseFilter);
					if (FAILED(hr))
					{
						OutputDebugString(_T("BindToObject Failed"));
					}
				}
			}
			pBag->Release();
		}
		pM->Release();
	}

	pEm->Release();
	pCreateDevEnum->Release();
}

HRESULT CVideoCapture::InitCaptureGraphBuilder(IGraphBuilder **ppGraph, ICaptureGraphBuilder2 **ppBuild)
{
	if (!ppGraph || !ppBuild)
	{
		return E_POINTER;
	}
	IGraphBuilder *pGraph = NULL;
	ICaptureGraphBuilder2 *pBuild = NULL;
	// Create the Capture Graph Builder.
	HRESULT hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&pBuild);
	if (SUCCEEDED(hr))
	{
		// Create the Filter Graph Manager.
		hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);
		if (SUCCEEDED(hr))
		{
			// Initialize the Capture Graph Builder.
			pBuild->SetFiltergraph(pGraph);  //将过滤器图表添加到管理器中

			// Return both interface pointers to the caller.
			*ppBuild = pBuild;
			*ppGraph = pGraph; // The caller must release both interfaces.

			pGraph->QueryInterface(IID_IMediaControl, (void **)&m_pMediaControl);
			pGraph->QueryInterface(IID_IVideoWindow, (void **)&m_pVW);

			return S_OK;
		}
		else
		{
			pBuild->Release();
		}
	}

	return hr; // Failed
}

void CVideoCapture::FreeMediaType(AM_MEDIA_TYPE *pmt)
{
	if (pmt == NULL)
	{
		return;
	}

	if (pmt->cbFormat != 0)
	{
		CoTaskMemFree((PVOID)pmt->pbFormat);
		// Strictly unnecessary but tidier
		pmt->cbFormat = 0;
		pmt->pbFormat = NULL;
	}

	if (pmt->pUnk != NULL)
	{
		pmt->pUnk->Release();
		pmt->pUnk = NULL;
	}
}

