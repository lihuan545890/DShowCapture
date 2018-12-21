#ifndef _H264_CODEC_H_
#define _H264_CODEC_H_
#pragma warning( disable : 4996)

extern "C"{
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libavutil/opt.h"
	#include "libswscale/swscale.h"
}


class H264Encoder
{
	public:
		H264Encoder();
		~H264Encoder();
		int InitEncode(int width, int height, int fps, int bitrate);
		int Encode(unsigned char  *buf, int size);
		int StopEncode();
		
	private:
		AVCodec* pCodec;
		AVCodecContext* pCodecCtx;
		AVPacket enc_pkt;
		AVFrame *pFrameYUV;	
	
};

class H264Decoder
{
	public:
		H264Decoder();
		~H264Decoder();
		int InitDecode();
		int Decode(unsigned char  *buf, int size);
		int StopDecode();
		
	private:
		AVCodec* pCodec;
		AVCodecContext* pCodecCtx;
		AVPacket enc_pkt;
		AVFrame *pFrameYUV;	
	
};
#endif