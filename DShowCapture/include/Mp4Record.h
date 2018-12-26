#ifndef _MP4_RECORD_H_
#define _MP4_RECORD_H_

extern "C"{
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libavutil/opt.h"
	#include "libswscale/swscale.h"
}

class Mp4Record
{
	public:
		Mp4Record();
		~Mp4Record();
		int InitRecord(const char* filename);
		int StartRecord();
		int StopRecord();

	private:
		AVFormatContext *pFormatCtx;
		AVCodec *pVideoCodec, *pAudioCodec;
		AVCodecContext *pVideoCodecCtx, *pAudioCodecCtx;
		AVStream *pVideoStream, *pAudioStream;
		AVPacket stVideoPkt, stAudiopkt;
};
#endif