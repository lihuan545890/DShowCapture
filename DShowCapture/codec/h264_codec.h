#ifndef _H264_CODEC_H_
#define _H264_CODEC_H_

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
		int Encode(unsigned char  *src_buf, unsigned char  ** dst_buf);
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
		int InitDecode(int width, int height, int framerate);
		int Decode(unsigned char *src_buf, int size, unsigned char *dst_buf);
		int StopDecode();
		
	private:
		AVCodec* pCodec;
		AVCodecContext* pCodecCtx;
		AVPacket enc_pkt;
		AVFrame *pFrameYUV;	

};
#endif