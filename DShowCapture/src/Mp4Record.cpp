#include "Mp4Record.h"

Mp4Record::Mp4Record()
{

}

Mp4Record::~ Mp4Record()
{

}

int Mp4Record::InitRecord(const char * filename, int width, int height, int fps)
{
	if(filename == NULL)
	{
		return -1;
	}
	
	av_register_all();
	avformat_alloc_output_context2(&pFormatCtx, NULL, "mp4", filename);

	pVideoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if(!pVideoCodec)
	{
		return -1;
	}

	pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);
	pVideoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pVideoCodecCtx->width = width;
	pVideoCodecCtx->height = height;
	pVideoCodecCtx->time_base = (AVRational ) {1, fps};
	pVideoCodecCtx->bit_rate = 400000 * 4;
	pVideoCodecCtx->gop_size = fps;	
	pVideoCodecCtx->max_b_frames = 1;
	
	if (pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
		pVideoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


	AVDictionary *param = 0;

	av_opt_set(pVideoCodecCtx->priv_data, "preset", "ultrafast", 0);
	av_opt_set(pVideoCodecCtx->priv_data, "tune", "zerolatency", 0);	
}

int Mp4Record::StartRecord()
{

}

int Mp4Record::StopRecord()
{

}