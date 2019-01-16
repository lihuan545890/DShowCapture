#include "h264_codec.h"
#include "afx.h"

H264Encoder::H264Encoder()
{

}

H264Encoder::~H264Encoder()
{

}

int H264Encoder::InitEncode(int width, int height, int fps, int bitrate)
{
	av_register_all();
	pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);

	pCodecCtx = avcodec_alloc_context3(pCodec);
	pCodecCtx->bit_rate = bitrate;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = width;
	pCodecCtx->height = height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den =	fps;
	pCodecCtx->gop_size = fps;
//	pCodecCtx->time_base = (AVRational){1, fps};
//	pCodecCtx->max_b_frames = 1;

//	av_opt_set(pCodecCtx->priv_data, "profile", "main", 0);
//	av_opt_set(pCodecCtx->priv_data, "preset", "ultrafast", 0);

	//av_opt_set(pCodecCtx->priv_data, "profile", "base", 0);
	av_opt_set(pCodecCtx->priv_data, "preset", "ultrafast", 0);
	av_opt_set(pCodecCtx->priv_data, "tune", "zerolatency", 0);

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		TRACE("open codec failed\n");
		return -1;
	}

	pFrameYUV = av_frame_alloc(); // 初始化的时候AVFrame中的元素data,linesize均为空。未指向任何内存数据，必须指向一块内存
	uint8_t * out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, width, height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, width, height);

	return 0;
}

//FILE *fp=fopen("d:\\yuv\\test.h264", "wb");;
//FILE *fp1 = fopen("d:\\yuv\\test.yuv", "wb");
int H264Encoder::Encode(unsigned char  *src_buf, unsigned char  ** dst_buf)
{
	int enc_got_frame = 0;
	int enc_size = 0;

	if (src_buf == NULL)
	{
		return -1;
	}

	av_init_packet(&enc_pkt);
	enc_pkt.data = NULL;
	enc_pkt.size = 0;

	pFrameYUV->format = AV_PIX_FMT_YUV420P;
	pFrameYUV->width = pCodecCtx->width;
	pFrameYUV->height = pCodecCtx->height;

	int y_size = pCodecCtx->width * pCodecCtx->height;
	memcpy(pFrameYUV->data[0], src_buf, y_size);
	memcpy(pFrameYUV->data[1], src_buf + y_size, y_size / 4);
	memcpy(pFrameYUV->data[2], src_buf + y_size * 5 / 4, y_size / 4);	

	if (avcodec_encode_video2(pCodecCtx, &enc_pkt, pFrameYUV, &enc_got_frame)) {
		TRACE("encode video failed !!!\n");
		return -1;
	}

	if(enc_got_frame)
	{
		//memcpy(dst_buf, enc_pkt.data, enc_pkt.size);
		*dst_buf = enc_pkt.data;
		enc_size = enc_pkt.size;				
//		av_free_packet(&enc_pkt);
		return enc_size;
	}
	else
	{
		return -1;
	}


}

int H264Encoder::StopEncode()
{
	return 0;
}

H264Decoder::H264Decoder()
{

}

H264Decoder::~H264Decoder()
{

}

int H264Decoder::InitDecode()
{
	return 0;
}

int H264Decoder::Decode(unsigned char  *buf, int size)
{
	return 0;
}

int H264Decoder::StopDecode()
{
	return 0;
}
