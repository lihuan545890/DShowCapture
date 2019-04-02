//
#ifndef __H264_H__
#define __H264_H__

#include "h264frame.h"

// Encode context
class CH264EncodeRtp
{
public:
	CH264EncodeRtp(){};
	~CH264EncodeRtp(){};

    virtual bool EncodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags);
	
    virtual void SetMaxRTPFrameSize (unsigned size) ;
public:
	H264Frame  m_encapsulation;

};


class CH264DecodeRtp
{
public:
	CH264DecodeRtp() { m_bHaveGetKeyFrame = false; } ;
	~CH264DecodeRtp(){};

	virtual bool DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags, int &nWidth , int &nHeight );

public:
	H264Frame  m_fullFrame;

	bool       m_bHaveGetKeyFrame;
	int			m_nWidth;
	int			m_nHeight;

};

#endif