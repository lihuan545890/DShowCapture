#include "stdafx.h"
//#include "QDshowRecordToMp4.h"
#include "SampleGrabberCB.h"

#define uint8_t BYTE
int YUY2toI420(int inWidth, int inHeight, uint8_t *pSrc, uint8_t *pDest)
{
	int i, j;
	//���ȶ�I420���������岼��ָ��
	uint8_t *u = pDest + (inWidth * inHeight);
	uint8_t *v = u + (inWidth * inHeight) / 4;

	if (pSrc == NULL || pDest == NULL)
	{
		return -1;
	}


	for (i = 0; i < inHeight / 2; i++)
	{
		/*��ȡ�Ĳ�����:�����ѭ�����棬ȡ�������ڵ���*/
		uint8_t *src_l1 = pSrc + inWidth * 2 * 2 * i;//��Ϊ4:2:2��ԭ������ռ���ڴ棬�൱һ������ռ2���ֽڣ�2�����غϳ�4���ֽ�
		uint8_t *src_l2 = src_l1 + inWidth * 2;//YUY2��ż������һ��
		uint8_t *y_l1 = pDest + inWidth * 2 * i;//ż����
		uint8_t *y_l2 = y_l1 + inWidth;//ż���е���һ��
		for (j = 0; j < inWidth / 2; j++)//�ڲ�ѭ��
		{
			// two pels in one go//һ�κϳ���������
			//ż���У�ȡ��������;Y,U,V;ż���е���һ�У�ֻȡY
			*y_l1++ = src_l1[0];//Y
			*u++ = src_l1[1];//U
			*y_l1++ = src_l1[2];//Y
			*v++ = src_l1[3];//V
							 //����ֻ��ȡY
			*y_l2++ = src_l2[0];
			*y_l2++ = src_l2[2];
			//YUY2,4������Ϊһ��
			src_l1 += 4;
			src_l2 += 4;
		}
	}

	return 0;
}


int YUV422toYUV420(unsigned char *yuv420, unsigned char *yuv422, int Y_width, int Y_height)
{
	int len = Y_width * Y_height;
	unsigned char* pY420 = yuv420;
	unsigned char* pU420 = yuv420 + len;
	unsigned char* pV420 = pU420 + len / 4;


	if (yuv420 == NULL || yuv422 == NULL)
	{
		return -1;
	}

	unsigned char* pY422 = yuv422;
	for (int i = 0; i < len; i++)
	{
		*pY420 = *pY422;
		pY420++;
		pY422 += 2;
	}

	unsigned char* pU422 = yuv422 + 1;
	for (int i = 0; i < len / 4; i++)
	{
		*pU420 = (*pU422 + *(pU422 + 4)) / 2;
		pU420++;
		pU422 += 8;
	}

	unsigned char* pV422 = yuv422 + 3;
	for (int i = 0; i < len / 4; i++)
	{
		*pV420 = (*pV422 + *(pV422 + 4)) / 2;
		pV420++;
		pV422 += 8;
	}

	return 0;
}


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
 
FILE *yuvfp = fopen("c:\\yuv\\test.yuv", "wb");
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


