#ifndef _MP4_RECORD_H_
#define _MP4_RECORD_H_

extern "C"{
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libavutil/opt.h"
	#include "libavutil/time.h"	
	#include "libswscale/swscale.h"
}
#include <pthread.h>
#include <framequeue.h>

class Mp4Record
{
	public:
		Mp4Record();
		~Mp4Record();
		int InitRecord(const char* filename, ENCODE_PARAMS params);
		int StartRecord(FrameQueue *video_queue, FrameQueue *audio_queue);
		int StopRecord();

		friend void * RecordVideoThread(void *param);
		friend void * RecordAudioThread(void *param);

	private:
		AVFormatContext *pFormatCtx;
		AVCodec *pVideoCodec, *pAudioCodec;
		AVCodecContext *pVideoCodecCtx, *pAudioCodecCtx;
		AVStream *pVideoStream, *pAudioStream;
		AVPacket stVideoPkt, stAudiopkt;
		AVFrame *pFrameYUV, *pFramePCM;
		int64_t startTime;
		int nb_samples;
	
		pthread_t m_RecVidThrID;
		pthread_t m_RecAudThrID;	
		pthread_mutex_t m_mutex;
		bool m_bStart;

		FrameQueue *m_stVQueue;
		FrameQueue *m_stAQueue;		
		
};
#endif