
// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // �� Windows ͷ���ų�����ʹ�õ�����
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

// �ر� MFC ��ĳЩ�����������ɷ��ĺ��Եľ�����Ϣ������
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC ��������ͱ�׼���
#include <afxext.h>         // MFC ��չ





#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC �� Internet Explorer 4 �����ؼ���֧��
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC �� Windows �����ؼ���֧��
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // �������Ϳؼ����� MFC ֧��
#include <stdio.h>  
#include <stdlib.h>  
//#include "inttypes.h"
#include "stdint.h"

#include <fstream>
#include <string>
#include <regex>
using namespace std;
#define __D3DRM_H__
//DShow�����
#include "Wmcodecdsp.h"
#pragma comment(lib,"wmcodecdspuuid.lib")

#include "Dshow.h"
#include <qedit.h>
#include "Dmodshow.h"
#include "dmo.h"

#ifdef _DEBUG
#pragma comment(lib,"strmbasd.lib")
#else
#pragma comment(lib,"STRMBASE.lib")
#endif
#pragma comment(lib,"strmiids.lib")
#pragma comment(lib,"quartz.lib")
#pragma comment(lib,"dmoguids.lib")   

#pragma comment(lib,"avcodec.lib")  
#pragma comment(lib,"avfilter.lib")  
#pragma comment(lib,"avformat.lib")  
#pragma comment(lib,"avutil.lib")  
#pragma comment(lib,"swscale.lib")  

#pragma comment(lib,"libjrtp.lib")  
//#pragma comment(lib,"jrtplib_d.lib")  
//#pragma comment(lib,"jthread_d.lib")  
//Ҫ֧��YUV420,Ҳ����WMMEDIASUBTYPE_I420������Ӵ�ͷ�ļ�
#include <wmsdkidl.h>
#include "libyuv.h"
#pragma comment(lib,"libyuv.lib")  
#pragma comment(lib, "pthreadVC2.lib")




#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


