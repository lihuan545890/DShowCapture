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
	memset(&this->ddsd,0,sizeof(this->ddsd));// 清空DirectDraw表面描述结构体
	this->lpDD=NULL; 				// DirectDraw对象指针清空
	this->lpDDSPrimary=NULL;			// DirectDraw主表面指针清空
	this->lpDDSOffscreen=NULL; 			// DirectDraw离屏表面指针清空
	this->lpClipper=NULL; 			// DirectDraw裁剪对象清空

	this->bitmap_width=0;			// 待显示图象的宽度
	this->bitmap_height=0;			// 待显示图象的高度
}

CDirectDraw::~CDirectDraw()
{
	this->ReleaseDirectDraw( );
}

BOOL CDirectDraw::InitDirectDraw(HWND hwnd , int width , int height)
{
	if( !::IsWindow(hwnd) ) return FALSE;


	m_hwnd = hwnd;
	 // 创建DirectCraw对象
	if (DirectDrawCreateEx(
			NULL,			//指向DirectDraw接口的GUID的指针，NULL表示采用默认
			(VOID**)&lpDD,	//用来接受初始化的DirectDraw对象的地址
			IID_IDirectDraw7,	//IID_IDirectDraw7，当前版本
			 NULL) != DD_OK) 	//NULL, 保留
	{
		return FALSE;
	}

	// 设置DirectDraw控制级
	if( FAILED ( this->lpDD->SetCooperativeLevel( 
			hwnd,			//与DirectDraw对象联系主窗口
			DDSCL_NORMAL | DDSCL_NOWINDOWCHANGES ) ) )//控制级标志
	{
		return FALSE;
	}

	//清空DirectDraw表面描述结构体
	ZeroMemory(&ddsd,sizeof(ddsd));		
	// 填充主表面描述
	ddsd.dwSize = sizeof(ddsd);		//DirectDraw表面描述结构体大小
	ddsd.dwFlags = DDSD_CAPS;		//设定DDSURFACEDESC2结构中的ddsCaps有效
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;//主表面
	
	//1. 创建主表面
	if (lpDD->CreateSurface( &ddsd, //被填充了表面信息的DDSURFACEDESC2结构的地址
				    &lpDDSPrimary,	//接收主表面指针
				    NULL) != DD_OK)	//NULL, 保留
	{
		return FALSE;
	}

	//创裁减器
	if( lpDD->CreateClipper( 0, 		//现在不用，必须设为0
				    &lpClipper,	//指向剪裁器对象的指针
				     NULL ) != DD_OK )//NULL
		return FALSE;
	//裁减器与显示窗口联系
    if( lpClipper->SetHWnd( 0, hwnd ) != DD_OK )
    {
        lpClipper->Release();
        return FALSE;
    }
	//把裁减器加到主表面
    if( lpDDSPrimary->SetClipper( lpClipper ) != DD_OK )
    {
        lpClipper->Release();
        return FALSE;
    }
    // Done with clipper
    lpClipper->Release();

	//2. 创建YUV表面
	ZeroMemory(&ddsd, sizeof(ddsd));	// 清空表面描述体 
	ddsd.dwSize = sizeof(ddsd);
	// 离屏表面
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;//DDSCAPS_OVERLAY | DDSCAPS_OFFSCREENPLAIN;
	// 填充标志
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;//宽、高与像素结构
	ddsd.dwWidth = width;	//离屏显示的宽
	ddsd.dwHeight = height;	//离屏显示的高
	ddsd.ddpfPixelFormat.dwFlags  = DDPF_FOURCC | DDPF_YUV ;		// 填充四个字符及YUV标志
	ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y','V', '1', '2');	// 四个字符为YV12
	ddsd.ddpfPixelFormat.dwYUVBitCount = 8;							// YUV位宽

	// 创建YUV表面
	if (lpDD->CreateSurface(&ddsd, &lpDDSOffscreen, NULL) != DD_OK)//
	{
		return FALSE;
	}

	this->bitmap_width = width;		//图像的宽
	this->bitmap_height = height;	//图像的高

	// 待显示的图像窗口
	rctSour.left = 0;
	rctSour.top = 0;
	rctSour.right = ddsd.dwWidth;
	rctSour.bottom = ddsd.dwHeight;

	return TRUE;
}

void CDirectDraw::ReleaseDirectDraw( void )
{
	if( this->lpClipper ){	// 释放裁减器
		this->lpClipper->Release(); this->lpClipper=NULL;
	}
	if( this->lpDDSOffscreen ){// 释放离屏表面
		this->lpDDSOffscreen->Release(); this->lpDDSOffscreen=NULL;  
	}
	if( this->lpDDSPrimary ) {// 释放主表面
		this->lpDDSPrimary->Release(); this->lpDDSPrimary=NULL;
	}
	if( this->lpDD ){		// 释放DirectDraw对象
		this->lpDD->Release(); this->lpDD=NULL;  
	}
}

BOOL CDirectDraw::DrawDirectDraw(void * buffer)
{
	HRESULT ddRval;

	if( buffer==NULL) 
		return FALSE;
	
	// 获取目标客户区坐标
	GetClientRect(m_hwnd,&rcDest);
	// 把窗口坐标转换为屏幕坐标
	ClientToScreen(m_hwnd, (LPPOINT)&rcDest.left);
	ClientToScreen(m_hwnd, (LPPOINT)&rcDest.right);
	
	// 查询锁定离屏表面
	do {
		ddRval = lpDDSOffscreen->Lock(NULL, // 指向某个RECT的指针，它指定将被锁定的页面区域。如果该参数为 NULL，整个页面将被锁定 
									&ddsd,	// DDSURFACEDESC结构的地址，将被填充页面的相关信息
									DDLOCK_WAIT | DDLOCK_WRITEONLY,//锁定标志
									NULL);	// NULL
	} while(ddRval == DDERR_WASSTILLDRAWING);// 块传送器正忙，继续查询
	
	// 传送完毕
	if(ddRval != DD_OK)
		return 1;
	// 拷贝待显示图像到主表面内存
	CopyToDDraw( (LPBYTE)ddsd.lpSurface , buffer);
	// 解锁离屏表面
	lpDDSOffscreen->Unlock(NULL);
	// 将离屏表面的YUV源图像（rctSour）画到主表面的rcDest目标区
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