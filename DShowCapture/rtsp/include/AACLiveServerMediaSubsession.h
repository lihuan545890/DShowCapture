#include <liveMedia.hh>
#include "framequeue.h"


class AACLiveServerMediaSubsession : public OnDemandServerMediaSubsession
{
public:
	static AACLiveServerMediaSubsession* createNew(UsageEnvironment& env, int samplerate, int bitrate, FrameQueue *aQueue);

protected:
	AACLiveServerMediaSubsession(UsageEnvironment& env, int samplerate, int bitrate, FrameQueue *aQueue);
	virtual ~AACLiveServerMediaSubsession();

protected:
	virtual FramedSource * createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);

	int m_nSampleRate;
	int m_nBitRate;
	FrameQueue *m_pAQueue;
};