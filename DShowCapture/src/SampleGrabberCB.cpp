#include "stdafx.h"
//#include "QDshowRecordToMp4.h"
#include "SampleGrabberCB.h"

CSampleGrabberCB::CSampleGrabberCB(void)
{
	m_bBeginEncode = FALSE; 
	m_tVidParam.pYUVBuf = NULL;
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

	if (m_nMediaType == 0)
	{
		if(m_tVidParam.pYUVBuf == NULL)
		{
			m_tVidParam.pYUVBuf = (unsigned char *) malloc(m_tVidParam.nWidth * m_tVidParam.nHeight * 3 / 2);
		}
		int src_stride_yuy2 = m_tVidParam.nWidth * 2;
		unsigned char * src_yuy2 =  pBuffer;

		
		int dst_stride_y = m_tVidParam.nWidth;
		int dst_stride_u = m_tVidParam.nWidth / 2;
		int dst_stride_v = m_tVidParam.nWidth / 2;		

		uint8_t *dst_y = m_tVidParam.pYUVBuf;
		uint8_t *dst_u = m_tVidParam.pYUVBuf + m_tVidParam.nWidth * m_tVidParam.nHeight;
		uint8_t *dst_v = m_tVidParam.pYUVBuf + m_tVidParam.nWidth * m_tVidParam.nHeight * 5 / 4;		
		libyuv::YUY2ToI420(src_yuy2, src_stride_yuy2, dst_y, dst_stride_y, dst_u, dst_stride_u, dst_v, dst_stride_v, m_tVidParam.nWidth, m_tVidParam.nHeight);

	}
	else
	{
	//	fwrite(pBuffer, BufferLen, 1, fp1);
	}


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


