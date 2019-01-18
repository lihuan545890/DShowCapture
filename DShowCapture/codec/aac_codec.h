#ifndef _AAC_CODEC_H_
#define _AAC_CODEC_H_

extern "C"{
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libavutil/opt.h"
	#include "libswscale/swscale.h"
}


class AACEncoder
{
	public:
		AACEncoder();
		~AACEncoder();
		int InitEncode(int sample_rate, int bit_rate);
		int Encode(unsigned char  *src_buf, int src_len, unsigned char  * dst_buf);
		int StopEncode();
		
	private:
		AVCodec* pCodec;
		AVCodecContext* pCodecCtx;
		AVPacket enc_pkt;
		AVFrame *pFramePCM;	
		uint8_t * out_buffer;
		int size;
		int index;
		int readbytes;
};

class AACDecoder
{
	public:
		AACDecoder();
		~AACDecoder();
		int InitDecode();
		int Decode(unsigned char  *buf, int size);
		int StopDecode();
		
	private:
		AVCodec* pCodec;
		AVCodecContext* pCodecCtx;
		AVPacket enc_pkt;
		AVFrame *pFramePCM;	
	
};
#endif