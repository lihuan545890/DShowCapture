#include "h264_codec.h"

H264Encoder::H264Encoder()
{

}

H264Encoder::~H264Encoder()
{

}

int H264Encoder::InitEncode(int width, int height, int fps, int bitrate)
{
	av_register_all();
	//av_register_input_format();
	pCodec = avcodec_find_encoder_by_name("libx264");

	pFrameYUV = av_frame_alloc(); // 初始化的时候AVFrame中的元素data,linesize均为空。未指向任何内存数据，必须指向一块内存
	uint8_t * out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, width, height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, width, height);

	pCodecCtx = avcodec_alloc_context3(pCodec);
	pCodecCtx->bit_rate = 400000 * 5;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = width;
	pCodecCtx->height = height;
//	pCodecCtx->time_base = (AVRational){1, fps};
	pCodecCtx->max_b_frames = 1;

	av_opt_set(pCodecCtx->priv_data, "profile", "baseline", 0);
	av_opt_set(pCodecCtx->priv_data, "preset", "ultrafast", 0);

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		return -1;
	}

	return 0;
}

int H264Encoder::Encode(unsigned char  *buf, int size)
{
	return 0;
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