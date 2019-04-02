// DirectDraw.cpp: implementation of the CDirectDraw class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "DirectDraw.h"

//#define RGBFMT
#pragma comment(lib,"ddraw.lib")
#pragma comment(lib,"dxguid.lib")

CDirectDraw::CDirectDraw()
{
	memset(&this->ddsd,0,sizeof(this->ddsd));// ���DirectDraw���������ṹ��
	this->lpDD=NULL; 				// DirectDraw����ָ�����
	this->lpDDSPrimary=NULL;			// DirectDraw������ָ�����
	this->lpDDSOffscreen=NULL; 			// DirectDraw��������ָ�����
	this->lpClipper=NULL; 			// DirectDraw�ü��������

	this->bitmap_width=0;			// ����ʾͼ��Ŀ��
	this->bitmap_height=0;			// ����ʾͼ��ĸ߶�
}

CDirectDraw::~CDirectDraw()
{
	this->ReleaseDirectDraw( );
}

BOOL CDirectDraw::InitDirectDraw(HWND hwnd , int width , int height)
{
	if( !::IsWindow(hwnd) ) return FALSE;


	m_hwnd = hwnd;
	 // ����DirectCraw����
	if (DirectDrawCreateEx(
			NULL,			//ָ��DirectDraw�ӿڵ�GUID��ָ�룬NULL��ʾ����Ĭ��
			(VOID**)&lpDD,	//�������ܳ�ʼ����DirectDraw����ĵ�ַ
			IID_IDirectDraw7,	//IID_IDirectDraw7����ǰ�汾
			 NULL) != DD_OK) 	//NULL, ����
	{
		return FALSE;
	}

	// ����DirectDraw���Ƽ�
	if( FAILED ( this->lpDD->SetCooperativeLevel( 
			hwnd,			//��DirectDraw������ϵ������
			DDSCL_NORMAL | DDSCL_NOWINDOWCHANGES ) ) )//���Ƽ���־
	{
		return FALSE;
	}

	//���DirectDraw���������ṹ��
	ZeroMemory(&ddsd,sizeof(ddsd));		
	// �������������
	ddsd.dwSize = sizeof(ddsd);		//DirectDraw���������ṹ���С
	ddsd.dwFlags = DDSD_CAPS;		//�趨DDSURFACEDESC2�ṹ�е�ddsCaps��Ч
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;//������
	
	//1. ����������
	if (lpDD->CreateSurface( &ddsd, //������˱�����Ϣ��DDSURFACEDESC2�ṹ�ĵ�ַ
				    &lpDDSPrimary,	//����������ָ��
				    NULL) != DD_OK)	//NULL, ����
	{
		return FALSE;
	}

	//���ü���
	if( lpDD->CreateClipper( 0, 		//���ڲ��ã�������Ϊ0
				    &lpClipper,	//ָ������������ָ��
				     NULL ) != DD_OK )//NULL
		return FALSE;
	//�ü�������ʾ������ϵ
    if( lpClipper->SetHWnd( 0, hwnd ) != DD_OK )
    {
        lpClipper->Release();
        return FALSE;
    }
	//�Ѳü����ӵ�������
    if( lpDDSPrimary->SetClipper( lpClipper ) != DD_OK )
    {
        lpClipper->Release();
        return FALSE;
    }
    // Done with clipper
    lpClipper->Release();

	//2. ����YUV����
	ZeroMemory(&ddsd, sizeof(ddsd));	// ��ձ��������� 
	ddsd.dwSize = sizeof(ddsd);
	// ��������
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;//DDSCAPS_OVERLAY | DDSCAPS_OFFSCREENPLAIN;
	// ����־
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;//���������ؽṹ
	ddsd.dwWidth = width;	//������ʾ�Ŀ�
	ddsd.dwHeight = height;	//������ʾ�ĸ�
	ddsd.ddpfPixelFormat.dwFlags  = DDPF_FOURCC | DDPF_YUV ;		// ����ĸ��ַ���YUV��־
	ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y','V', '1', '2');	// �ĸ��ַ�ΪYV12
	ddsd.ddpfPixelFormat.dwYUVBitCount = 8;							// YUVλ��

	// ����YUV����
	if (lpDD->CreateSurface(&ddsd, &lpDDSOffscreen, NULL) != DD_OK)//
	{
		return FALSE;
	}

	this->bitmap_width = width;		//ͼ��Ŀ�
	this->bitmap_height = height;	//ͼ��ĸ�

	// ����ʾ��ͼ�񴰿�
	rctSour.left = 0;
	rctSour.top = 0;
	rctSour.right = ddsd.dwWidth;
	rctSour.bottom = ddsd.dwHeight;

	return TRUE;
}

void CDirectDraw::ReleaseDirectDraw( void )
{
	if( this->lpClipper ){	// �ͷŲü���
		this->lpClipper->Release(); this->lpClipper=NULL;
	}
	if( this->lpDDSOffscreen ){// �ͷ���������
		this->lpDDSOffscreen->Release(); this->lpDDSOffscreen=NULL;  
	}
	if( this->lpDDSPrimary ) {// �ͷ�������
		this->lpDDSPrimary->Release(); this->lpDDSPrimary=NULL;
	}
	if( this->lpDD ){		// �ͷ�DirectDraw����
		this->lpDD->Release(); this->lpDD=NULL;  
	}
}

BOOL CDirectDraw::DrawDirectDraw(void * buffer)
{
	HRESULT ddRval;

	if( buffer==NULL) 
		return FALSE;
	
	// ��ȡĿ��ͻ�������
	GetClientRect(m_hwnd,&rcDest);
	// �Ѵ�������ת��Ϊ��Ļ����
	ClientToScreen(m_hwnd, (LPPOINT)&rcDest.left);
	ClientToScreen(m_hwnd, (LPPOINT)&rcDest.right);
	
	// ��ѯ������������
	do {
		ddRval = lpDDSOffscreen->Lock(NULL, // ָ��ĳ��RECT��ָ�룬��ָ������������ҳ����������ò���Ϊ NULL������ҳ�潫������ 
									&ddsd,	// DDSURFACEDESC�ṹ�ĵ�ַ���������ҳ��������Ϣ
									DDLOCK_WAIT | DDLOCK_WRITEONLY,//������־
									NULL);	// NULL
	} while(ddRval == DDERR_WASSTILLDRAWING);// �鴫������æ��������ѯ
	
	// �������
	if(ddRval != DD_OK)
		return 1;
	// ��������ʾͼ���������ڴ�
	CopyToDDraw( (LPBYTE)ddsd.lpSurface , buffer);
	// ������������
	lpDDSOffscreen->Unlock(NULL);
	// �����������YUVԴͼ��rctSour�������������rcDestĿ����
	this->lpDDSPrimary->Blt( &rcDest , this->lpDDSOffscreen , rctSour, DDBLT_WAIT, NULL );	
	return 1;
}

void CDirectDraw::CopyToDDraw( void * destination_buffer , void * source_buffer )
{
	if( ! destination_buffer || ! source_buffer ) 
		return;

	unsigned int i;
	BYTE* lpSurf = (BYTE *)destination_buffer;

	BYTE* lpY = (BYTE *)source_buffer;
	BYTE* lpU = (BYTE *)source_buffer + bitmap_width * bitmap_height;
	BYTE* lpV = (BYTE *)source_buffer + bitmap_width * bitmap_height * 5 / 4;	

	// fill Y data
	for(i=0; i<ddsd.dwHeight; i++)
	{
		memcpy(lpSurf, lpY, ddsd.dwWidth);
		lpY += bitmap_width;
		lpSurf += ddsd.lPitch;
	}
	
	// fill V data
	for(i=0; i<ddsd.dwHeight/2; i++)
	{
		memcpy(lpSurf, lpV, ddsd.dwWidth / 2);
		lpV += bitmap_width / 2;
		lpSurf += ddsd.lPitch / 2;
	}
	
	// fill U data
	for(i=0; i<ddsd.dwHeight/2; i++)
	{
		memcpy(lpSurf, lpU, ddsd.dwWidth / 2);
		lpU += bitmap_width / 2;
		lpSurf += ddsd.lPitch / 2;
	}

}