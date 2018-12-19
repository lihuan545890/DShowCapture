#include "stdafx.h"
//#include "QDshowRecordToMp4.h"
#include "SampleGrabberCB.h"


CSampleGrabberCB::CSampleGrabberCB(void)
{
	m_bBeginEncode = FALSE; 
}

CSampleGrabberCB::~CSampleGrabberCB(void)
{
}

ULONG STDMETHODCALLTYPE CSampleGrabberCB::AddRef() 
{ 
	return 2; 
}

ULONG STDMETHODCALLTYPE CSampleGrabberCB::Release() 
{ 
	return 1; 
}

HRESULT STDMETHODCALLTYPE CSampleGrabberCB::QueryInterface(REFIID riid, void ** ppv)
{
	if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown )
	{ 
		*ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
		return NOERROR;
	} 
        
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CSampleGrabberCB::SampleCB(double SampleTime, IMediaSample *pSample)
{

	return 0;
}
 
//FILE *fp = fopen("d:\\yuv\\test.yuv", "wb");
//FILE *fp1 = fopen("d:\\yuv\\test.pcm", "wb");
HRESULT STDMETHODCALLTYPE CSampleGrabberCB::BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen)
{
	//开始编码，也是开始录制
/*	TRACE("cb data BufferLen:%d\n", BufferLen);
	if (m_nMediaType == 0)
	{
		fwrite(pBuffer, BufferLen, 1, fp);
	}
	else
	{
		fwrite(pBuffer, BufferLen, 1, fp1);
	}
*/

	if (m_bBeginEncode)
	{
		//timeb curTime;
		//ftime(&curTime);
		
		BYTE* pByte = new BYTE[BufferLen];
		memcpy(pByte, pBuffer, BufferLen);
		/*
		GrabDataInfo sData;
		sData.pData = pByte;
		sData.nDataSize = BufferLen;
		sData.dSampleTime = SampleTime; //curTime.time + ((double)(curTime.millitm) / 1000.0);
		sData.nType = 0;

		theApp.m_mxGlobalMutex.Lock();
		theApp.m_arrGrabData.Add(sData);
		theApp.m_mxGlobalMutex.Unlock();
		
		CString str;
		str.Format(_T("\n Video--BufferLen:%ld, SampleTime:%f "), BufferLen, sData.dSampleTime);
		OutputDebugString(str);*/
	}

	return 0;
}


