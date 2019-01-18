#include  <afx.h>  
#include "RtspServerInstance.h"
#include "H264LiveServerMediaSubsession.h"
#include "AACLiveServerMediaSubsession.h"
//#define ACCESS_CONTROL

RTSPServerInstance::RTSPServerInstance()
	:m_rtspServer(NULL),
	m_scheduler(NULL),
	m_uEnv(NULL)
	
{

}

void *RTSPStartThread(void *param)
{
	RTSPServerInstance * pServer = (RTSPServerInstance*)param;
	pServer->StartServer(pServer->m_nWidth, pServer->m_nHeight, pServer->m_nFps, pServer->m_vQueue, pServer->m_aQueue);

	return 0;
}

RTSPServerInstance::~ RTSPServerInstance()
{

}

bool RTSPServerInstance::StartServer(int nWidth, int nHeight, int nFps, FrameQueue * vQueue, FrameQueue * aQueue)
{
	m_scheduler = BasicTaskScheduler::createNew();
	if(!m_scheduler)
	{
		return false;
	}

	m_uEnv = BasicUsageEnvironment::createNew(*m_scheduler);
	if(!m_uEnv)
	{
		return false;
	}

  UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
  // To implement client access control to the RTSP server, do the following:
  authDB = new UserAuthenticationDatabase;
  authDB->addUserRecord("admin", "123456"); // replace these with real strings
  // Repeat the above with each <username>, <password> that you wish to allow
  // access to the server.
#endif
	

	m_rtspServer = RTSPServer::createNew(*m_uEnv, 554, authDB);
    if (m_rtspServer == NULL) 
    {
        m_rtspServer = RTSPServer::createNew(*m_uEnv, 8554, authDB);
    }	

	ServerMediaSession* sms = ServerMediaSession::createNew(*m_uEnv, "RTSPServer",
			0, "Camera server, streamed by the LIVE555 Media Server");	

//	sms->addSubsession(H264LiveServerMediaSubsession::createNew(*m_uEnv,
//		 nWidth, nHeight, nFps, vQueue));

	sms->addSubsession(AACLiveServerMediaSubsession::createNew(*m_uEnv, 44100, 64000, aQueue));
	m_rtspServer->addServerMediaSession(sms);
	char* url = m_rtspServer->rtspURL(sms);
	TRACE("Play this Stream %d x %d fps:%d using the URL: \"%s\"",nWidth,  nHeight,  nFps, url);
	delete[] url;	

	m_uEnv->taskScheduler().doEventLoop();
	TRACE("RTSPServerInstance start end\n");
	return true;
}

bool RTSPServerInstance::Start(ENCODE_PARAMS param, FrameQueue *vQueue, FrameQueue *aQueue)
{
	TRACE("RTSPServerInstance start\n");
	m_nWidth = param.stVidParams.nWidth;
	m_nHeight = param.stVidParams.nHeight;
	m_nFps = param.stVidParams.nFrameRate;
	m_vQueue = vQueue;
	m_aQueue = aQueue;
	pthread_create(&m_RtspThrID, NULL, RTSPStartThread, this);

	return true;
}

bool RTSPServerInstance::Stop()
{
	return true;
}