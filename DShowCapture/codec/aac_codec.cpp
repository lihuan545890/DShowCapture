#include "aac_codec.h"
#include "afx.h"

AACEncoder::AACEncoder()
{
	readbytes = 0;
}

AACEncoder::~AACEncoder()
{

}

int AACEncoder::InitEncode(int sample_rate, int bit_rate)
{
//	av_register_all();
	avcodec_register_all();
	pCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	/*
	pCodecCtx = avcodec_alloc_context3(pCodec);
	pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
	pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
	pCodecCtx->sample_rate = sample_rate ;//44100 8000
	pCodecCtx->sample_fmt = pCodec->sample_fmts ? pCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;;
	pCodecCtx->bit_rate = bit_rate;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = pCodecCtx->sample_rate;
	pCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;	
	*/

	pCodecCtx = avcodec_alloc_context3(pCodec);
	pCodecCtx->codec_id = AV_CODEC_ID_AAC;
	pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
	pCodecCtx->sample_fmt = pCodec->sample_fmts ? pCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;//AV_SAMPLE_FMT_S16;
	pCodecCtx->sample_rate = 44100;
	pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
	pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
	pCodecCtx->bit_rate = 64000;


	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		TRACE("open codec failed\n");
		return -1;
	}

	pFramePCM = av_frame_alloc(); // 初始化的时候AVFrame中的元素data,linesize均为空。未指向任何内存数据，必须指向一块内存
    pFramePCM->nb_samples = pCodecCtx->frame_size;
	pFramePCM->format = pCodecCtx->sample_fmt;
//	pFramePCM->sample_rate = pCodecCtx->sample_rate;

	size = av_samples_get_buffer_size(NULL,pCodecCtx->channels,
				pCodecCtx->frame_size,pCodecCtx->sample_fmt,1);


	out_buffer = (uint8_t *)av_malloc(size);
	avcodec_fill_audio_frame(pFramePCM,pCodecCtx->channels,pCodecCtx->sample_fmt,(const uint8_t *)out_buffer,size,1);

	return 0;
}

FILE *fp=fopen("d:\\yuv\\test.AAC", "wb");;
//FILE *fp1 = fopen("d:\\yuv\\test.yuv", "wb");
int AACEncoder::Encode(unsigned char  *src_buf,int src_len, unsigned char  * dst_buf)
{
	int enc_got_frame = 0;
	int enc_size = 0;


	int frameBufLen = 0;

	if (src_buf == NULL)
	{
		return -1;
	}

	pFramePCM->pts = index++;

	while(readbytes < src_len)
	{
		av_init_packet(&enc_pkt);	
		enc_pkt.data = NULL;
		enc_pkt.size = 0;	
		int left_size = src_len - readbytes;
		if(left_size < size)
		{
			memcpy(out_buffer, src_buf + readbytes, left_size);	
	 		readbytes += left_size;		
		}
		else
		{
			memcpy(out_buffer, src_buf + readbytes, size);		
	 		readbytes += size;		
		}


		if (avcodec_encode_audio2(pCodecCtx, &enc_pkt, pFramePCM, &enc_got_frame) < 0) {
			TRACE("couldn't encode frame\n");
			return -1;	
		}		
		TRACE("encode  aac  frame  size:%d, enc_got_frame:%d sample size:%d\n ", enc_pkt.size, enc_got_frame, size);
		if (enc_got_frame) {
			memcpy(dst_buf + enc_size, enc_pkt.data, enc_pkt.size);
			enc_size += enc_pkt.size;
			av_free_packet(&enc_pkt);

		}
		
	}
	TRACE("encode one audio frame over readbytes:%d, enc_size:%d\n", readbytes, enc_size);
	readbytes = 0;

	return enc_size;


}

int AACEncoder::StopEncode()
{
	return 0;
}

AACDecoder::AACDecoder()
{

}

AACDecoder::~AACDecoder()
{

}

int AACDecoder::InitDecode()
{
	return 0;
}

int AACDecoder::Decode(unsigned char  *buf, int size)
{
	return 0;
}

int AACDecoder::StopDecode()
{
	return 0;
}
