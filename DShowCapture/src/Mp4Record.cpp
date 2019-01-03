#include "Mp4Record.h"
#include "stdafx.h"
Mp4Record::Mp4Record()
{
	m_bStart = false;
	m_mutex = PTHREAD_MUTEX_INITIALIZER;	
}

Mp4Record::~ Mp4Record()
{
	m_bStart = false;
}

int Mp4Record::InitRecord(const char * filename, RECORD_PARAMS params)
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
		TRACE("find encoder failed!\n");
		return -1;
	}

	pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);
	pVideoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pVideoCodecCtx->width = params.stVidParams.nWidth;
	pVideoCodecCtx->height = params.stVidParams.nHeight;
	pVideoCodecCtx->time_base.num = 1;
	pVideoCodecCtx->time_base.den = params.stVidParams.nFrameRate;
	pVideoCodecCtx->bit_rate = params.stVidParams.nBitRate;
	pVideoCodecCtx->gop_size = params.stVidParams.nFrameRate;	
	pVideoCodecCtx->max_b_frames = 1;
	
	if (pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
		pVideoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	av_opt_set(pVideoCodecCtx->priv_data, "preset", "ultrafast", 0);
	av_opt_set(pVideoCodecCtx->priv_data, "tune", "zerolatency", 0);	

	if(avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0)
	{
		TRACE("open encoder failed!\n");
		return -1;
	}

	pVideoStream = avformat_new_stream(pFormatCtx, pVideoCodec);
	if(pVideoStream == NULL)
	{
		TRACE("new video stream( failed!\n");
		return -1;
	}
	pVideoStream->time_base.num = 1;
	pVideoStream->time_base.den = params.stVidParams.nFrameRate;
	pVideoStream->codec = pVideoCodecCtx;

	pFrameYUV  = av_frame_alloc();	
	uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pVideoCodecCtx->width, pVideoCodecCtx->height));

	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pVideoCodecCtx->width, pVideoCodecCtx->height);	
	
	pAudioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if(pAudioCodec == NULL)
	{
		TRACE("find audio encoder failed!\n");
		return -1;
	}

	pAudioCodecCtx = avcodec_alloc_context3(pAudioCodec);
	pAudioCodecCtx->bit_rate = params.stAudParams.nBitRate;	
	pAudioCodecCtx->sample_rate = 44100;
	pAudioCodecCtx->channels = av_get_channel_layout_nb_channels(pAudioCodecCtx->channels);
	pAudioCodecCtx->channel_layout = AV_CH_LAYOUT_MONO;
	pAudioCodecCtx->sample_fmt = pAudioCodec->sample_fmts ? pAudioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
	pAudioCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
	int nRet=avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) ;
	if(nRet< 0)
	{
		TRACE("open audio encoder failed! nRet:%d\n", nRet);
		return -1;
	}	

	pAudioStream = avformat_new_stream(pFormatCtx, pAudioCodec);
	if(pVideoStream == NULL)
	{
		TRACE("new audio stream failed!\n");	
		return -1;
	}
	pAudioStream->time_base.num = 1;
	pAudioStream->time_base.den = pAudioCodecCtx->sample_rate;
	pAudioStream->codec = pAudioCodecCtx;	
	pFramePCM  = av_frame_alloc();	
	int size = av_samples_get_buffer_size(NULL,pAudioCodecCtx->channels,
			pAudioCodecCtx->frame_size, pAudioCodecCtx->sample_fmt,0);

	uint8_t *frame_buf = (uint8_t *)av_malloc(size);
	memset(frame_buf, 0, size);
	avcodec_fill_audio_frame(pFramePCM, pAudioCodecCtx->channels, pAudioCodecCtx->sample_fmt, (const uint8_t *)frame_buf, size, 0);


	if (avio_open(&pFormatCtx->pb, filename, AVIO_FLAG_READ_WRITE) < 0){
		TRACE("avio_open encoder failed!\n");		
		return -1;
	}

	av_dump_format(pFormatCtx, 0, filename, 1);	
	avformat_write_header(pFormatCtx, NULL);

	startTime = av_gettime();

	return 0;
}

int Mp4Record::StartRecord(FrameQueue *video_queue, FrameQueue *audio_queue)
{
	m_bStart = true;
	m_stVQueue = video_queue;
	m_stAQueue = audio_queue;

	pthread_create(&m_RecVidThrID, NULL, RecordVideoThread, this);
	pthread_create(&m_RecAudThrID, NULL, RecordAudioThread, this);

	return 0;
}

int Mp4Record::StopRecord()
{
	m_bStart = false;
	pthread_mutex_lock(&m_mutex);
	int nRet = av_write_trailer(pFormatCtx);
	pthread_mutex_unlock(&m_mutex);
	if(pVideoStream)
	{
		avcodec_close(pVideoStream->codec);
	}

	if(pAudioStream)
	{
		avcodec_close(pAudioStream->codec);
	}
	
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);


	return 0;
}


void *RecordVideoThread(void *param)
{
	int nRet;
	int nGotFrame = 0;
	
	Mp4Record *pMp4Record = (Mp4Record *)param;
	while(1)
	{
		if(pMp4Record->m_bStart)
		{
			pthread_mutex_lock(&pMp4Record->m_mutex);
			StreamBuf buf;
			frame_queue_get(pMp4Record->m_stVQueue, &buf, 1);

			pMp4Record->stVideoPkt.data = NULL;
			pMp4Record->stVideoPkt.size = 0;
			av_init_packet(&pMp4Record->stVideoPkt);

			int nSize = pMp4Record->pVideoCodecCtx->width * pMp4Record->pVideoCodecCtx->height;
			pMp4Record->pFrameYUV->format = AV_PIX_FMT_YUV420P;
			pMp4Record->pFrameYUV->width = pMp4Record->pVideoCodecCtx->width;
			pMp4Record->pFrameYUV->height= pMp4Record->pVideoCodecCtx->height;			

			memcpy(pMp4Record->pFrameYUV->data[0], buf.frame, nSize);	
			memcpy(pMp4Record->pFrameYUV->data[1], buf.frame + nSize, nSize / 4);
			memcpy(pMp4Record->pFrameYUV->data[2], buf.frame + nSize * 5 / 4, nSize / 4);	
			
			nRet = avcodec_encode_video2(pMp4Record->pVideoCodecCtx, &pMp4Record->stVideoPkt, pMp4Record->pFrameYUV, &nGotFrame);
			if(nGotFrame)
			{
				pMp4Record->stVideoPkt.stream_index = pMp4Record->pVideoStream->index;

				AVRational time_base = pMp4Record->pFormatCtx->streams[0]->time_base;
				AVRational r_framerate1 = {pMp4Record->pVideoCodecCtx->time_base.den, 1};
		
				AVRational time_base_q =  {1, AV_TIME_BASE};
				double calc_duration = (double)(AV_TIME_BASE)*(1.0 / av_q2d(r_framerate1));

				int64_t timett = av_gettime();
				int64_t now_time = timett - pMp4Record->startTime;
				pMp4Record->stVideoPkt.pts = av_rescale_q(now_time, time_base_q, time_base);
				pMp4Record->stVideoPkt.dts = pMp4Record->stVideoPkt.pts;
				pMp4Record->stVideoPkt.duration = av_rescale_q(calc_duration, time_base_q, time_base);
				pMp4Record->stVideoPkt.pos = -1;
		
				nRet = av_write_frame(pMp4Record->pFormatCtx, &pMp4Record->stVideoPkt);	
				av_free_packet(&pMp4Record->stVideoPkt);				
			}
			
			pthread_mutex_unlock(&pMp4Record->m_mutex);
		}
		else
		{
			//break;
		}
	}


	return NULL;
}

//FILE *fp1 = fopen("d:\\yuv\\test.pcm", "wb");
void *RecordAudioThread(void *param)
{
	int nRet;
	int nGotFrame = 0;	
	Mp4Record *pMp4Record = (Mp4Record *)param;
	while(1)
	{
		if(pMp4Record->m_bStart)
		{

			StreamBuf buf;
			int nRet = frame_queue_get(pMp4Record->m_stAQueue, &buf, 1);
//			fwrite(buf.frame, buf.bufsize, 1, fp1);

			pMp4Record->pFramePCM->nb_samples = pMp4Record->pAudioCodecCtx->frame_size;
			
			pMp4Record->pFramePCM->format = pMp4Record->pAudioCodecCtx->sample_fmt;
			pMp4Record->pFramePCM->channel_layout = pMp4Record->pAudioCodecCtx->channel_layout;
			pMp4Record->pFramePCM->sample_rate = pMp4Record->pAudioCodecCtx->sample_rate;
/*
			int size = av_samples_get_buffer_size(NULL,pMp4Record->pAudioCodecCtx->channels,
					pMp4Record->pAudioCodecCtx->frame_size,pMp4Record->pAudioCodecCtx->sample_fmt,0);

			uint8_t *frame_buf = (uint8_t *)av_malloc(size);
			memset(frame_buf, 0, size);
			avcodec_fill_audio_frame(pMp4Record->pFramePCM, pMp4Record->pAudioCodecCtx->channels, pMp4Record->pAudioCodecCtx->sample_fmt, (const uint8_t *)frame_buf, size, 0);
*/
			pMp4Record->pFramePCM->data[0] = buf.frame;
			
			pMp4Record->stAudiopkt.data = NULL;
			pMp4Record->stAudiopkt.size = 0;
			av_init_packet(&pMp4Record->stAudiopkt);
	
			pMp4Record->nb_samples += pMp4Record->pFramePCM->nb_samples;
			nRet = avcodec_encode_audio2(pMp4Record->pAudioCodecCtx, &pMp4Record->stAudiopkt, pMp4Record->pFramePCM, &nGotFrame);
			TRACE("---------------nGotFrame: %d, nRet: %d, bufsize:%d\n", nGotFrame,nRet, buf.bufsize);	
	//		av_frame_free(&pMp4Record->pFramePCM);
	/*			
			if(nRet == 0)//if (nGotFrame)
			{

				pMp4Record->stAudiopkt.stream_index = pMp4Record->pAudioStream->index;
				AVRational time_base = pMp4Record->pFormatCtx->streams[pMp4Record->pAudioStream->index]->time_base;

				AVRational r_framerate1 = {pMp4Record->pAudioCodecCtx->sample_rate, 1 };
				AVRational time_base_q =  {1, AV_TIME_BASE};

				double calc_duration = (double)(AV_TIME_BASE)*(1 / av_q2d(r_framerate1));

				int64_t timett = av_gettime();
				int64_t now_time = timett - pMp4Record->startTime;
				pMp4Record->stAudiopkt.pts = av_rescale_q(now_time, time_base_q, time_base);
				pMp4Record->stAudiopkt.dts=pMp4Record->stAudiopkt.pts;
				pMp4Record->stAudiopkt.duration = av_rescale_q(calc_duration, time_base_q, time_base);
		
				pthread_mutex_lock(&pMp4Record->m_mutex);
				nRet = av_write_frame(pMp4Record->pFormatCtx, &pMp4Record->stAudiopkt);
				pthread_mutex_unlock(&pMp4Record->m_mutex);
					
			
				
			}
*/

			
		}
		else
		{

		}
	}


	return NULL;
}

