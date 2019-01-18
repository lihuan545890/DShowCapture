#include "SimpleRTPSink.hh"
#include "AACLiveServerMediaSubsession.h"
#include "AACLiveFramedSource.h"

AACLiveServerMediaSubsession* AACLiveServerMediaSubsession::createNew(UsageEnvironment& env, int samplerate, int bitrate, FrameQueue *aQueue)
{
	return new AACLiveServerMediaSubsession(env, samplerate, bitrate, aQueue);
}

AACLiveServerMediaSubsession::AACLiveServerMediaSubsession(UsageEnvironment& env, int samplerate, int bitrate, FrameQueue *aQueue)
	: OnDemandServerMediaSubsession(env, true), 
	m_nSampleRate(samplerate), 
	m_nBitRate(bitrate), 
	m_pAQueue(aQueue)
{

}

AACLiveServerMediaSubsession::~AACLiveServerMediaSubsession()
{

}


FramedSource* AACLiveServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
	AACLiveFramedSource *audioSource = AACLiveFramedSource::createNew(envir(), m_nSampleRate, m_nBitRate, m_pAQueue);
	if (audioSource == NULL)
		return NULL;
    estBitrate = 96; // kbps, estimate
	return audioSource;
}

RTPSink* AACLiveServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{

    return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,  
                      rtpPayloadTypeIfDynamic,  
                      44100,  
                      "audio", "AAC-hbr", "",  
                      1);  

}