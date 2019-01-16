/*
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#include <iostream>
#include "H264LiveServerMediaSubsession.h"
#include "H264LiveFramedSource.h"

using namespace std;

H264LiveServerMediaSubsession* H264LiveServerMediaSubsession::createNew(UsageEnvironment& env, 
   int width, int height, int fps, FrameQueue *vQueue)
{
    return new H264LiveServerMediaSubsession(env, width, height, fps, vQueue);
}

H264LiveServerMediaSubsession::H264LiveServerMediaSubsession(UsageEnvironment& env, 
    int width, int height, int fps, FrameQueue *vQueue)
    : OnDemandServerMediaSubsession(env, True),
      mAuxSDPLine(NULL), mDoneFlag(0), mDummyRTPSink(NULL),
      mWidth(width), mHeight(height), mFps(fps), m_pVQueue(vQueue)
{

}

H264LiveServerMediaSubsession::~H264LiveServerMediaSubsession()
{
    if (mAuxSDPLine)
    {
        delete[] mAuxSDPLine;
    }

}

static void afterPlayingDummy(void* clientData) 
{
    H264LiveServerMediaSubsession* subsess = (H264LiveServerMediaSubsession*)clientData;
    subsess->afterPlayingDummy1();
}

void H264LiveServerMediaSubsession::afterPlayingDummy1() 
{
    // Unschedule any pending 'checking' task:
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    // Signal the event loop that we're done:
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) 
{
    H264LiveServerMediaSubsession* subsess = (H264LiveServerMediaSubsession*)clientData;
    subsess->checkForAuxSDPLine1();
}

void H264LiveServerMediaSubsession::checkForAuxSDPLine1() 
{
    char const* dasl;

    if (mAuxSDPLine != NULL) 
    {
        // Signal the event loop that we're done:
        setDoneFlag();
    } 
    else if (mDummyRTPSink != NULL && (dasl = mDummyRTPSink->auxSDPLine()) != NULL) 
    {

        mAuxSDPLine = strDup(dasl);
        mDummyRTPSink->stopPlaying();
        mDummyRTPSink = NULL;

        // Signal the event loop that we're done:
        setDoneFlag();
    } 
    else if (!mDoneFlag) 
    {
        // try again after a brief delay:
        double delay = 10;  // ms  
        int uSecsToDelay = delay * 1000;  // us  
        nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
            (TaskFunc*)checkForAuxSDPLine, this);
    }
}

 
char const* H264LiveServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) 
{
   if (mAuxSDPLine != NULL) return mAuxSDPLine; // it's already been set up (for a previous client)

    if (mDummyRTPSink == NULL) 
    { 
        mDummyRTPSink = rtpSink;

        mDummyRTPSink->startPlaying(*inputSource, NULL, NULL);

        checkForAuxSDPLine(this);
    }

    envir().taskScheduler().doEventLoop(&mDoneFlag);

    return mAuxSDPLine;
}

FramedSource* H264LiveServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
    // Create the video source:

    H264LiveFramedSource* cameraSource = H264LiveFramedSource::createNew(envir(), mWidth, mHeight, mFps, m_pVQueue);
    if (cameraSource == NULL) return NULL;

    return H264VideoStreamFramer::createNew(envir(), cameraSource);
}

RTPSink* H264LiveServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
