#include "AACLiveFramedSource.h"

#include <GroupsockHelper.hh> // for "gettimeofday()"


AACLiveFramedSource* AACLiveFramedSource::createNew(UsageEnvironment& env, int samplerate, int bitrate, FrameQueue *aQueue)
{

	AACLiveCaptureThread *thread = new AACLiveCaptureThread();
	if(thread == NULL)
	{
		return NULL;
	}

	if(!thread->Create(samplerate, bitrate, aQueue))
	{
		delete thread;
		return NULL;
	}
	
	return  new AACLiveFramedSource(env, samplerate, bitrate, aQueue, thread);
}


AACLiveFramedSource::AACLiveFramedSource(UsageEnvironment& env, int samplerate, int bitrate, FrameQueue *aQueue, AACLiveCaptureThread *thread)
			: FramedSource(env), mThread(thread)
{


}

AACLiveFramedSource::~AACLiveFramedSource()
{
    mThread->Destroy();
    envir().taskScheduler().unscheduleDelayedTask(mToken);	
}

void AACLiveFramedSource::getNextFrame(void * ptr)
{
    ((AACLiveFramedSource*)ptr)->getNextFrame1();  	
}

void AACLiveFramedSource::getNextFrame1()
{
    int frameSize, truncatedSize;
    mThread->Export(fTo, maxFrameSize(), &frameSize, &truncatedSize);
    fFrameSize = frameSize;	
    fNumTruncatedBytes = truncatedSize;	
    mThread->Capture();	

    afterGetting(this); 	
}

void AACLiveFramedSource::doGetNextFrame()
{
    mToken = envir().taskScheduler().scheduleDelayedTask(0, getNextFrame, this);
}

unsigned int AACLiveFramedSource::maxFrameSize()
{
	return 0;
}