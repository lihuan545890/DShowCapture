#ifndef RTSP_SERVER_INSTANCE_H
#define RTSP_SERVER_INSTANCE_H
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"
#include "framequeue.h"

class RTSPServerInstance
{
	public:
		RTSPServerInstance();
		~RTSPServerInstance();
		bool Start(ENCODE_PARAMS param, FrameQueue *vQueue, FrameQueue *aQueue);
		bool StartServer(int nWidth, int nHeight, int nFps, FrameQueue *vQueue, FrameQueue *aQueue);
		bool Stop();
		friend void *RTSPStartThread(void *param);

	private:
		UsageEnvironment* m_uEnv;
		TaskScheduler* m_scheduler;
		RTSPServer* m_rtspServer;
		pthread_t m_RtspThrID;		
		int m_nWidth;
		int m_nHeight;
		int m_nFps;
		FrameQueue *m_vQueue;
		FrameQueue *m_aQueue;
};


#endif