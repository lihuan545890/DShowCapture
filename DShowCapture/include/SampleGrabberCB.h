#pragma once
#include "qedit.h"
#include "framequeue.h"

#define COLOR_FormatYUY2 0
#define COLOR_FormatRGB24 1

typedef struct
{
	int nWidth;
	int nHeight;
	int nColorFormat;
	unsigned char *pYUVBuf;
}VID_PARAM;

class CSampleGrabberCB :
	public ISampleGrabberCB
{
public:
	CSampleGrabberCB(void);
	virtual ~CSampleGrabberCB(void);

public:
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppv);
	HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime, IMediaSample *pSample);
	HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen);

public:
	BOOL m_bBeginEncode;
	int  m_nMediaType;
	VID_PARAM m_tVidParam;

	FrameQueue *m_pVQueue;
	FrameQueue *m_pAQueue;
};

