
#include "stdafx.h"
#include "rtp_receiver.h"


#define MAX_FRAME_LEN  2 * 1024 * 1024
extern void checkerror(int rtperr);
using namespace jrtplib;

RTPReceiver::RTPReceiver()
{
	m_FrameData = (uint8_t*)malloc(MAX_FRAME_LEN);
	
}

RTPReceiver::~RTPReceiver()
{

}

void *RecvDataThread(void *param)
{
	RTPReceiver *pReceiver = (RTPReceiver*)param;
	FILE *fp = fopen("d:\\yuv\\test_recv.h264", "wb");	
	uint8_t destbuf[1024*100] = {0};
	uint8_t *pSrcData;	
	int nLen;
	int status;
	int pos = 0;
	
	while(1)
	{
		RTPPacket *pRtpPkt;	
		status = pReceiver->m_RtpSess.Poll();	

		checkerror(status);

		if (pReceiver->m_RtpSess.GotoFirstSourceWithData())
		{
			do
			{
				if ((pRtpPkt = pReceiver->m_RtpSess.GetNextPacket()) != NULL)
				{
					pSrcData = pRtpPkt->GetPacketData();
					nLen	 = pRtpPkt->GetPacketLength();
					unsigned int nFrameDataLen = MAX_FRAME_LEN;
					int sizeX = 0, sizeY = 0;
					unsigned int nFlags = 0;
					unsigned int nRawDataLen = nLen;
					if (pSrcData == NULL)
					{
						continue;
					}
					int nRet = pReceiver->m_H264Rtp.DecodeFrames(pSrcData, nRawDataLen, pReceiver->m_FrameData, nFrameDataLen, nFlags, sizeX, sizeY);
		
					pReceiver->m_pDecoder->Decode(pReceiver->m_FrameData, nFrameDataLen, pReceiver->m_pYUVBuf);
					pReceiver->m_pDDraw->DrawDirectDraw(pReceiver->m_pYUVBuf);


				
				}
				pReceiver->m_RtpSess.DeletePacket(pRtpPkt);

			}while (pReceiver->m_RtpSess.GotoNextSourceWithData());
		}		
		
		//fwrite();
	} 
}

void RTPReceiver::RecvH264Nalu(RTPSession * sess, uint8_t *dest_buf, int *len)
{
	int status;
	uint8_t *pSrcData;
//	uint8_t destbuf[1024*100] = {0};
	int nLen;
	int pos = 0;

	if(sess == NULL)
	{
		return ;
	}
	
	RTPPacket *pRtpPkt;	
	status = sess->Poll();	

	checkerror(status);

	if (sess->GotoFirstSourceWithData())
	{
		do
		{
			if ((pRtpPkt = sess->GetNextPacket()) != NULL)
			{
				pSrcData = pRtpPkt->GetPayloadData();
				nLen	 = pRtpPkt->GetPayloadLength();

				//m_h264Rtp.DecodeFrames()
				
				
				sess->DeletePacket(pRtpPkt);
				
			}

		}while (sess->GotoNextSourceWithData());
	}
	
}

int RTPReceiver::StartRTPReceiver(CDirectDraw *pDDraw, ENCODE_PARAMS params, FrameQueue *vQueue, FrameQueue *aQueue)
{
	uint16_t portbase = 6664;	
	int status;
	m_pDDraw = pDDraw;
	
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;	
	sessparams.SetOwnTimestampUnit(1.0/10.0);	

	sessparams.SetAcceptOwnPackets(true);	
	transparams.SetPortbase(portbase);	
	status = m_RtpSess.Create(sessparams,&transparams);		
	checkerror(status);
	m_RtpSess.BeginDataAccess();		

	m_vQueue = vQueue;
	m_aQueue = aQueue;

	m_pDecoder = new H264Decoder();
	m_pDecoder->InitDecode(params.stVidParams.nWidth, params.stVidParams.nHeight, params.stVidParams.nFrameRate);
	m_pYUVBuf = (unsigned char *)malloc(params.stVidParams.nWidth * params.stVidParams.nHeight * 3 / 2);
	pthread_create(&m_RecvThread, NULL, RecvDataThread, this);
	return 0;
}

int RTPReceiver::StopRTPReceiver()
{
	free(m_pYUVBuf);
	return 0;
}
	