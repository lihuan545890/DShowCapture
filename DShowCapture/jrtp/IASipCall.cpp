#include "IASipCallCore.h"
#include "IASipCall.h"
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpsessionparams.h>
#include "SdpHandler.h"
#include "payloadtype.h"
#include "telephonyEvent.h"
#include "IAMisc.h"
#include "fmtp.h"
#include "../../../../iActiveSDK_2/av_com/include/iAGUID.h"
#include "../../../../iActiveSDK_2/av_com/comHelper/iAValueCopy_C.h"

using namespace jrtplib;

static I_UINT MAX_FRAME_LEN = 2 * 1024 * 1024; // 2MB
static I_INT MAX_VOC_BUFF_LEN = 480 * 10;
#define IACTIVE_ENC_264_TAIL 8

CIAPhoneCall::CIAPhoneCall(void* pComInterface, void* pAVFManager)
{
	auth_pending    = false;
	lpLastTevPacket = NULL;
	m_bCanSendData  = true;
	m_bHold         = false;
	//memset( m_byRawCapData , 0 , 1024 );
	//m_nCurRawLen    = 0;
	nFramNumber     = 1;
	state           = LCStateInit;
	start_time      = 0;
	bNeedKeyFrame   = false;
	lastTime        = 0;
	m_pFrameData    = NULL;

	nIBFCPFramNumber     = 1;
    m_pIBFCPFrameData    = NULL;

	bDirectCallbyIp = false;
	memset( szUserName , 0x00 , 128 );
	memset( szAddress , 0x00 , 128 );
	memset( szToken , 0x00 ,128 );

	memset( &audio_params , 0x00 , sizeof(audio_params) );
	memset( &video_params , 0x00 , sizeof(video_params) );

	profile         = NULL;

	rtp_session = new RTPSession;
	rtp_session_video = new RTPSession;

	sdpctx = new CSdpHandler;

	h263Enc.SetMaxRTPFrameSize(1200);
	h264Enc.SetMaxRTPFrameSize (1200);

	video_send_thread = NULL;
	audio_send_thread = NULL;

	audio_thread      = NULL;
	video_thread      = NULL;

	//audio & video locker
	//m_AudioDataLocker = CIALocker_C_Create();
	m_VideoDataLocker = CIALocker_C_Create();
	m_RingBufferLocker = CIALocker_C_Create();

	//audio & video array
	m_arrVideoData = CIActPtrList_C_Create(NULL);
	//m_arrAudioData = CIActPtrList_C_Create(NULL);

	//audio & video event
	//m_AudioDataEvent = CIAEvent_C_CreateEvent( I_TRUE , I_FALSE );
	m_VideoDataEvent = CIAEvent_C_CreateEvent( I_TRUE , I_FALSE );
	//CIAEvent_C_ResetEvent( m_AudioDataEvent );
	CIAEvent_C_ResetEvent( m_VideoDataEvent );

	default_astream = NULL;
	default_vstream = NULL;

	m_piiaAudioEncoder = NULL;
	m_piiaAudioDecoder = NULL;
	m_piaComInterface = (IAComManagerInterface*)pComInterface;
	m_piiAVFManager = (IIAAVFrameManagerInterface_C*)pAVFManager;

	// Create ring buffer
	IAComManagerInterface_GetClassObject( m_piaComInterface, &RING_BUFFER_HELPER_GUID, (void**)&m_pRingBuf );
	// Init 
	IIARingBufferInterface_C_Init( m_pRingBuf, MAX_VOC_BUFF_LEN );

	m_dwAudWFX = WAVE_FORMAT_HM16;
	m_lpAudResampleDec = NULL;

	m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	m_waveFormat.nChannels = 1;
	m_waveFormat.nSamplesPerSec = 8000;
	m_waveFormat.wBitsPerSample = 16;

	m_bufferSize = 0;

	m_dwTimeLastSendVfu = 0;

	m_pIBFCPProc = NULL;
	ibfcp_rtp_session_video = new RTPSession;
	ibfcp_video_thread = NULL;
	ibfcp_video_send_thread = NULL;
	m_ibfcp_vstream = NULL;

	ibfcph263Enc.SetMaxRTPFrameSize(1200);
	ibfcph264Enc.SetMaxRTPFrameSize (1200);

	m_ibfcpVideoDataLocker = CIALocker_C_Create();
	m_ibfcparrVideoData = CIActPtrList_C_Create(NULL);
	m_ibfcpVideoDataEvent = CIAEvent_C_CreateEvent( I_TRUE , I_FALSE );
}

CIAPhoneCall::~CIAPhoneCall()
{
	WriteLog_C(I_TRUE, "[~CIAPhoneCall]Begin ~CIAPhoneCall");
	if (profile)
	{
		rtp_profile_destroy( (RtpProfile*)profile );
		profile = NULL;
	}

	if (ibfcp_rtp_session_video)
	{
		delete (RTPSession*) ibfcp_rtp_session_video;
		ibfcp_rtp_session_video = NULL;
	}

	if (m_pIBFCPProc)
	{
		delete m_pIBFCPProc;
		m_pIBFCPProc = NULL;
	}

	CIAEvent_C_DestroyEvent(m_ibfcpVideoDataEvent);
	CIActPtrList_C_Destroy(m_ibfcparrVideoData);
	CIALocker_C_Destroy(m_ibfcpVideoDataLocker);

	if (rtp_session)
	{
		delete (RTPSession*) rtp_session;
		rtp_session = NULL;
	}

	if (rtp_session_video)
	{
		delete (RTPSession*) rtp_session_video;
		rtp_session_video = NULL;
	}

	if (sdpctx)
	{
		delete (CSdpHandler*) sdpctx;
		sdpctx = NULL;
	}

	//CIAEvent_C_DestroyEvent(m_AudioDataEvent);
	CIAEvent_C_DestroyEvent(m_VideoDataEvent);

	//CIActPtrList_C_Destroy(m_arrAudioData);
	CIActPtrList_C_Destroy(m_arrVideoData);

	//CIALocker_C_Destroy(m_AudioDataLocker);
	CIALocker_C_Destroy(m_VideoDataLocker);
	CIALocker_C_Destroy(m_RingBufferLocker);

	IA_SAFE_RELEASE_COM_C( m_pRingBuf );
	IA_SAFE_RELEASE_COM_C( m_lpAudResampleDec );
	IA_SAFE_RELEASE_COM_C(m_piiaAudioDecoder);
	IA_SAFE_RELEASE_COM_C(m_piiaAudioEncoder);
}

/* init local support video stream of remote */
bool CIAPhoneCall::InitDefaultVideoStream()
{
	int nIndex = GetSupportVideoStream( &video_params );

	if( nIndex==-1 )    
	{
		WriteLog_C(I_TRUE, "get support video stream failed.\n");
		return false;
	}

	default_vstream = &video_params.vstream[nIndex];

	return true;
}

/* init local support audio stream of remote */
bool CIAPhoneCall::InitDefaultAudioStream()
{
	int nIndex = GetSupportAudioStream( &audio_params , NULL );

	if( nIndex==-1 )   
	{
		WriteLog_C(I_TRUE, "get support audio stream failed.\n");
		return false;
	}

	default_astream = &audio_params.astream[nIndex];

	return true;
}

void CIAPhoneCall::SetCallBack( SIP_VIDEO_DATA_CALLBACK videoCallBack , SIP_AUDIO_DATA_CALLBACK audioCallBack , void* parameter )
{
	OnVideoDataCB = videoCallBack;
	OnAudioDataCB = audioCallBack;
	m_parameter           = parameter; 
}

/*
获取用户信息
*/
void CIAPhoneCall::GetUserInfo( char* pToken , char* pUserName , char* pAddress )
{
	if( !pUserName || !pAddress || !pToken )     return;

	strcpy( pToken , szToken );
	strcpy( pUserName , szUserName );
	strcpy( pAddress , szAddress );
}

/*
获取远端视频支持的视频信息
*/
void CIAPhoneCall::GetCallVideoInfo(int* pProfileID , int* pLevelID , int* pWidth , int* pHeight , int* pBitRate , char* pCodec )
{
	if( default_vstream==NULL )
	{
		WriteLog_C(I_TRUE, "[CIAPhoneCall::GetCallVideoInfo] default_vstream error!!");
		return;
	}

	if(pCodec)
		strcpy( pCodec , default_vstream->vcodec );

	if( !stricmp(default_vstream->vcodec,"H264") )
	{
		h264_profile_t* h264_profile = (h264_profile_t*)default_vstream->vprofile;
		if(pProfileID)
			*pProfileID = h264_profile->profile_idc;
		if(pLevelID)
			*pLevelID = h264_profile->profile_level;
		if(pWidth && pHeight)
		{
			*pWidth     = h264_profile->max_width;
			*pHeight    = h264_profile->max_height;
		}
		if(pBitRate == NULL)
			return;

		if( h264_profile->max_br!=0 )
		{
			*pBitRate = h264_profile->max_br;
			CIASipCallCore* pLocalCore = (CIASipCallCore*)pCore;
			if( *pBitRate > pLocalCore->m_nMaxBandWidth )
				*pBitRate = pLocalCore->m_nMaxBandWidth;
		}
		else
			*pBitRate = default_vstream->vbitrate;
	}
	else if( !stricmp(default_vstream->vcodec,"H263") || !stricmp(default_vstream->vcodec,"H263-1998") )
	{
		h263_profile_t* h263_profile = (h263_profile_t*)default_vstream->vprofile;
		int nW = h263_profile->mpi[0].size.w;
		int nH = h263_profile->mpi[0].size.h;
		
		if(pBitRate)
			*pBitRate = default_vstream->vbitrate;
		if(pWidth && pHeight)
			get_h263_max_video_size( h263_profile , pWidth , pHeight );
	}
}

/*
add to video data send array
*/
void CIAPhoneCall::AddLocalVideoData( void* pAVFrame )
{
	CIALocker_C_Lock( m_VideoDataLocker );

	IAUnkown_AddRef( (IIAAVFrameInterface_C*)pAVFrame );
	CIActPtrList_C_AddTail( m_arrVideoData , pAVFrame , 0 );
	
	CIALocker_C_Unlock( m_VideoDataLocker );
	
	CIAEvent_C_SetEvent(m_VideoDataEvent);
}


int CIAPhoneCall::ReadAudioFromRingBuf(void * buf, int len)
{
	int nReadLen = 0;

	if(!m_pRingBuf )
	{
		return 0;
	}

	CIALocker_C_Lock( m_RingBufferLocker );
	nReadLen = IIARingBufferInterface_C_GetReadAvailable( m_pRingBuf );
	CIALocker_C_Unlock( m_RingBufferLocker );

	if( nReadLen < len )
		return 0;

	// Read the data ...
	CIALocker_C_Lock( m_RingBufferLocker );
	nReadLen = IIARingBufferInterface_C_Read( m_pRingBuf, (BYTE*)buf, len );
	CIALocker_C_Unlock( m_RingBufferLocker );

	return nReadLen;
}

/*
add to audio data send array
*/
void CIAPhoneCall::AddLocalAudioData( void* pAVFrame )
{
	//CIALocker_C_Lock( m_AudioDataLocker );

	//IAUnkown_AddRef( (IIAAVFrameInterface_C*)pAVFrame );
	//CIActPtrList_C_AddTail( m_arrAudioData , pAVFrame , 0 );
	//CIAEvent_C_SetEvent(m_AudioDataEvent);

	//CIALocker_C_Unlock( m_AudioDataLocker );
	if (!m_pRingBuf || !pAVFrame)
	{
		WriteLog_C(I_TRUE, "[AddLocalAudioData] param error!!");
		return;
	}

	IIAAVFrameInterface_C* piiaAVFrame = (IIAAVFrameInterface_C*)pAVFrame;

	BYTE *lpData = IIAAVFrameInterface_C_GetDataPtr( piiaAVFrame );
	int nLen = IIAAVFrameInterface_C_GetAudioBufSize( piiaAVFrame );
	DWORD wfx = IIAAVFrameInterface_C_GetAudioWfx( piiaAVFrame );

	if( wfx != m_dwAudWFX )
	{ // Resample 
		if( !m_lpAudResampleDec ) 
		{
			IAComManagerInterface_GetClassObject( m_piaComInterface, &AUDIO_RESAMPLE_FILTER_GUID, (void**)&m_lpAudResampleDec );
			if( !m_lpAudResampleDec )
				return;
		}

		// Need re init ?
		int nResLen;
		IA_AUDIO_RESAMPLE_PARAM *pCurParam = (IA_AUDIO_RESAMPLE_PARAM*)IIAAVProcessFilterInterface_C_GetCurCfg( m_lpAudResampleDec, &nResLen );
		if( pCurParam->input_wfx != wfx )
		{ 
			IA_WAVE_FORMAT	wfxDest;
			WFXToWaveFormat( m_dwAudWFX, &wfxDest) ;
			IA_WAVE_FORMAT wfxIn ;
			WFXToWaveFormat( wfx, &wfxIn) ;

			IA_AUDIO_RESAMPLE_PARAM resampleParam;
			memset( &resampleParam, 0, sizeof(IA_AUDIO_RESAMPLE_PARAM) );
			resampleParam.cb = sizeof(IA_AUDIO_RESAMPLE_PARAM) ;
			resampleParam.input_channels = wfxIn.nChannels;
			resampleParam.input_rate = wfxIn.nSamplesPerSec;
			resampleParam.input_wfx = wfx;

			resampleParam.output_channels = wfxDest.nChannels;
			resampleParam.output_rate = wfxDest.nSamplesPerSec;
			resampleParam.output_wfx = m_dwAudWFX;

			IIAAVProcessFilterInterface_C_CfgFilter( m_lpAudResampleDec, &resampleParam, resampleParam.cb );

			if( !IIAAVProcessFilterInterface_C_StartFilter( m_lpAudResampleDec, 0, 0 ) )
			{
				WriteLog_C(I_TRUE, "[AddLocalAudioData] StartFilter error!!");
				return;
			}
		}

		IIAAVFrameInterface_C *pRes = IIAAVProcessFilterInterface_C_ProcessAudio( m_lpAudResampleDec, piiaAVFrame );
		if (pRes)
		{
			lpData = IIAAVFrameInterface_C_GetDataPtr(pRes);
			nLen = IIAAVFrameInterface_C_GetAudioBufSize(pRes);
			IIARingBufferInterface_C_Write(m_pRingBuf, lpData, nLen);

			IA_SAFE_RELEASE_COM_C(pRes);
		}
		else
			return;
	}
	else
		IIARingBufferInterface_C_Write( m_pRingBuf, lpData, nLen );
}

/*
声音发送线程
*/
void os_sound_send_thread( void* pParam )
{
	CIAPhoneCall* pCall = (CIAPhoneCall*)pParam;
	int nEncLen = 0;
	int nEncLenOnce = 0;
	int nReadLen = 0;
	int nCmpMode = 0;
	BYTE byEncData[1024];
	BYTE byPCM[2*1024];
	I_BYTE* lpData = NULL;

	if (pCall == NULL)
	{
		WriteLog_C(I_TRUE, "[os_sound_send_thread] param error!!!");
		return;
	}

	WriteLog_C(I_TRUE, "[os_sound_send_thread] thread start.");

	if( !pCall->InitAudioEncoderInterface() )
	{
		WriteLog_C(I_TRUE, "[os_sound_send_thread] failed to init audio encoder!");
		return;
	}

	while(pCall->state == LCStateAVRunning)
	{
		//if( pCall->state != LCStateAVRunning )
		//{
		//	WriteLog_C(I_TRUE, "[os_sound_start_thread]state is changed, state=%d", pCall->state);
		//	break;
		//}
		//编20ms的音频数据
		//nSampleLen = 320;
		//if( pCall->default_astream->local_pt==98/*ilbc*/ || pCall->default_astream->local_pt==4/*g7231*/ )
		//	nSampleLen = 480;

		nReadLen = pCall->ReadAudioFromRingBuf( byPCM, pCall->m_bufferSize );
		if( nReadLen != pCall->m_bufferSize )
		{
			Sleep(5);
			continue;
		}

		IIAAVFrameInterface_C* pEncAVFrame = 
			IIAAVFrameManagerInterface_C_GenerateTempAF( pCall->m_piiAVFManager, (I_BYTE*)byPCM, pCall->m_bufferSize, pCall->m_dwAudWFX );

		nEncLen = 0;

		while(pCall->state == LCStateAVRunning && pCall->m_piiaAudioEncoder)
		{
			nEncLenOnce = IIAAudioCodecInterface_C_Encode(pCall->m_piiaAudioEncoder, pEncAVFrame, &byEncData[nEncLen] );
			IA_SAFE_RELEASE_COM_C(pEncAVFrame);

			nEncLen += nEncLenOnce;

			if( nEncLenOnce == 0 )
				break;
		}

		pCall->OnSendAudioData( (char*)byEncData, nEncLen );
	}

	if (pCall->m_piiaAudioEncoder)
	{
		WriteLog_C(I_TRUE, "[os_sound_send_thread]close audio encoder.");
		IIAAudioCodecInterface_C_CloseEncoder(pCall->m_piiaAudioEncoder);
	}
	else
	{
		WriteLog_C(I_TRUE, "[os_sound_send_thread]audio encoder point is invalid!!");
	}
}

/*
视频发送线程
*/
void os_video_send_thread( void* pParam )
{
	CIAPhoneCall* pCall = (CIAPhoneCall*)pParam;
	IA_VIDEO_INFO* pVideoInfo = NULL;
	I_BYTE* lpData = NULL;
	I_BYTE byDest[1500] = { 0 };
	int nCmpMode = 0;
	int nLen = 0;
	int nFlagPos = 0;
	unsigned int nDestLen = 0;
	unsigned int nInFrameLen = 0;
	unsigned int nFlags = 0;
	unsigned int nEndFrameMark = 0;
	bool bEncodeOK = false;
	int  nSendTimes = 0;

	WriteLog_C(I_TRUE, "[os_video_send_thread] thread start.");

	while(pCall && pCall->state == LCStateAVRunning)
	{	
		//if( pCall->state != LCStateAVRunning )
		//{
		//	WriteLog_C(I_TRUE, "[os_video_send_thread]state is changed, state=%d", pCall->state);
		//	break;
		//}

		if( !CIAEvent_C_WaitForEvent( pCall->m_VideoDataEvent , 1000 , I_TRUE ) )
			continue;

		if ( CIActPtrList_C_IsEmpty( pCall->m_arrVideoData ) )
			continue;

		CIALocker_C_Lock( pCall->m_VideoDataLocker );

		IIAAVFrameInterface_C* pAVFrame = (IIAAVFrameInterface_C*)CIActPtrList_C_GetHead( pCall->m_arrVideoData );
		CIActPtrList_C_RemoveHead( pCall->m_arrVideoData );

		if ( CIActPtrList_C_IsEmpty( pCall->m_arrVideoData ) )
			CIAEvent_C_ResetEvent( pCall->m_VideoDataEvent );
		else
			CIAEvent_C_SetEvent( pCall->m_VideoDataEvent );

		CIALocker_C_Unlock( pCall->m_VideoDataLocker );

		pVideoInfo  = IIAAVFrameInterface_C_GetVidInfo(pAVFrame);
		nCmpMode    = pVideoInfo->nCSP;
		nLen        = IIAAVFrameInterface_C_GetAudioBufSize(pAVFrame);
		lpData      = IIAAVFrameInterface_C_GetDataPtr(pAVFrame);

		// detect if have tail ?
		nFlagPos = nLen - IACTIVE_ENC_264_TAIL;
		
		if( lpData[nFlagPos] == 0 && lpData[nFlagPos+1] == 0  )
		{ // iActiveForamt
			nLen -= IACTIVE_ENC_264_TAIL;
		}
		//WriteLog_C( I_TRUE , "SIP:os_video_send_thread, frameIndex:%d, bH264Tail:%d, dataLen:%d, bH264:%d", lpData[0],  ( lpData[nFlagPos] == 0 && lpData[nFlagPos+1] == 0  ) , nLen , nCmpMode==CSP_H264 );
		/////////////////////////////////

		if( nCmpMode==CSP_H263 )
			nCmpMode = VIDCMP_H263;
		else if( nCmpMode==CSP_H264 )
			nCmpMode = VIDCMP_H264;
		else
		{
			IA_SAFE_RELEASE_COM_C(pAVFrame);
			continue;
		}

		if( lpData[0]==0xff )
		{
			pCall->bNeedKeyFrame = false;
		}

		// video-index
		lpData += 1;
		nLen -= 1;

		nInFrameLen = nLen;
		nFlags = 0;

		while(pCall->state == LCStateAVRunning)
		{
			bEncodeOK = false;
			memset(byDest, 0, 1500);

			if (nCmpMode == VIDCMP_H263)
			{
				nDestLen = 1500;
				bEncodeOK = pCall->h263Enc.EncodeFrames( lpData, nInFrameLen, byDest, nDestLen, nFlags );
			}
			else if(nCmpMode == VIDCMP_H264)
			{
				nDestLen  = 1212;
				bEncodeOK = pCall->h264Enc.EncodeFrames( lpData, nInFrameLen, byDest, nDestLen, nFlags );
			}

			if(bEncodeOK)
			{
				nEndFrameMark = nFlags & PluginCodec_ReturnCoderLastFrame;
				
				pCall->OnSendVideoData( nCmpMode, nEndFrameMark, (char*)&byDest[12], nDestLen-12, false);

				if( nEndFrameMark )  
				{
					//WriteLog_C(I_TRUE, "[os_video_send_thread] send one frames over by rtp.");
					break;
				}
				nSendTimes ++;
				nSendTimes = nSendTimes > 10000 ? 0:nSendTimes;
				if( nSendTimes %3 == 0 )
					Sleep(2);
			}
			else
			{
				WriteLog_C(I_TRUE, "[os_video_send_thread] Encode frames failed!!");
			}
		}

		IA_SAFE_RELEASE_COM_C(pAVFrame);
	}

	WriteLog_C(I_TRUE, "[os_video_send_thread] thread is down.");
}

//#define LEN_G7231_PTIME	24
//#define LEN_GSM_PTIME	33
//#define LEN_PCM_PTIME	160
//#define LEN_ILBC_PTIME	50
//#define LEN_G729_PTIME	20
/*
声音接收线程
*/
void os_sound_start_thread (void *pParam)
{
	CIAPhoneCall* pCall = (CIAPhoneCall*) pParam;
	RTPSession * pSession = NULL;
	int nTevPt = 0;
	int nRemotePt = 0;

	if((pCall == NULL) || (pCall->OnAudioDataCB == NULL))
	{
		WriteLog_C(I_TRUE, "[os_sound_start_thread]param error!");
		return;
	}

	pSession = (RTPSession *)pCall->rtp_session;
	nTevPt = rtp_profile_get_payload_number_from_mime ((RtpProfile*)pCall->profile, "telephone-event");
	nRemotePt = pCall->default_astream->remote_pt;

	WriteLog_C(I_TRUE, "[os_sound_start_thread] thread start.");

	if (!pCall->InitAudioDecoderInterface(nRemotePt))
	{
		WriteLog_C(I_TRUE, "[os_sound_start_thread] failed to init audio decoder!");

		return;
	}

	while (true)
	{	
		if( pCall->state != LCStateAVRunning )
		{
			//WriteLog_C(I_TRUE, "[os_sound_start_thread]state is changed, state=%d", pCall->state);
			break;
		}

		pSession->BeginDataAccess();

		if (pSession->GotoFirstSource())
		{
			do
			{
				RTPPacket *packet = pSession->GetNextPacket();
				if (packet)
				{
// [2016/02/16] add by xj
#if 1
					// 音频数据接收过程中如果对方的格式发生变化，
					// 需要关闭旧解码器，打开新解码器。
					if (packet->GetPayloadType() != nRemotePt)
					{
						nRemotePt = packet->GetPayloadType();

						WriteLog_C(I_TRUE, "[os_sound_start_thread] the remote payload is changed[%d].", nRemotePt);

						if (!pCall->InitAudioDecoderInterface(nRemotePt))
						{
							WriteLog_C(I_TRUE, "[os_sound_start_thread] failed to init audio decoder!");

							pSession->DeletePacket(packet);

							break;
						}
					}
#endif
					pCall->DecodeAndCallbackRawVocData(/* packet->GetPayloadType(),*/ packet->GetPayloadData(), packet->GetPayloadLength() );

					if( packet->GetPayloadType() == nTevPt )
					{
						pCall->CheckDtmfRecved( (void*)packet );
						if(pCall->lpLastTevPacket != NULL)
						{
							delete (RTPPacket*)pCall->lpLastTevPacket;
							pCall->lpLastTevPacket = NULL;
						}
						pCall->lpLastTevPacket = packet;
					}
					else
					{
						pSession->DeletePacket(packet);
					}
				}
				//else
				//{
				//	WriteLog_C(I_TRUE, "remote packet is null!!");
				//}
			} while (pSession->GotoNextSource());
		}

		pSession->EndDataAccess();
		Sleep(1);
	}

	if (pCall->m_piiaAudioDecoder)
	{
		WriteLog_C(I_TRUE, "[os_sound_start_thread]close audio decoder.");
		IIAAudioCodecInterface_C_CloseDecoder(pCall->m_piiaAudioDecoder);
	}
	else
	{
		WriteLog_C(I_TRUE, "[os_sound_start_thread]audio decoder point is invalid!!");
	}
}

/*
视频接收线程
*/
void os_video_start_thread(void * pParam)
{
	CIAPhoneCall* pCall = (CIAPhoneCall*)pParam;
	if (pCall == NULL)
	{
		WriteLog_C(I_TRUE, "[CIAPhoneCall::DecodeAndCallbackRawVocData]param error!");
		return;
	}

	RTPSession* pSession = (RTPSession*)pCall->rtp_session_video;

	if((pCall == NULL) || (pCall->OnVideoDataCB == NULL) || (pSession == NULL))
	{
		WriteLog_C(I_TRUE, "[CIAPhoneCall::DecodeAndCallbackRawVocData]param error!");
		return;
	}

	//get negotiation video support
	((CIASipPhoneCallsManager*)pCall->m_parameter)->OnRcvCallStatus( pCall->cid , LCStateVideoInput );

	WriteLog_C(I_TRUE, "[os_video_start_thread] thread start.");

	while (true)
	{	
		if( pCall->state != LCStateAVRunning )
		{
			//WriteLog_C(I_TRUE, "[os_video_start_thread]state is changed, state=%d", pCall->state);
			break;
		}

		pSession->BeginDataAccess();

		if( pSession->GotoFirstSource() )
		{
			do
			{
				RTPPacket *packet = pSession->GetNextPacket();
				if (packet)
				{
					BYTE * pData = packet->GetPayloadData();
					DWORD dwHeader = *(DWORD*)&pData[0];

					if( packet->GetPayloadType() >= RTP_PROFILE_MAX_PAYLOADS )
						return;

					int nPayloadType = packet->GetPayloadType();
                    int nLocalPt = -1;
					for (std::map <int, int>::iterator it = pCall->m_PayloadCompareMap.begin();
						 it != pCall->m_PayloadCompareMap.end(); 
						 ++it)
					{
						if (it->second == nPayloadType)
						{
							nLocalPt = it->first;
							break;
						}
					}

					if (nLocalPt == -1 && nPayloadType > 0)
					{
                        nLocalPt = nPayloadType;
					}
					//WriteLog_C( I_TRUE , "Receive  payload:%d, LocalPt:%d", nPayloadType, nLocalPt);

					if (nLocalPt < 0)
					{
						return;
					}

					PayloadType *pt = rtp_profile_get_payload( (RtpProfile*)pCall->profile , nLocalPt);
					if(!pt)
					{
						WriteLog_C( I_TRUE , "Receive bad rtp media payload type no adapt with negotiation payload type!!" );
						return;
					}

					if( stricmp(pt->mime_type, "H264") == 0 )
					{
						pCall->OnRcvRtpVideoData( VIDCMP_H264 , false , (char*)packet->GetPacketData() , packet->GetPacketLength() );
					}
					else if( stricmp(pt->mime_type, "H263-1998") == 0 )
					{
						pCall->OnRcvRtpVideoData( VIDCMP_H263 , false , (char*)packet->GetPacketData() , packet->GetPacketLength() );
					}
				}
				
				pSession->DeletePacket(packet);

			}while( pSession->GotoNextSource() );
		}

		pSession->EndDataAccess();

		Sleep (1);
	}

	WriteLog_C(I_TRUE, "[os_video_start_thread] thread is down.");
}

/*
视频接收线程
*/
void os_ibfcpvideo_start_thread(void * pParam)
{
	CIAPhoneCall* pCall = (CIAPhoneCall*)pParam;
	if (pCall == NULL)
	{
		WriteLog_C(I_TRUE, "[CIAPhoneCall::DecodeAndCallbackRawVocData]param error!");
		return;
	}

	RTPSession* pSession = (RTPSession*)pCall->ibfcp_rtp_session_video;

	if((pCall == NULL) || (pCall->OnVideoDataCB == NULL) || (pSession == NULL))
	{
		WriteLog_C(I_TRUE, "[CIAPhoneCall::DecodeAndCallbackRawVocData]param error!");
		return;
	}

	//get negotiation video support
	//((CIASipPhoneCallsManager*)pCall->m_parameter)->OnRcvCallStatus( pCall->cid , LCStateIBFCPVideoStart);

	WriteLog_C(I_TRUE, "[os_ibfcpvideo_start_thread] thread start.");

	while (true)
	{	
		if( pCall->state != LCStateAVRunning )
		{
			//WriteLog_C(I_TRUE, "[os_ibfcpvideo_start_thread]state is changed, state=%d", pCall->state);
			break;
		}

		pSession->BeginDataAccess();

		if( pSession->GotoFirstSource() )
		{
			do
			{
				RTPPacket *packet = pSession->GetNextPacket();
				if (packet)
				{
					BYTE * pData = packet->GetPayloadData();
					DWORD dwHeader = *(DWORD*)&pData[0];

					if( packet->GetPayloadType() >= RTP_PROFILE_MAX_PAYLOADS )
						return;

					int nPayloadType = packet->GetPayloadType();
					PayloadType *pt = rtp_profile_get_payload( (RtpProfile*)pCall->profile , packet->GetPayloadType() );
					if(!pt)
					{
						WriteLog_C( I_TRUE , "Receive bad rtp media payload type no adapt with negotiation payload type!!" );
						return;
					}

					if( stricmp(pt->mime_type, "H264") == 0 )
					{
						pCall->OnRcvIBFCPRtpVideoData( VIDCMP_H264 , true , (char*)packet->GetPacketData() , packet->GetPacketLength() );
					}
					else if( stricmp(pt->mime_type, "H263-1998") == 0 )
					{
						pCall->OnRcvIBFCPRtpVideoData( VIDCMP_H263 , true , (char*)packet->GetPacketData() , packet->GetPacketLength() );
					}
				}

				pSession->DeletePacket(packet);

			}while( pSession->GotoNextSource() );
		}

		pSession->EndDataAccess();

		Sleep (1);
	}

	WriteLog_C(I_TRUE, "[os_ibfcpvideo_start_thread] thread is down.");
}

/*
视频发送线程
*/
void os_ibfcpvideo_send_thread( void* pParam )
{
	CIAPhoneCall* pCall = (CIAPhoneCall*)pParam;
	IA_VIDEO_INFO* pVideoInfo = NULL;
	I_BYTE* lpData = NULL;
	I_BYTE byDest[1500] = { 0 };
	int nCmpMode = 0;
	int nLen = 0;
	int nFlagPos = 0;
	unsigned int nDestLen = 0;
	unsigned int nInFrameLen = 0;
	unsigned int nFlags = 0;
	unsigned int nEndFrameMark = 0;
	bool bEncodeOK = false;
	int  nSendTimes = 0;

	//WriteLog_C(I_TRUE, "[os_ibfcpvideo_send_thread] thread start.");

	while(pCall && pCall->state == LCStateAVRunning)
	{	
		//if( pCall->state != LCStateAVRunning )
		//{
		//	WriteLog_C(I_TRUE, "[os_video_send_thread]state is changed, state=%d", pCall->state);
		//	break;
		//}

		if( !CIAEvent_C_WaitForEvent( pCall->m_ibfcpVideoDataEvent , 1000 , I_TRUE ) )
			continue;

		if ( CIActPtrList_C_IsEmpty( pCall->m_ibfcparrVideoData ) )
			continue;

		CIALocker_C_Lock( pCall->m_ibfcpVideoDataLocker );

		IIAAVFrameInterface_C* pAVFrame = (IIAAVFrameInterface_C*)CIActPtrList_C_GetHead( pCall->m_ibfcparrVideoData );
		CIActPtrList_C_RemoveHead( pCall->m_ibfcparrVideoData );

		if ( CIActPtrList_C_IsEmpty( pCall->m_ibfcparrVideoData ) )
			CIAEvent_C_ResetEvent( pCall->m_ibfcpVideoDataEvent );
		else
			CIAEvent_C_SetEvent( pCall->m_ibfcpVideoDataEvent );

		CIALocker_C_Unlock( pCall->m_ibfcpVideoDataLocker );

		pVideoInfo  = IIAAVFrameInterface_C_GetVidInfo(pAVFrame);
		nCmpMode    = pVideoInfo->nCSP;
		nLen        = IIAAVFrameInterface_C_GetAudioBufSize(pAVFrame);
		lpData      = IIAAVFrameInterface_C_GetDataPtr(pAVFrame);

		// detect if have tail ?
		nFlagPos = nLen - IACTIVE_ENC_264_TAIL;

		if( lpData[nFlagPos] == 0 && lpData[nFlagPos+1] == 0  )
		{ // iActiveForamt
			nLen -= IACTIVE_ENC_264_TAIL;
		}
		//WriteLog_C( I_TRUE , "SIP:os_ibfcpvideo_send_thread, frameIndex:%d, bH264Tail:%d, dataLen:%d, bH264:%d", lpData[0],  ( lpData[nFlagPos] == 0 && lpData[nFlagPos+1] == 0  ) , nLen , nCmpMode==CSP_H264 );
		/////////////////////////////////

		if( nCmpMode==CSP_H263 )
			nCmpMode = VIDCMP_H263;
		else if( nCmpMode==CSP_H264 )
			nCmpMode = VIDCMP_H264;
		else
		{
			IA_SAFE_RELEASE_COM_C(pAVFrame);
			continue;
		}

		if( lpData[0]==0xff )
		{
			pCall->bNeedKeyFrame = false;
		}

		// video-index
		lpData += 1;
		nLen -= 1;

		nInFrameLen = nLen;
		nFlags = 0;

		while(pCall->state == LCStateAVRunning)
		{
			bEncodeOK = false;
			memset(byDest, 0, 1500);

			if (nCmpMode == VIDCMP_H263)
			{
				nDestLen = 1500;
				bEncodeOK = pCall->ibfcph263Enc.EncodeFrames( lpData, nInFrameLen, byDest, nDestLen, nFlags );
			}
			else if(nCmpMode == VIDCMP_H264)
			{
				nDestLen  = 1212;
				bEncodeOK = pCall->ibfcph264Enc.EncodeFrames( lpData, nInFrameLen, byDest, nDestLen, nFlags );
			}

			if(bEncodeOK)
			{
				nEndFrameMark = nFlags & PluginCodec_ReturnCoderLastFrame;

				//WriteLog_C(I_TRUE, "[os_ibfcpvideo_send_thread] send one frames.");
				pCall->OnSendVideoData( nCmpMode, nEndFrameMark, (char*)&byDest[12], nDestLen-12 , true);

				if( nEndFrameMark )  
				{
					//WriteLog_C(I_TRUE, "[os_video_send_thread] send one frames over by rtp.");
					break;
				}
				nSendTimes ++;
				nSendTimes = nSendTimes > 10000 ? 0:nSendTimes;
				if( nSendTimes %3 == 0 )
					Sleep(2);
			}
			else
			{
				WriteLog_C(I_TRUE, "[os_ibfcpvideo_send_thread] Encode frames failed!!");
			}
		}

		IA_SAFE_RELEASE_COM_C(pAVFrame);
	}

	WriteLog_C(I_TRUE, "[os_ibfcpvideo_send_thread] thread is down.");
}

bool CIAPhoneCall::CheckEndDtmf(void*  ppacket, char &chTev)
{
	RTPPacket * packet = (RTPPacket *) ppacket;
	BOOL bEnd = FALSE;
	//	char dtmf = 0;
	int num1 = 0;
	//	num1 = packet->GetPayloadLength()/sizeof (telephone_event_t);
	unsigned char * pData = packet->GetPayloadData();
	telephone_event_t tev;
	memcpy (&tev, pData, sizeof(telephone_event_t) );
	if (tev.E ==1)
	{
		chTev = tev.event;   
		bEnd = TRUE;
	}
	return bEnd;
}

void CIAPhoneCall::CheckDtmfRecved(void* ppacket)
{
	RTPPacket * packet = (RTPPacket *) ppacket;
	bool bEnd = false;
	char chTev = 0;
	if (!lpLastTevPacket)
	{
		bEnd = CheckEndDtmf (packet, chTev);
	}
	else
	{
		RTPPacket * pLastPacket = (RTPPacket *)lpLastTevPacket;
		if (packet->GetTimestamp() == pLastPacket->GetTimestamp())
		{
			unsigned char * pOldData = pLastPacket->GetPayloadData();
			unsigned char * pCurData = packet->GetPayloadData();
			telephone_event_t tv_old, tv_cur;
			memcpy (&tv_old, pOldData, sizeof(telephone_event_t) );
			memcpy (&tv_cur, pCurData, sizeof(telephone_event_t) );
			if (tv_cur.E  == 1)
			{
				if (tv_old.E == 0)
				{
					bEnd = true;
					chTev = tv_cur.event;
				}
			}
		}
		else
		{
			bEnd = CheckEndDtmf(packet, chTev);
		}
	}
	if (bEnd)
	{
		WriteLog_C(I_TRUE, "Receive cur telephone event %d\n", chTev);
	}
}

void CIAPhoneCall::SendVfuRequest()
{
	( (CIASipCallCore*)pCore )->CoreCallSendVfuRequest( did );
}

int CIAPhoneCall::BuildTelephoneEvent(char *packet, unsigned char event, int end, unsigned char volume, unsigned short duration )
{
	telephone_event_t event_hdr;
	event_hdr.event = event;
	event_hdr.R = 0;
	event_hdr.E = end;
	event_hdr.volume = volume;
	event_hdr.duration = htons(duration);
	WriteLog_C(I_TRUE, "send dtmf event %x\n", *(int*) &event_hdr);
	memcpy (packet, &event_hdr, sizeof (telephone_event_t));
	return 0;
}

int CIAPhoneCall::SendTelephoneEvent( char dtmf )
{
	unsigned char tev_type;
	switch( (unsigned char)dtmf )
	{
	case '1':
		tev_type=TEV_DTMF_1;
		break;
	case '2':
		tev_type=TEV_DTMF_2;
		break;
	case '3':
		tev_type=TEV_DTMF_3;
		break;
	case '4':
		tev_type=TEV_DTMF_4;
		break;
	case '5':
		tev_type=TEV_DTMF_5;
		break;
	case '6':
		tev_type=TEV_DTMF_6;
		break;
	case '7':
		tev_type=TEV_DTMF_7;
		break;
	case '8':
		tev_type=TEV_DTMF_8;
		break;
	case '9':
		tev_type=TEV_DTMF_9;
		break;
	case '*':
		tev_type=TEV_DTMF_STAR;
		break;
	case '0':
		tev_type=TEV_DTMF_0;
		break;
	case '#':
		tev_type=TEV_DTMF_POUND;
		break;
	default:
		return -1;
	}

	int nPt = rtp_profile_get_payload_number_from_mime( (RtpProfile*)profile, "telephone-event" );
	char *packet = new char [sizeof (telephone_event_t)];
	int base_time = 160;
	int timestamp = 0;
	for (int i=0; i<7; i++)
	{
		int nRes = 0;
		bool bMark = false;
		BuildTelephoneEvent (packet, tev_type, 0, 10, base_time + i*160);
		if (i == 0)
		{
			bMark = true;
			//			timestamp = 8640;
		}

		nRes = ((RTPSession*)rtp_session)->SendPacket( packet , sizeof(telephone_event_t), nPt, bMark, timestamp);

		if (i == 6)//重复3次结束
		{
			Sleep(19);
			BuildTelephoneEvent (packet, tev_type, 1, 10, base_time + i*160);
			for (int j=0; j<2; j++)
			{
				nRes = ((RTPSession*)rtp_session)->SendPacket (packet, sizeof(telephone_event_t), nPt, bMark, timestamp);
				Sleep (20);
			}
		}
		Sleep (20);
	}

	delete [] packet;
	packet = NULL;

	return 0;
}

void CIAPhoneCall::SendDtmf( char dtmf )
{
	m_bCanSendData = false;

	Sleep (100);

	CIASipCallCore* pSipCore = (CIASipCallCore*)pCore;

	if( pSipCore->m_sip_conf.use_info )
		pSipCore->CoreSendDtmf( dtmf , did );
	else
		SendTelephoneEvent(dtmf);

	m_bCanSendData = true;
}

/*
创建rtp会话,发送音视频数据
*/  //96 119 109 8 0
int CIAPhoneCall::CreateRtpSession( void* rtp_session , VideoStream* params , int remport_v )
{
	RTPSession *session = (RTPSession *)rtp_session;
	RtpProfile* rtp_profile = (RtpProfile*)profile;

	RTPSessionParams sessionparams;
	RTPUDPv4TransmissionParams transparams;
	int nRet = -1;

	WriteLog_C( I_TRUE, "params->local_pt:%d!!", params->local_pt );

	sessionparams.SetOwnTimestampUnit(1.0/(double)rtp_profile->payload[params->local_pt]->clock_rate);
	transparams.SetPortbase(params->localport);

	nRet = session->Create(sessionparams, &transparams);

	if(nRet < 0)
	{
		WriteLog_C( I_TRUE, "[CIAPhoneCall::CreateRtpSession]Open local rtp port[%d] error!!", transparams.GetPortbase() );
		
		return nRet;
	}
	
	RTPIPv4Address addr( htonl(inet_addr(params->remoteaddr)) , params->remoteport );

	WriteLog_C( I_TRUE, "[CIAPhoneCall::CreateRtpSession]params->remoteaddr=%s, params->remoteport = %d\n", params->remoteaddr, params->remoteport );

	if( params->remoteport != 0)
		nRet = session->AddDestination(addr);

	session->SetDefaultMark(false);

	return 0;
}

void CIAPhoneCall::EndCall()
{
	m_PayloadCompareMap.clear();

	state = LCStateInit;
	RTPTime delay(0.020);
	delay = RTPTime(0.01);

	if( ibfcp_video_thread != NULL ) 
	{
		IAWaitForThreadExit_C(&ibfcp_video_thread, 3000);
		ibfcp_video_thread = NULL;
	}

	if (ibfcp_video_send_thread != NULL)
	{
		IAWaitForThreadExit_C(&ibfcp_video_send_thread, 3000);
		ibfcp_video_send_thread = NULL;
	}

	if( ibfcp_rtp_session_video )
	{
		((RTPSession *)ibfcp_rtp_session_video)->BYEDestroy (delay,"Time's up",9);
	}

	if (m_pIBFCPProc)
	{
		delete m_pIBFCPProc;
		m_pIBFCPProc = NULL;
	}

	if( audio_send_thread != NULL )
	{
		IAWaitForThreadExit_C(&audio_send_thread, 3000);
		audio_send_thread = NULL;
	}

	if( audio_thread != NULL ) 
	{
		IAWaitForThreadExit_C(&audio_thread, 3000);
		audio_thread = NULL;
	}

	if( rtp_session )
	{
		((RTPSession *)rtp_session)->BYEDestroy (delay, "Time's up", strlen("Time's up"));
	}

	if( video_send_thread!=NULL )
	{
		IAWaitForThreadExit_C(&video_send_thread , 3000);
		video_send_thread = NULL;
	}

	if( video_thread != NULL ) 
	{
		IAWaitForThreadExit_C(&video_thread, 3000);
		video_thread = NULL;
	}

	if( rtp_session_video )
	{
		((RTPSession *)rtp_session_video)->BYEDestroy (delay,"Time's up",9);
	}

	free(m_pFrameData);
	m_pFrameData = NULL;

	free(m_pIBFCPFrameData);
	m_pIBFCPFrameData = NULL;

	FreeVideoStream(&video_params);
}

void CIAPhoneCall::SetCallID( int nCid )
{
	cid = nCid;
	sprintf( szToken , "%d" , nCid );

	WriteLog_C(I_TRUE, "[CIAPhoneCall::SetCallID] new call token=%s", szToken);
}

void CIAPhoneCall::SetCallAddress( char* pAddress )
{
	if( pAddress && strlen(pAddress) )
		strcpy( szAddress , pAddress );
}

void CIAPhoneCall::GetCallToken( char* pszToken )
{
	if( pszToken )
		strcpy( pszToken , szToken );
}

void CIAPhoneCall::SetCallContextID( int nTranssionId , int nDialogId )
{
	tid = nTranssionId;
	did = nDialogId;
}

bool CIAPhoneCall::AcceptCall()
{        
	/* answer receive remote calling */
	((CIASipCallCore*)pCore)->CoreAcceptDialog( tid , ((CSdpHandler*)sdpctx)->m_local_media_sdp_str );
	WriteLog_C( I_TRUE , "Send local sdp Answer:\n%s" , ((CSdpHandler*)sdpctx)->m_local_media_sdp_str );

	//return CreateRtpConnection();
	return true;
}

bool CIAPhoneCall::HaveRemoteSdp()
{
	return (((CSdpHandler*)sdpctx)->m_remote_media_sdp_str)?true:false; 
}

bool CIAPhoneCall::OnCallingAnswer( char* sdp_body )
{
	((CSdpHandler*)sdpctx)->ReadAnswer( sdp_body );
	
	CreateRtpConnection();

	if (m_pIBFCPProc)
	{
		m_pIBFCPProc->CreateBFCPLink();
	}

	return true;
}

bool CIAPhoneCall::CreateRtpConnection()
{
	bool bSupportAudio = InitDefaultAudioStream();
	bool bSupportVideo = InitDefaultVideoStream();

#if 1
	if( !bSupportAudio && !bSupportVideo )
	{
		WriteLog_C(I_TRUE, "[CIAPhoneCall::CreateRtpConnection] must be support audio/video.");
		return false;
	}

	/* create audio rtp session rcv data */
	if( default_astream )
	{
		// hack VideoStream struct
		int nRes = CreateRtpSession( rtp_session , (VideoStream*)default_astream , 0 );
		if (nRes < 0)   return false;
	}

	/* create video rtp session rcv data */
	if( rtp_session_video && default_vstream )
	{
		int nRes = CreateRtpSession( rtp_session_video , default_vstream , 0 );
		if (nRes < 0)   return false;
	}
#endif
	/* set call state */
	state = LCStateAVRunning;

	/* start local receive audio data */
	if( default_astream )
		IACreateThread_C( (void *)os_sound_start_thread, this, &audio_thread );

	/* start local receive video data */
	if( default_vstream )
	{
		IACreateThread_C( (void*)os_video_start_thread, this, &video_thread );
		m_pFrameData = (I_BYTE*)malloc(MAX_FRAME_LEN);
	}

	/* start local send audio data */
	if( default_astream )
		IACreateThread_C( (void*)os_sound_send_thread , this , &audio_send_thread );

	/* start local send video data */
	if( default_vstream )
		IACreateThread_C( (void*)os_video_send_thread , this , &video_send_thread );

	return true;
}

void CIAPhoneCall::RefuseCall()
{
	/* refuse remote calling */
	((CIASipCallCore*)pCore)->CoreRefuseDialog( tid );

	/* terminate session */
	((CIASipCallCore*)pCore)->CoreSendTerminate( cid , did );
}

bool CIAPhoneCall::InitAudioEncoderInterface()
{
	const GUID* pAudioCodecGuid = NULL;
	I_BOOL bRetOpen = I_FALSE;
	int nVocCmp = -1;
	PayloadType *pt = NULL;
	AUDIO_CODEC_PARAM sAcp;

	if (m_piiaAudioEncoder != NULL)
	{
		WriteLog_C(I_TRUE, "[CIAPhoneCall::InitAudioEncoderInterface]audio encoder is opened.");
		
		IIAAudioCodecInterface_C_CloseEncoder(m_piiaAudioEncoder);
		
		return false;
	}

	memset(&sAcp, 0x00, sizeof(AUDIO_CODEC_PARAM));

	nVocCmp = GetVocCmp(default_astream->remote_pt);
	//WriteLog_C(I_TRUE, "[CIAPhoneCall::InitAudioEncoderInterface]nVocCmp=%d", nVocCmp);
	sAcp.CODECID = nVocCmp;

	m_bufferSize = 320;

	switch(nVocCmp){
	case VOCCMP_GSM:
		{
			pAudioCodecGuid = &AUD_CODEC_GSM_GUID;
		}
		break;
	case VOCCMP_ULAW:
		{
			pAudioCodecGuid = &AUD_CODEC_ULAW_GUID;
		}
		break;
	case VOCCMP_ALAW:
		{
			pAudioCodecGuid = &AUD_CODEC_ALAW_GUID;
		}
		break;
	case VOCCMP_ILBC:
		{
			pAudioCodecGuid = &AUD_CODEC_ILBC_GUID;
			m_bufferSize = 480;
		}
		break;
	case VOCCMP_G729:
		{
			pAudioCodecGuid = &AUD_CODEC_G729_GUID;
		}
		break;
	case VOCCMP_SPEEX:
		{
			pAudioCodecGuid = &AUD_CODEC_SPEEX_GUID;
		}
		break;
	case VOCCMP_PCM_HM16:
		{
			pAudioCodecGuid = &AUD_CODEC_PCM_GUID;
		}
		break;
	default:
		WriteLog_C(I_TRUE, "[CIAPhoneCall::InitAudioEncoderInterface]encoder audio compress type is invalid!!");
		return false;
	}

	pt = rtp_profile_get_payload((RtpProfile*)profile, default_astream->remote_pt);
	if(pt)
	{
		if (pt->clock_rate == 16000)
		{
			sAcp.codecWfx = WAVE_FORMAT_16M16;
		}
		else
		{
			sAcp.codecWfx = WAVE_FORMAT_HM16;
		}
	}

	m_dwAudWFX = sAcp.codecWfx;
	sAcp.nBps = default_astream->vbitrate;

	IAComManagerInterface_GetClassObject(m_piaComInterface, pAudioCodecGuid, (void**)&m_piiaAudioEncoder);

	if( m_piiaAudioEncoder )
		bRetOpen = IIAAudioCodecInterface_C_OpenEncoder(m_piiaAudioEncoder, &sAcp);
	
	WriteLog_C(I_TRUE, "[CIAPhoneCall::InitAudioEncoderInterface]:Open encoder Res = %d encoderPt=0x%x", bRetOpen, m_piiaAudioEncoder);
	
	return bRetOpen == I_TRUE;
}

bool CIAPhoneCall::InitAudioDecoderInterface( int nPayloadType )
{
	const GUID* pAudioCodecGuid = NULL;
	I_BOOL bRetOpen = I_FALSE;
	int nVocCmp = -1;
	AUDIO_CODEC_PARAM sAcp;

	if (m_piiaAudioDecoder != NULL)
	{
		//WriteLog_C(I_TRUE, "[CIAPhoneCall::InitAudioDecoderInterface]audio decoder is opened, close the audio decoder.");

		IIAAudioCodecInterface_C_CloseDecoder( m_piiaAudioDecoder );
		IA_SAFE_RELEASE_COM_C( m_piiaAudioDecoder );
		
		//return false;
	}

	nVocCmp = GetVocCmp(nPayloadType);

	//WriteLog_C(I_TRUE, "[CIAPhoneCall::InitAudioDecoderInterface]nVocCmp = %d", nVocCmp);

	memset(&sAcp, 0x00, sizeof(AUDIO_CODEC_PARAM));
	sAcp.CODECID = nVocCmp;
	sAcp.codecWfx = WAVE_FORMAT_HM16;
	sAcp.outWfx = WAVE_FORMAT_16M16;

	switch(nVocCmp){
	case VOCCMP_GSM:
		{
			pAudioCodecGuid = &AUD_CODEC_GSM_GUID;
		}
		break;
	case VOCCMP_ULAW:
		{
			pAudioCodecGuid = &AUD_CODEC_ULAW_GUID;
		}
		break;
	case VOCCMP_ALAW:
		{
			pAudioCodecGuid = &AUD_CODEC_ALAW_GUID;
		}
		break;
	case VOCCMP_ILBC:
		{
			pAudioCodecGuid = &AUD_CODEC_ILBC_GUID;
		}
		break;
	case VOCCMP_G729:
		{
			pAudioCodecGuid = &AUD_CODEC_G729_GUID;
		}
		break;
	case VOCCMP_SPEEX:
		{
			pAudioCodecGuid = &AUD_CODEC_SPEEX_GUID;
		}
		break;
	case VOCCMP_PCM_HM16:
		{
			pAudioCodecGuid = &AUD_CODEC_PCM_GUID;
		}
		break;
	default:
		{
			WriteLog_C(I_TRUE, "[CIAPhoneCall::InitAudioDecoderInterface]decoder audio compress type is invalid!!");
			return false;
		}
	}

	IAComManagerInterface_GetClassObject(m_piaComInterface, pAudioCodecGuid, (void**)&m_piiaAudioDecoder);

	if( m_piiaAudioDecoder )
		bRetOpen = IIAAudioCodecInterface_C_OpenDecoder(m_piiaAudioDecoder, &sAcp);

	//WriteLog_C(I_TRUE, "[CIAPhoneCall::InitAudioDecoderInterface]:Open decoder Res = %d decoderPt = 0x%x", bRetOpen , m_piiaAudioDecoder);

	return bRetOpen == I_TRUE;
}

int CIAPhoneCall::GetVocCmp( int nPayloadType )
{
	int nVocCmp = -1;
	PayloadType *pt = rtp_profile_get_payload((RtpProfile*)profile, nPayloadType);

	if (pt == NULL)
		return nVocCmp;

	if (stricmp (pt->mime_type, "PCMU") == 0)
		nVocCmp = VOCCMP_ULAW;
	else if (stricmp (pt->mime_type, "PCMA") == 0)
		nVocCmp = VOCCMP_ALAW;
	else if (stricmp (pt->mime_type, "GSM") == 0)
		nVocCmp = VOCCMP_GSM;
	else if (stricmp (pt->mime_type, "G729") == 0)
		nVocCmp = VOCCMP_G729;
	else if (stricmp (pt->mime_type, "iLBC") == 0)
		nVocCmp = VOCCMP_ILBC;
	else if (stricmp (pt->mime_type, "speex") == 0)
		nVocCmp = VOCCMP_SPEEX;

	return nVocCmp;
}

/* 对收到的声音数据解码后回调上层 */
void CIAPhoneCall::DecodeAndCallbackRawVocData(/* int nPayloadType,*/ unsigned char * lpData, int nLen )
{
	IIAAVFrameInterface_C* pAvFrame = NULL;

	if( (lpData == NULL) || (nLen == 0) || !m_piiaAudioDecoder )
	{
		WriteLog_C(I_TRUE, "[DecodeAndCallbackRawVocData] param error!!");
		return;
	}

	//WriteLog_C(I_TRUE, "[DecodeAndCallbackRawVocData] nLen=%d", nLen);

#if 1
	while (true) 
	{
		pAvFrame = IIAAudioCodecInterface_C_Decode(m_piiaAudioDecoder, (I_BYTE*)lpData, (I_INT)nLen, I_TRUE);
		lpData = NULL;

		if (pAvFrame == NULL)
		{
			//WriteLog_C(I_TRUE, "[DecodeAndCallbackRawVocData] pAvFrame == NULL.");
			break;
		}

		(*OnAudioDataCB)( szToken , WAVE_FORMAT_16M16 , 
			(char*)IIAAVFrameInterface_C_GetDataPtr(pAvFrame), 
			IIAAVFrameInterface_C_GetAudioBufSize(pAvFrame), 
			m_parameter );

		IA_SAFE_RELEASE_COM_C(pAvFrame);
	} ;
#else
	(*OnAudioDataCB)(szToken , WAVE_FORMAT_HM16 , (char*)lpData, nLen, m_parameter);
#endif
}

/* send audio through rtp session */
void CIAPhoneCall::OnRtpAudioOutData(I_BYTE *pData, int nLen)
{
	int timestamp = 160;
	if( default_astream->local_pt == 97 || default_astream->local_pt == 4)
		timestamp = 240;
	//WriteLog_C( true , "send Audio pt = %d \n" , default_astream->remote_pt );
	int nRes = ((RTPSession*)rtp_session)->SendPacket((char *)pData, nLen, default_astream->remote_pt, false, timestamp);
}

/* rcv rtp session video data */
void CIAPhoneCall::OnRcvRtpVideoData( int nCmpMode , bool bH239Data , char *pData , int nLen )
{
	if ((pData == NULL) || (nLen == 0))
	{
		WriteLog_C(I_TRUE, "[CIAPhoneCall::OnRcvRtpVideoData]:data error!");
		return;
	}

	//WriteLog_C( I_TRUE , "Receive RTP Packet! %d",  nCmpMode);

	I_BYTE *byFrameData = m_pFrameData;
	unsigned int nFrameDataLen = MAX_FRAME_LEN;
	unsigned int nFlags = 0;
	unsigned int nRawDataLen = nLen;
	int nOff = 0;

//	int nKeyPos = nOff++;						//frame no;
	nOff += 6;  // [cmpCodec-1][width-2][height-2][frameIndex]
	int sizeX = 0 , sizeY = 0;
	bool bDeocdeOK = false;
	switch(nCmpMode)
	{
	case VIDCMP_H263:
		bDeocdeOK = h263Dec.DecodeFrames( (BYTE*)pData, nRawDataLen, (BYTE*)&byFrameData[nOff], nFrameDataLen, nFlags, sizeX, sizeY);
		break;
	case VIDCMP_H264:
		{
			//WriteLog_C( I_TRUE , "Decode H264 Frames!");
		bDeocdeOK = h264Dec.DecodeFrames( (BYTE*)pData, nRawDataLen, (BYTE*)&byFrameData[nOff], nFrameDataLen, nFlags, sizeX, sizeY);
		}
		break;
	default:
		break;
	}

	if( !bDeocdeOK )
		return;

	if ( nFlags & PluginCodec_ReturnCoderRequestIFrame )
	{
		if( IAGetCurrentTime() - m_dwTimeLastSendVfu > 3000 )
		{
			m_dwTimeLastSendVfu = IAGetCurrentTime();
			SendVfuRequest();
		}
		return;
	}

	if ( nFlags & PluginCodec_ReturnCoderLastFrame )
	{
		if ( nFlags & PluginCodec_ReturnCoderIFrame )
		{
			nFramNumber = 0xFF;
		}
		
		nOff = 0;
		byFrameData[nOff] = nCmpMode;																						nOff ++;
		IA_memcpy_int16_short( &byFrameData[nOff], sizeX);															nOff += LEN_WORD;
		IA_memcpy_int16_short( &byFrameData[nOff], sizeY);															nOff += LEN_WORD;
		byFrameData[nOff] = (nFramNumber != 0xff) ? nFramNumber++ : nFramNumber;				nOff ++;
		
		if( nFramNumber>200 || nFramNumber==0xff )	nFramNumber = 1;

		//WriteLog_C( I_TRUE , "Decode H264 Frames Callback!sizeX:%d, sizeY:%d", sizeX , sizeY);
		(*OnVideoDataCB)( szToken , nCmpMode , bH239Data , (char*)byFrameData , nFrameDataLen+6 , sizeX , sizeY , m_parameter );
	}
}

/* rcv rtp session audio data */
void CIAPhoneCall::OnRcvRtpAudioData( int nCmpMode , char *pData , int nLen )
{
	(*OnAudioDataCB)( szToken , nCmpMode , pData , nLen , m_parameter );
}

/*
send audio data through rtp
*/
void CIAPhoneCall::OnSendAudioData(char* lpData , int nLen )
{
	if( !m_bHold && m_bCanSendData )
	{
		OnRtpAudioOutData( (BYTE*)lpData, nLen);
	}
}

/*
send video data through rtp
*/
void CIAPhoneCall::OnSendVideoData( int nCmpMode , bool bMark , char* lpData , int nLen , bool bBFCP)
{
	int nPt = 0;

	if( state != LCStateAVRunning )
		return;

	switch(nCmpMode)
	{
	case VIDCMP_H263:
		nPt = GetSupportVideoStreamPt( &video_params , (char*)"H263-1998" );/*rtp_profile_get_payload_number_from_mime( (RtpProfile*)profile, "H263-1998" );*///payload_type_h263_1998.type;
		break;
	case VIDCMP_H264:
		nPt = GetSupportVideoStreamPt( &video_params , (char*)"H264" );/*rtp_profile_get_payload_number_from_mime( (RtpProfile*)profile, "H264" );*/
		break;
	}

	/*std::map <int, int>::iterator it = m_PayloadCompareMap.find(nPt);
	if (it == m_PayloadCompareMap.end())
	{
		return;
	}

	int remotePt = it->first;
	WriteLog_C( I_TRUE , "Local Pt:%d, Remote Pt:%d\n", nPt, remotePt );*/

	if(m_bCanSendData)
	{
		if( lastTime == 0)
			lastTime = timeGetTime();
		int nTimeStampInc = 0;
		if(bMark)
		{
			nTimeStampInc = timeGetTime()-lastTime;
			lastTime += nTimeStampInc;
		}

		if (bBFCP)
		{
			int nRes = ( (RTPSession*)ibfcp_rtp_session_video)->SendPacket(lpData, nLen, nPt, bMark, nTimeStampInc);
			if( nRes!=0 )
			{
				WriteLog_C( I_TRUE , "Local Send iBFCP Rtp Video Packet failed!!\n" );
			}
		} 
		else
		{
			int nRes = ( (RTPSession*)rtp_session_video )->SendPacket(lpData, nLen, nPt, bMark, nTimeStampInc);
			if( nRes!=0 )
			{
				WriteLog_C( I_TRUE , "Local Send Rtp Video Packet failed!!\n" );
			}
		}
	}
}

/* get specific codec video stream */
int GetSupportVideoStreamByCodecForBFCP( VMediaStream* videostream , char* vcodec , int localport)
{
	int nRes = -1;
	for( int i=0; i<videostream->nCount; i++ )
	{
		WriteLog_C( I_TRUE , "videostream->vstream[i].vcodec:%s, port:%d --%d\n", videostream->vstream[i].vcodec,  videostream->vstream[i].localport, localport);
		if( videostream->vstream[i].bsupport &&
			!stricmp(videostream->vstream[i].vcodec,vcodec) &&
			 videostream->vstream[i].localport == localport)
		{
			nRes = i;
			break;
		}
	}

	return nRes;
}

bool CIAPhoneCall::CreateBFCPRtpConnection(int nMediaType)
{
	//if (ibfcp_video_thread != NULL || ibfcp_video_send_thread != NULL)
	//{
	//	WriteLog_C( I_TRUE , "Has in BFCP, Can not create BFCP Rtp Connection!!\n" );
	//	return false;
	//}

	WriteLog_C( I_TRUE , "Create BFCP Rtp Connection!!\n" );

	if (m_ibfcp_vstream == NULL)
	{
		int localport = ((CSdpHandler*)sdpctx)->m_bfcp_rtp_v_port;

		int nRes = -1;

		//first check h264 video encode
		nRes = GetSupportVideoStreamByCodecForBFCP( &video_params, (char*)"H264", localport);
		if( nRes == -1 )
		{
			//if h264 failed , check h263 video encode 
			nRes = GetSupportVideoStreamByCodecForBFCP( &video_params, (char*)"H263-1998", localport);
			if( nRes == -1 )
			{
				nRes = GetSupportVideoStreamByCodecForBFCP( &video_params, (char*)"H263", localport);
			}
		}

		if( nRes == -1)
			return false;
		WriteLog_C( I_TRUE , "Get Support Video Stream By Codec For BFCP Success!!\n" );

		m_ibfcp_vstream = &video_params.vstream[nRes];

		if (ibfcp_rtp_session_video == NULL)
		{
			ibfcp_rtp_session_video = new RTPSession;
		}

		/* create video rtp session rcv data */
		if( m_ibfcp_vstream )
		{
			int nRes = CreateRtpSession( ibfcp_rtp_session_video , m_ibfcp_vstream , 0 );
			if (nRes < 0)   return false;
		}
	}

	/* set call state */
	state = LCStateAVRunning;

	/* start local receive video data */
	if(nMediaType == IBFCP_MEDIAFLOOR_CTL_RECV)
	{
		WriteLog_C(I_TRUE, "Create thread for recv ibfcp video!!\n");
		m_pIBFCPProc->m_MediaStatus = IBFCP_MEDIAFLOOR_STATUS_RECVING;

		((CIASipPhoneCallsManager*)m_parameter)->OnRcvCallStatus(cid , LCStateIBFCPVideoStart, true);

		if (ibfcp_video_thread == NULL)
		{
			IACreateThread_C( (void*)os_ibfcpvideo_start_thread, this, &ibfcp_video_thread);
			m_pIBFCPFrameData = (I_BYTE*)malloc(MAX_FRAME_LEN);
			SendVfuRequest();
		}
	}

	if(nMediaType == IBFCP_MEDIAFLOOR_CTL_SEND)
	{
		WriteLog_C(I_TRUE, "Create thread for send ibfcp video!!\n");
		m_pIBFCPProc->m_MediaStatus = IBFCP_MEDIAFLOOR_STATUS_SENDING;

		((CIASipPhoneCallsManager*)m_parameter)->OnRcvCallStatus(cid , LCStateIBFCPVideoRequestRes, true);

		if (ibfcp_video_send_thread == NULL)
		{
			IACreateThread_C( (void*)os_ibfcpvideo_send_thread , this , &ibfcp_video_send_thread);
		}
	}

	return true;
}

bool CIAPhoneCall::ReleaseBFCPRtpConnection(int nMediaType)
{
	if (nMediaType == IBFCP_MEDIAFLOOR_CTL_RECV) 
	{
		WriteLog_C( I_TRUE , "Release thread for recv ibfcp video!!\n" );

		((CIASipPhoneCallsManager*)m_parameter)->OnRcvCallStatus(cid , LCStateIBFCPVideoEnd, true);

		//if( ibfcp_rtp_session_video )
		//{
		//	RTPTime delay(0.020);
		//	delay = RTPTime(0.01);
		//	((RTPSession *)ibfcp_rtp_session_video)->BYEDestroy (delay,"Time's up",9);
		//}

		m_pIBFCPProc->m_MediaStatus = IBFCP_MEDIAFLOOR_STATUS_NOMEDIA;

		if (ibfcp_video_thread != NULL )
		{
			//delete m_pIBFCPFrameData;
			//m_pIBFCPFrameData = NULL;
			//IAWaitForThreadExit_C(&ibfcp_video_thread, 3000);
			//IACloseHandle(ibfcp_video_thread);
			//ibfcp_video_thread = NULL;
		}
	}

	if (nMediaType == IBFCP_MEDIAFLOOR_CTL_SEND)
	{
		WriteLog_C( I_TRUE , "Release thread for send ibfcp video!!\n" );

		((CIASipPhoneCallsManager*)m_parameter)->OnRcvCallStatus(cid , LCStateIBFCPVideoSendEnd, true);

		//if( ibfcp_rtp_session_video )
		//{
		//	RTPTime delay(0.020);
		//	delay = RTPTime(0.01);
		//	((RTPSession *)ibfcp_rtp_session_video)->BYEDestroy (delay,"Time's up",9);
		//}

		m_pIBFCPProc->m_MediaStatus = IBFCP_MEDIAFLOOR_STATUS_NOMEDIA;

		if (ibfcp_video_send_thread != NULL)
		{
			//IAWaitForThreadExit_C(&ibfcp_video_send_thread, 3000);
			//IACloseHandle(ibfcp_video_send_thread);
			//ibfcp_video_send_thread = NULL;
		}
	}
	return true;
}

/* rcv rtp session video data */
void CIAPhoneCall::OnRcvIBFCPRtpVideoData( int nCmpMode , bool bH239Data , char *pData , int nLen )
{
	if ((pData == NULL) || (nLen == 0))
	{
		WriteLog_C(I_TRUE, "[CIAPhoneCall::OnRcvRtpVideoData]:data error!");
		return;
	}

	//WriteLog_C( I_TRUE , "Receive RTP Packet! %d",  nCmpMode);

	I_BYTE *byFrameData = m_pIBFCPFrameData;
	unsigned int nFrameDataLen = MAX_FRAME_LEN;
	unsigned int nFlags = 0;
	unsigned int nRawDataLen = nLen;
	int nOff = 0;

	//	int nKeyPos = nOff++;						//frame no;
	nOff += 6;  // [cmpCodec-1][width-2][height-2][frameIndex]
	int sizeX = 0 , sizeY = 0;
	bool bDeocdeOK = false;
	switch(nCmpMode)
	{
	case VIDCMP_H263:
		bDeocdeOK = ibfcph263Dec.DecodeFrames( (BYTE*)pData, nRawDataLen, (BYTE*)&byFrameData[nOff], nFrameDataLen, nFlags, sizeX, sizeY);
		break;
	case VIDCMP_H264:
		{
			//WriteLog_C( I_TRUE , "Decode H264 Frames!");
			bDeocdeOK = ibfcph264Dec.DecodeFrames( (BYTE*)pData, nRawDataLen, (BYTE*)&byFrameData[nOff], nFrameDataLen, nFlags, sizeX, sizeY);
		}
		break;
	default:
		break;
	}

	if( !bDeocdeOK )
		return;

	if ( nFlags & PluginCodec_ReturnCoderRequestIFrame )
	{
		if( IAGetCurrentTime() - m_dwTimeLastSendVfu > 3000 )
		{
			m_dwTimeLastSendVfu = IAGetCurrentTime();
			SendVfuRequest();
		}
		return;
	}

	if ( nFlags & PluginCodec_ReturnCoderLastFrame )
	{
		if ( nFlags & PluginCodec_ReturnCoderIFrame )
		{
			nIBFCPFramNumber = 0xFF;
		}

		nOff = 0;
		byFrameData[nOff] = nCmpMode;																						nOff ++;
		IA_memcpy_int16_short( &byFrameData[nOff], sizeX);															nOff += LEN_WORD;
		IA_memcpy_int16_short( &byFrameData[nOff], sizeY);															nOff += LEN_WORD;
		byFrameData[nOff] = (nIBFCPFramNumber != 0xff) ? nIBFCPFramNumber++ : nIBFCPFramNumber;				nOff ++;

		if( nIBFCPFramNumber>200 || nIBFCPFramNumber==0xff )	nIBFCPFramNumber = 1;

		//WriteLog_C( I_TRUE , "Decode H264 Frames Callback!sizeX:%d, sizeY:%d", sizeX , sizeY);
		(*OnVideoDataCB)( szToken , nCmpMode , bH239Data , (char*)byFrameData , nFrameDataLen+6 , sizeX , sizeY , m_parameter );
	}
}

/*
add to video data send array
*/
void CIAPhoneCall::AddIBFCPVideoData( void* pAVFrame )
{
	CIALocker_C_Lock( m_ibfcpVideoDataLocker );

	IAUnkown_AddRef( (IIAAVFrameInterface_C*)pAVFrame );
	CIActPtrList_C_AddTail( m_ibfcparrVideoData , pAVFrame , 0 );

	CIALocker_C_Unlock( m_ibfcpVideoDataLocker );

	CIAEvent_C_SetEvent(m_ibfcpVideoDataEvent);
}