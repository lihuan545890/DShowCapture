// DDraw.h: interface for the CDirectDraw class.
//
//////////////////////////////////////////////////////////////////////


#include <ddraw.h>

class CDirectDraw
{
public:
	CDirectDraw();
	virtual ~CDirectDraw();

	// 初始化DirectDraw
	BOOL InitDirectDraw(HWND hwnd,int width,int height);
	// 释放DirectDraw资源
	void ReleaseDirectDraw(void);
	// 画图
	BOOL DrawDirectDraw(void *buffer);
	
protected:
	//把图像拷贝到DirectX表面
	void CopyToDDraw(void* destination_buffer,void* source_buffer);

private:
	//DirectX
	DDSURFACEDESC2         ddsd;			//DirectDraw表面描述结构体
	LPDIRECTDRAW7          lpDD;			//DirectDraw对象指针
	LPDIRECTDRAWSURFACE7    lpDDSPrimary;	//DirectDraw主表面指针
	LPDIRECTDRAWSURFACE7    lpDDSOffscreen;	//DirectDraw离屏表面指针
	LPDIRECTDRAWCLIPPER     lpClipper;		//DirectDraw裁剪对象

	//图像大小
	int                    bitmap_width;
	int                    bitmap_height;
	
	HWND	m_hwnd;
	//显示的源与目标区域
	CRect rctSour;
	CRect rcDest;
};

