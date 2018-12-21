
// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 从 Windows 头中排除极少使用的资料
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

// 关闭 MFC 对某些常见但经常可放心忽略的警告消息的隐藏
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展





#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // 功能区和控件条的 MFC 支持
#include <stdio.h>  
#include <stdlib.h>  
//#include "inttypes.h"
#include "stdint.h"

#include <fstream>
#include <string>
#include <regex>
using namespace std;

//DShow库加载
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


//要支持YUV420,也就是WMMEDIASUBTYPE_I420必须添加此头文件
#include <wmsdkidl.h>
#include "libyuv.h"
#pragma comment(lib,"libyuv.lib")  





#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


