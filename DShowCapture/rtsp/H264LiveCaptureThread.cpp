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
#include "H264LiveCaptureThread.h"
#include "framequeue.h"
#include "stdafx.h"

using namespace std;

//FILE *fp =fopen("d:\\yuv\\test.h264", "wb");
//FILE *fp1 = fopen("d:\\yuv\\test.yuv", "wb");

//FILE *fp =fopen("/sdcard/test.yuv", "wb");
H264LiveCaptureThread::H264LiveCaptureThread()
    : mRunning(false), mExitFlag(false), mFrameBufLen(0), mTruncatedLen(0)
{
    memset(mFrameBuf, 0, sizeof(mFrameBuf));
	mLockThread = PTHREAD_MUTEX_INITIALIZER;
	mLockBuf = PTHREAD_MUTEX_INITIALIZER;
	mCondThread = PTHREAD_COND_INITIALIZER;

	m_pEncoder = new H264Encoder();
	m_pEncBuf = new unsigned char[512 * 1024];
}
 
H264LiveCaptureThread::~H264LiveCaptureThread()
{
	delete m_pEncoder;
	delete m_pEncBuf;
}

bool H264LiveCaptureThread::Create(int width, int height, int fps, FrameQueue *vQueue)
{	
    if (mRunning)
    {
        return false;
    }

	mWidth     = width;
	mHeight    = height;
	mFrameRate = fps;
	m_pVideoQueue = vQueue;
	
	m_pEncoder->InitEncode(mWidth, mHeight, mFrameRate, 1024 * 1000);

    if (0 != pthread_create(&mThread, NULL, H264LiveCaptureThread::H264LiveCaptureProc, this))
    {
        return false;
    }

    mRunning = true;

    Capture();

    return true;
}

void H264LiveCaptureThread::Destroy()
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

void H264LiveCaptureThread::Capture()
{
    if (!mRunning)
    {
        return;
    }

    pthread_cond_signal(&mCondThread);
}

void H264LiveCaptureThread::Export(void* buf, int len, int* frameLen, int* truncatedLen)
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

bool H264LiveCaptureThread::GetExitFlag()
{
    return mExitFlag;
}


void* H264LiveCaptureThread::H264LiveCaptureProc(void* ptr)
{
    H264LiveCaptureThread* thread = (H264LiveCaptureThread*)ptr;
	TRACE("H264LiveCaptureProc.....................\n");
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

#if 1
void H264LiveCaptureThread::CaptureProc()
{
    int ret;
    unsigned char * outBuf = NULL;  
    int outLen = 0;
	int nRes = -1;

    pthread_mutex_lock(&mLockThread);
    pthread_cond_wait(&mCondThread, &mLockThread);
    pthread_mutex_unlock(&mLockThread);

   	StreamBuf frameBuf;
 	ret = frame_queue_get(m_pVideoQueue, &frameBuf, 1);
   
    if (frameBuf.bufsize> 0)
    {	
	   	if(frame_queue_count(m_pVideoQueue) == 0)
	   	
		{

			if(frameBuf.frame != NULL)
			{
				nRes = m_pEncoder->Encode(frameBuf.frame, &outBuf);
			}
			else
			{
				return ;
			}


	        unsigned int frameSize = nRes;
	        int truncatedSize = 0;
	        if (frameSize > sizeof(mFrameBuf))
	        {
	            truncatedSize = frameSize - sizeof(mFrameBuf);
	            frameSize = sizeof(mFrameBuf); 
	        }  
			if(outBuf != NULL)
			{	
				memcpy(mFrameBuf, outBuf, frameSize);
			}

	        pthread_mutex_lock(&mLockBuf);
			
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
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
#else
void H264LiveCaptureThread::CaptureProc()
{
    int ret;
    unsigned char * outBuf = NULL;  
    int outLen = 0;
	int nRes = -1;

   	StreamBuf frameBuf;
 	ret = frame_queue_get(m_pVideoQueue, &frameBuf, 1);
    if (frameBuf.bufsize> 0)	
    {
	   	if(frame_queue_count(m_pVideoQueue) == 0)   	
		{

			if(frameBuf.frame != NULL)
			{	
				nRes = m_pEncoder->Encode(frameBuf.frame, &outBuf);
			}

	   	}
    }	
}
#endif


