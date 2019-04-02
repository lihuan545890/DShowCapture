//
#include <string.h>
#include "h264-rtp.h"
#include "Is_H264KeyFrame.h"
#include "stdafx.h"
#define MAX_NAL_SIZE 128

bool CH264EncodeRtp::EncodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
{
	// if there are NALU's encoded, return them
	if (!m_encapsulation.HasRTPFrames()) {
		//search the H264 NALUs from src
		h264_nal nalPos[MAX_NAL_SIZE];
		unsigned int inputLen = srcLen;
		int nRetNalSize = GetNalSize ((I_BYTE*) src, inputLen, nalPos, MAX_NAL_SIZE);
		m_encapsulation.BeginNewFrame(nRetNalSize);
//		m_encapsulation.SetTimestamp(srcRTP.GetTimestamp());
		for (int i = 0; i < nRetNalSize; i++)
		{
			m_encapsulation.AddNALU(nalPos[i].type, nalPos[i].len, (unsigned char *) nalPos[i].pdata);
		}
	}
	
	// create RTP frame from destination buffer
	RTPFrame dstRTP(dst, dstLen);
	dstLen = 0;
	m_encapsulation.GetRTPFrame(dstRTP, flags);
	dstLen = dstRTP.GetFrameLen();
//	flags |= PluginCodec_ReturnCoderLastFrame;

	return 1;

}

void CH264EncodeRtp::SetMaxRTPFrameSize (unsigned size) 
{
	m_encapsulation.SetMaxPayloadSize( size );
}



///
bool CH264DecodeRtp::DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags, int &nWidth , int &nHeight  )
{
	//WriteLog_C( I_TRUE , "Decode H264 Frames!");

	RTPFrame srcRTP((const unsigned char *)src, srcLen );

	dstLen = 0;


	if ( !m_fullFrame.SetFromRTPFrame(srcRTP, flags) )
        return false;

	// Wait for marker to indicate end of frame
	if ( !srcRTP.GetMarker() )
        return false;
	
	int bytesToDecode = m_fullFrame.GetFrameSize();
	if (bytesToDecode== 0 ) {
        m_fullFrame.BeginNewFrame();
        return false;
	}
	nWidth = 0;
	nHeight = 0;

	bool bKeyFrame = Is_H264KeyFrame( (I_BYTE*)m_fullFrame.GetFramePtr(), bytesToDecode , &nWidth , &nHeight);
	//TRACE("nWidth:%d, nHeight: %d, bKeyFrame:%d, m_bHaveGetKeyFrame:%d\n", nWidth, nHeight, bKeyFrame, m_bHaveGetKeyFrame);
#if 1
	if( !bKeyFrame && !m_bHaveGetKeyFrame)
	{
		flags |= PluginCodec_ReturnCoderRequestIFrame;	
		m_fullFrame.BeginNewFrame();
		m_nWidth = 0;
		m_nHeight = 0;
		return true;
	}
#endif
	if( bKeyFrame )
	{
		flags |= PluginCodec_ReturnCoderIFrame;
		m_bHaveGetKeyFrame  = true;
		m_nWidth = nWidth;
		m_nHeight = nHeight;
	}

	memcpy( dst, m_fullFrame.GetFramePtr(), bytesToDecode );
	dstLen = m_fullFrame.GetFrameSize();
	flags |= PluginCodec_ReturnCoderLastFrame;
	nWidth = m_nWidth;
	nHeight = m_nHeight;
	m_fullFrame.BeginNewFrame();

    return true;
}

