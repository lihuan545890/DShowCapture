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

		unsigned char * src_yuy2 =  pBuffer;

		
		int dst_stride_y = m_tVidParam.nWidth;
		int dst_stride_u = m_tVidParam.nWidth / 2;
		int dst_stride_v = m_tVidParam.nWidth / 2;		

		uint8_t *dst_y = m_tVidParam.pYUVBuf;
		uint8_t *dst_u = m_tVidParam.pYUVBuf + m_tVidParam.nWidth * m_tVidParam.nHeight;
		uint8_t *dst_v = m_tVidParam.pYUVBuf + m_tVidParam.nWidth * m_tVidParam.nHeight * 5 / 4;		

		if(m_tVidParam.nColorFormat == COLOR_FormatYUY2)
		{
			int src_stride_yuy2 = m_tVidParam.nWidth * 2;		
			int nRet = libyuv::YUY2ToI420(src_yuy2, src_stride_yuy2, dst_y, dst_stride_y, dst_u, dst_stride_u, dst_v, dst_stride_v, m_tVidParam.nWidth, m_tVidParam.nHeight);
			if (nRet == 0)
			{
				if (m_tVidParam.pYUVBuf != NULL)
				{
					StreamBuf buf;
					buf.frame = m_tVidParam.pYUVBuf;
					buf.bufsize = m_tVidParam.nWidth * m_tVidParam.nHeight * 3 / 2;

				    if(frame_queue_count(m_pVQueue) > 0)
				    {
						StreamBuf tmpBuf;
					    frame_queue_get(m_pVQueue, &tmpBuf, 1);
				    }

				    frame_queue_put(m_pVQueue, &buf);
				    
				}

			}			
		}
		else if(m_tVidParam.nColorFormat == COLOR_FormatRGB24)
		{
			int src_stride_rgb24 = m_tVidParam.nWidth * 3;	
			if (src_yuy2 != NULL)
			{
				int nRet = libyuv::RGB24ToI420(src_yuy2, src_stride_rgb24, dst_y, dst_stride_y, dst_u, dst_stride_u, dst_v, dst_stride_v, m_tVidParam.nWidth, m_tVidParam.nHeight);
				if (nRet == 0)
				{
					if (m_tVidParam.pYUVBuf != NULL)
					{
						StreamBuf buf;
						buf.frame = m_tVidParam.pYUVBuf;
						buf.bufsize = m_tVidParam.nWidth * m_tVidParam.nHeight * 3 / 2;

					    frame_queue_put(m_pVQueue, &buf);
					}

				}
			}


		}

	}
	else
	{
	//	fwrite(pBuffer, BufferLen, 1, fp1);
	//TRACE("Audio sample size: %d\n ", BufferLen);
		StreamBuf buf;
		buf.frame = pBuffer;
		buf.bufsize = BufferLen;
		frame_queue_put(m_pAQueue, &buf);
	}

	return 0;
}


