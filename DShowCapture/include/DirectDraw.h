// DDraw.h: interface for the CDirectDraw class.
//
//////////////////////////////////////////////////////////////////////


#include <ddraw.h>

class CDirectDraw
{
public:
	CDirectDraw();
	virtual ~CDirectDraw();

	// ��ʼ��DirectDraw
	BOOL InitDirectDraw(HWND hwnd,int width,int height);
	// �ͷ�DirectDraw��Դ
	void ReleaseDirectDraw(void);
	// ��ͼ
	BOOL DrawDirectDraw(void *buffer);
	
protected:
	//��ͼ�񿽱���DirectX����
	void CopyToDDraw(void* destination_buffer,void* source_buffer);

private:
	//DirectX
	DDSURFACEDESC2         ddsd;			//DirectDraw���������ṹ��
	LPDIRECTDRAW7          lpDD;			//DirectDraw����ָ��
	LPDIRECTDRAWSURFACE7    lpDDSPrimary;	//DirectDraw������ָ��
	LPDIRECTDRAWSURFACE7    lpDDSOffscreen;	//DirectDraw��������ָ��
	LPDIRECTDRAWCLIPPER     lpClipper;		//DirectDraw�ü�����

	//ͼ���С
	int                    bitmap_width;
	int                    bitmap_height;
	
	HWND	m_hwnd;
	//��ʾ��Դ��Ŀ������
	CRect rctSour;
	CRect rcDest;
};

