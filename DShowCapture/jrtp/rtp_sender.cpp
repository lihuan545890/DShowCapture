#include "stdafx.h"
#include <iostream>
#include "rtp_sender.h"

using namespace jrtplib;
#define MAXLEN	(RTP_DEFAULTPACKETSIZE - 100)
#define MAX_RTP_PKT_LENGTH (RTP_DEFAULTPACKETSIZE - 100)
void checkerror(int rtperr)
{
	if(rtperr < 0)
	{
		//std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
		TRACE("ERROR: %s\n", RTPGetErrorString(rtperr).c_str());
	//	exit(-1);
	}
}

RTPSender::RTPSender()
{
	m_H264Rtp.SetMaxRTPFrameSize (1200);
}

RTPSender::~RTPSender()
{

}

void *SendDataThread(void *param)
{
	RTPSender *pSender = (RTPSender*)param;
	FILE *fp = fopen("d:\\yuv\\test.h264", "wb");
	unsigned char * outBuf = NULL;
	
	while(1)
	{

	   	StreamBuf frameBuf;
 		int ret = frame_queue_get(pSender->m_vQueue, &frameBuf, 1);
		if (frameBuf.bufsize > 0)
		{

			ret = pSender->m_pEncoder->Encode(frameBuf.frame, &outBuf);

			if(ret > 0)
			{
				pSender->SendH264Nalu(&pSender->m_RtpSess, outBuf, ret);
				
			}
		//	fwrite(outBuf, 1, ret, fp);
		}
		//TRACE(".................hello world bufsize:%d enc size: %d!\n", frameBuf.bufsize, ret);
		//Sleep(1 * 1000);
	}
}


int RTPSender::StartRTPSender(ENCODE_PARAMS params, FrameQueue *vQueue, FrameQueue *aQueue)
{
	int status;
	uint16_t srcport = 6666;
	uint16_t destport = 6664;
	uint8_t destip[] = { 10,18,0,63 };

	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;

	/* set h264 param */
	sessparams.SetUsePredefinedSSRC(true);  //设置使用预先定义的SSRC 
	sessparams.SetOwnTimestampUnit(1.0 / 90000); /* 设置采样间隔 */
	sessparams.SetAcceptOwnPackets(false);   //接收自己发送的数据包  	

	transparams.SetPortbase(srcport);
	status = m_RtpSess.Create(sessparams, &transparams);
	checkerror(status);

	RTPIPv4Address addr(destip, destport);
	status = m_RtpSess.AddDestination(addr);
	checkerror(status);

	m_RtpSess.SetDefaultTimestampIncrement(60);/* 设置时间戳增加间隔 */
	m_RtpSess.SetDefaultPayloadType(96);
	m_RtpSess.SetDefaultMark(true);

	m_vQueue = vQueue;
	m_aQueue = aQueue;


	m_pEncoder = new H264Encoder();
	m_pEncoder->InitEncode(params.stVidParams.nWidth, params.stVidParams.nHeight, params.stVidParams.nFrameRate, params.stVidParams.nBitRate);
	pthread_create(&m_SendThread, NULL, SendDataThread, this);

	return 0;
}

int RTPSender::StopRTPSender()
{
	return 0;
}

#if 0
void RTPSender::SendH264Nalu(RTPSession * sess, uint8_t * h264buf, int buflen)
{
     
    unsigned char *pbuf  = h264buf;
 
    char sendbuf[1400];
    memset(sendbuf,0,1400);
 
    int status;   

    if (buflen <= MAX_RTP_PKT_LENGTH)
    {   
        memcpy(sendbuf,pbuf,buflen);       
        status = sess->SendPacket((void *)sendbuf,buflen/*,payload,false,timestamp*/);
         
        checkerror(status);
 
    }   
	else if (buflen >= MAX_RTP_PKT_LENGTH)
	{
		//
		//得到该nalu需要用多少长度为MAX_RTP_PKT_LENGTH字节的RTP包来发送
		int k = 0, l = 0;
		k = buflen / MAX_RTP_PKT_LENGTH;
		l = buflen%MAX_RTP_PKT_LENGTH;
		int t = 0;//用于指示当前发送的是第几个分片RTP包       

		char nalHeader = pbuf[0]; // NALU 头
		while (t < k || (t == k&&l > 0))
		{
			if ((0 == t) || (t < k && 0 != t))//第一包到最后一包的前一包
			{
#if 0    //vlc播放需去除前导码0x000001 或者0x00000001
				sendbuf[0] = (nalHeader & 0x60) | 28;
				sendbuf[1] = (nalHeader & 0x1f);
				if (0 == t)
				{
					sendbuf[1] |= 0x80;
				}

				memcpy(sendbuf + 2, &pbuf[t*MAX_RTP_PKT_LENGTH], MAX_RTP_PKT_LENGTH + 2);
				status = sess->SendPacket((void *)sendbuf, MAX_RTP_PKT_LENGTH + 2);
#else		//ffmpeg解码
				memcpy(sendbuf, &pbuf[t*MAX_RTP_PKT_LENGTH], MAX_RTP_PKT_LENGTH);
				status = sess->SendPacket((void *)sendbuf, MAX_RTP_PKT_LENGTH);
				checkerror(status);
#endif
				t++;

			}
			//最后一包
			else if ((k == t&&l > 0) || (t == (k - 1) && l == 0) )
			{

				int iSendLen;
				if (l > 0)
				{
					iSendLen = buflen - t*MAX_RTP_PKT_LENGTH;
				}
				else
					iSendLen = MAX_RTP_PKT_LENGTH;
#if 0		//vlc
				sendbuf[0] = (nalHeader & 0x60) | 28;
				sendbuf[1] = (nalHeader & 0x1f);
				sendbuf[1] |= 0x40;

				memcpy(sendbuf + 2, &pbuf[t*MAX_RTP_PKT_LENGTH], iSendLen + 2);
				status = sess->SendPacket((void *)sendbuf, iSendLen + 2);
#else		//ffmpeg

				memcpy(sendbuf, &pbuf[t*MAX_RTP_PKT_LENGTH], iSendLen);
				status = sess->SendPacket((void *)sendbuf, iSendLen);
#endif
				checkerror(status);
				t++;
				//    Sleep(100);
			}
		}
	}
 
#ifndef RTP_SUPPORT_THREAD
        status = sess->Poll();
        checkerror(status);
#endif 
      //  RTPTime::Wait(RTPTime(0,500));   //第一个参数为秒，第二个参数为毫秒

 

     
}
#else

int64_t RTPSender::getCurrentTime()
{
    timeb t;
    ftime(&t);
    return t.time * 1000 + t.millitm;
}

void RTPSender::SendH264Nalu(RTPSession * sess, uint8_t * h264buf, int buflen)
{

	while(1)
	{

		char byDest[1500] = { 0 };
		unsigned int nDestLen = 0;
		unsigned int nFlags = 0;	
		unsigned int nPt = 96;
		bool bEncodeOK = false;
		//bool bMark = false;
		memset(byDest, 0, 1500);
		//nDestLen  = 1212;
		nDestLen  = 1212;
	 	unsigned int nInputLen = buflen;

		unsigned int nEndFrameMark = 0;
		
		bEncodeOK = m_H264Rtp.EncodeFrames(h264buf, nInputLen, (BYTE*)byDest, nDestLen, nFlags);


		if(bEncodeOK)
		{
			nEndFrameMark = nFlags & PluginCodec_ReturnCoderLastFrame;
			if (lastTime == 0)
				lastTime = getCurrentTime();
			int nTimeStampInc = 0;
			if (nEndFrameMark)
			{
				nTimeStampInc = getCurrentTime() - lastTime;

				lastTime += nTimeStampInc;
			}

			int nRes = sess->SendPacket((char*)&byDest[12], nDestLen-12, nPt, nEndFrameMark, nTimeStampInc);

			if( nEndFrameMark )  
			{

				break;
			}		
		}

	

	}

}

#endif
