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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "AACLiveCaptureThread.h"
#include "afx.h"

using namespace std;


AACLiveCaptureThread::AACLiveCaptureThread()
    : mRunning(false), mExitFlag(false), mFrameBufLen(0), mTruncatedLen(0)
{
    memset(mFrameBuf, 0, sizeof(mFrameBuf));
	mLockThread = PTHREAD_MUTEX_INITIALIZER;
	mLockBuf = PTHREAD_MUTEX_INITIALIZER;	
	mCondThread = PTHREAD_COND_INITIALIZER;	
	
	m_pEncoder = new AACEncoder();
	m_pEncBuf = new unsigned char[512 * 1024];	

//	fp1 = fopen("/sdcard/test.AAC", "wb");
}
 
AACLiveCaptureThread::~AACLiveCaptureThread()
{
	delete m_pEncoder;
	delete m_pEncBuf;	
}

bool AACLiveCaptureThread::Create(int samplerate, int bitrate, FrameQueue *aQueue)
{
    if (mRunning)
    {
        return false;
    }
		
	m_pAudioQueue = aQueue;
	m_pEncoder->InitEncode(samplerate, bitrate);

	
    if (0 != pthread_create(&mThread, NULL, AACLiveCaptureThread::AACLiveCaptureProc, this))
    {
        return false;
    }

    mRunning = true;

    Capture();

    return true;
}

void AACLiveCaptureThread::Destroy()
{
    if (mRunning)
    {
        mExitFlag = true;
        pthread_cond_signal(&mCondThread);

        pthread_join(mThread, NULL);

        pthread_mutex_destroy(&mLockBuf);
        pthread_mutex_destroy(&mLockThread);
        pthread_cond_destroy(&mCondThread);
        
        mRunning = false;
    }
}

void AACLiveCaptureThread::Capture()
{
    if (!mRunning)
    {
        return;
    }

    pthread_cond_signal(&mCondThread);
}

void AACLiveCaptureThread::Export(void* buf, int len, int* frameLen, int* truncatedLen)
{
    int exportLen = mFrameBufLen;

    pthread_mutex_lock(&mLockBuf);
    if (mFrameBufLen > len) 
    {   
        *truncatedLen = mTruncatedLen + mFrameBufLen - len;
        exportLen = len;
    }
    else
    {
        *truncatedLen = mTruncatedLen;
    }
    memcpy(buf, mFrameBuf, exportLen);
//	fwrite(buf, 1, exportLen, fp);
	//LOGI("exportLen: %d", exportLen);
    *frameLen = exportLen;

    pthread_mutex_unlock(&mLockBuf);
}

bool AACLiveCaptureThread::GetExitFlag()
{
    return mExitFlag;
}


void* AACLiveCaptureThread::AACLiveCaptureProc(void* ptr)
{
    AACLiveCaptureThread* thread = (AACLiveCaptureThread*)ptr;

    while (1)
    {
        if (thread->GetExitFlag())
        {
            break;
        }

        thread->CaptureProc();
    }

    pthread_exit(NULL);


	return 0;
}

FILE *aacfile = fopen("d:\\yuv\\test.aac", "wb");
void AACLiveCaptureThread::CaptureProc()
{
    int ret;
    unsigned char * outBuf = NULL;  
    int outLen = 0;

	
    pthread_mutex_lock(&mLockThread);
    pthread_cond_wait(&mCondThread, &mLockThread);
    pthread_mutex_unlock(&mLockThread);

	
	if(m_pAudioQueue == NULL)
	{
		return ;
	}
	StreamBuf frameBuf;
	ret = frame_queue_get(m_pAudioQueue, &frameBuf, 1); 

	if(frameBuf.bufsize> 0)
		ret = AAC_LIVE_CAPTURE_SUCCESS;
   
    if (AAC_LIVE_CAPTURE_SUCCESS == ret)
    {
	   	if(frame_queue_count(m_pAudioQueue) == 0)
	   	
		{
		//	fwrite(frameBuf.frame, 1, frameBuf.bufsize, aacfile);
			ret = m_pEncoder->Encode(frameBuf.frame, frameBuf.bufsize, m_pEncBuf);
	//		fwrite(m_pEncBuf, 1, ret, aacfile);
	//		TRACE("encode aac size: %d\n", ret);
	        unsigned int frameSize = ret;
	        int truncatedSize = 0;
	        if (frameSize > sizeof(mFrameBuf))
	        {
	            truncatedSize = frameSize - sizeof(mFrameBuf);
	            frameSize = sizeof(mFrameBuf); 
	        }  

	        pthread_mutex_lock(&mLockBuf);
			
			memcpy(mFrameBuf, m_pEncBuf, frameSize);

	        mFrameBufLen = frameSize;
	        mTruncatedLen = truncatedSize;
	        pthread_mutex_unlock(&mLockBuf);			
		}
		else
		{
	        pthread_mutex_lock(&mLockBuf);
	        mFrameBufLen = 0;
	        mTruncatedLen = 0;
	        pthread_mutex_unlock(&mLockBuf);
		}

    }
    else
    {
        pthread_mutex_lock(&mLockBuf);
        mFrameBufLen = 0;
        mTruncatedLen = 0;
        pthread_mutex_unlock(&mLockBuf);
    }


}


