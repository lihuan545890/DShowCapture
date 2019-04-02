#ifndef __RTP_RECEIVER_H__
#define __RTP_RECEIVER_H__


#include <pthread.h>
#include <rtpsession.h>
#include <rtpudpv4transmitter.h>
#include <rtpipv4address.h>
#include <rtpsessionparams.h>
#include <rtperrors.h>
#include <rtppacket.h>
#include <rtplibraryversion.h>

#include "framequeue.h"
#include "h264_codec.h"
#include "h264-rtp.h"
#include "DirectDraw.h"

using namespace jrtplib;

class RTPReceiver
{
	public:
		RTPReceiver();
		~RTPReceiver();

		int StartRTPReceiver(CDirectDraw *pDDraw, ENCODE_PARAMS params, FrameQueue *vQueue, FrameQueue *aQueue);
		int StopRTPReceiver();

	private:
		RTPSession m_RtpSess;		
		pthread_t  m_RecvThread;
		FrameQueue *m_vQueue;
		FrameQueue *m_aQueue;
		
		CH264DecodeRtp m_H264Rtp;
		H264Decoder *m_pDecoder;
		friend void *RecvDataThread(void *param);
		void RecvH264Nalu(RTPSession * sess, uint8_t *dest_buf, int *len);
		unsigned char* m_FrameData;
		unsigned char* m_pYUVBuf;	
		CDirectDraw *m_pDDraw;
};

#endif
