#ifndef __RTP_SENDER_H__
#define __RTP_SENDER_H__

#include <time.h>
#include <sys/timeb.h>

#include <pthread.h>
#include <rtpsession.h>
#include <rtpudpv4transmitter.h>
#include <rtpipv4address.h>
#include <rtpsessionparams.h>
#include <rtperrors.h>
#include <rtplibraryversion.h>

#include "framequeue.h"
#include "h264_codec.h"
#include "h264-rtp.h"

using namespace jrtplib;

class RTPSender
{
	public:
		RTPSender();
		~RTPSender();

		int StartRTPSender(ENCODE_PARAMS params, FrameQueue *vQueue, FrameQueue *aQueue);
		int StopRTPSender();

	private:
		RTPSession m_RtpSess;
		FrameQueue *m_vQueue;
		FrameQueue *m_aQueue;
		pthread_t m_SendThread;
		CH264EncodeRtp m_H264Rtp;
		uint64_t lastTime;

		H264Encoder *m_pEncoder;
		void SendH264Nalu(RTPSession* sess,uint8_t *h264buf,int buflen);
		friend void *SendDataThread(void *param);
		int64_t  getCurrentTime();
		
};

#endif
