#pragma once

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif
#include "framequeue.h"
#include "AACLiveCaptureThread.h"

class AACLiveFramedSource : public FramedSource
{
public:
	static AACLiveFramedSource * createNew(UsageEnvironment& env, int samplerate, int bitrate, FrameQueue *aQueue);
    static void getNextFrame(void* ptr);
    void getNextFrame1();

protected:
	AACLiveFramedSource(UsageEnvironment& env, int samplerate, int bitrate, FrameQueue *aQueue, AACLiveCaptureThread *thread);
	virtual ~AACLiveFramedSource();

	virtual void doGetNextFrame();
    virtual unsigned int maxFrameSize(); 

private:

    void* mToken;
	AACLiveCaptureThread *mThread;

};
