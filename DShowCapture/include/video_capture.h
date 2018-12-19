#ifndef _VIDEO_CAPTURE_H_
#define _VIDEO_CAPTURE_H_

#include "stdafx.h"
#include <sys/timeb.h>
#include "SampleGrabberCB.h"

//用来存储视音频数据
struct GrabDataInfo
{
	BYTE *pData;
	int nDataSize;
	double dSampleTime;
	int nType;     //0为视频，1为音频

	GrabDataInfo()
	{
		pData = NULL;
		nDataSize = 0;
		dSampleTime = 0.0;
		nType = -1;
	};

	GrabDataInfo(const GrabDataInfo &other)
	{
		*this = other;
	};

	GrabDataInfo& operator = (const GrabDataInfo& other)
	{
		pData = other.pData;
		nDataSize = other.nDataSize;
		dSampleTime = other.dSampleTime;
		nType = other.nType;
		return *this;
	};
};
typedef CArray <GrabDataInfo, GrabDataInfo&> ASGrabDataInfoArray;

//相机设备信息
struct ImgDeviceInfo
{
	CString strDevicePidVid;		//设备PIDVID
	CString strDeviceName;			//设备名称
	int nDeviceIndex;				//设备序号

	ImgDeviceInfo()
	{
		strDevicePidVid = _T("");
		strDeviceName = _T("");
		nDeviceIndex = -1;
	};

	ImgDeviceInfo(const ImgDeviceInfo &other)
	{
		*this = other;
	};

	ImgDeviceInfo& operator = (const ImgDeviceInfo& other)
	{
		strDevicePidVid = other.strDevicePidVid;
		strDeviceName = other.strDeviceName;
		nDeviceIndex = other.nDeviceIndex;
		return *this;
	};
};
typedef CArray <ImgDeviceInfo, ImgDeviceInfo&> ASImgDeviceInfoArray;

//分辨率
struct CamResolutionInfo
{
	int nWidth;				//分辨率宽
	int nHeight;			//分辨率高
	int nResolutionIndex;	//分辨率序号
	CString strSubType;     //视频子类型

	CamResolutionInfo()
	{
		nWidth = 640;
		nHeight = 480;
		nResolutionIndex = -1;
		strSubType = _T("");
	};

	CamResolutionInfo(const CamResolutionInfo &other)
	{
		*this = other;
	};

	CamResolutionInfo& operator = (const CamResolutionInfo& other)
	{
		nWidth = other.nWidth;
		nHeight = other.nHeight;
		nResolutionIndex = other.nResolutionIndex;
		strSubType = other.strSubType;
		return *this;
	};
};
typedef CArray <CamResolutionInfo, CamResolutionInfo&> ASCamResolutionInfoArray;

class CVideoCapture
{
public:
	CVideoCapture();
	~CVideoCapture();

	void ListVideoCaptureDevices(ASImgDeviceInfoArray &VidDevInfo);
	void ListAudioCaptureDevices(ASImgDeviceInfoArray &VidDevInfo);
	void GetVideoResolution(ASCamResolutionInfoArray &VidResolution);
	bool InitCapture(CString AudDevName, CString VidDevName);
	bool StartCapture(int index, HWND hwnd);
	bool StopCapture();

	IGraphBuilder *m_pGraphBuilder;
	ICaptureGraphBuilder2* m_pCapture;
	IBaseFilter* m_pVideoFilter;
	IBaseFilter* m_pAudioFilter;
	IMediaControl  *m_pMediaControl;
	IVideoWindow* m_pVW;
	IBaseFilter* m_pVideoGrabberFilter;
	IBaseFilter* m_pAudioGrabberFilter;
	ISampleGrabber* m_pVideoGrabber;
	ISampleGrabber* m_pAudioGrabber;
	CSampleGrabberCB m_videoCB;
	CSampleGrabberCB m_audioCB;
	HWND m_hShowWnd;

private:
	void CreateVideoFilter(CString strSelectedDevice, IBaseFilter **pBaseFilter);
	void CreateAudioFilter(CString strSelectedDevice, IBaseFilter **pBaseFilter);
	HRESULT InitCaptureGraphBuilder(IGraphBuilder **ppGraph, ICaptureGraphBuilder2 **ppBuild);
	void FreeMediaType(AM_MEDIA_TYPE *pmt);
};

#endif