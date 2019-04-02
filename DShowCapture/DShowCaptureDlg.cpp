
// DShowCaptureDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "DShowCapture.h"
#include "DShowCaptureDlg.h"
#include "afxdialogex.h"
//#include "DirectDraw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDShowCaptureDlg 对话框



CDShowCaptureDlg::CDShowCaptureDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDShowCaptureDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bInit = FALSE;
	m_bIsVideoOpen = FALSE;
	m_bIsRecord = FALSE;
	m_bIsPush = FALSE;
	m_bIsPull = FALSE;
}

void CDShowCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_VIDEODEVICES, m_cbxVideoDevList);
	DDX_Control(pDX, IDC_COMBO_AUDEODEVICES, m_cbxAudioDevList);
	DDX_Control(pDX, IDC_COMBO_VIDEORESOLUTION, m_cbxVideoResList);
}

BEGIN_MESSAGE_MAP(CDShowCaptureDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_PUSHSTREAM, &CDShowCaptureDlg::OnBnClickedButtonPushstream)
	ON_BN_CLICKED(IDC_BUTTON_CAPTURE, &CDShowCaptureDlg::OnBnClickedButtonCapture)
	ON_BN_CLICKED(IDC_BUTTON_GETDEVICES, &CDShowCaptureDlg::OnBnClickedButtonGetdevices)
	ON_BN_CLICKED(IDC_BUTTON_CAMERASET, &CDShowCaptureDlg::OnBnClickedButtonCameraset)
	ON_BN_CLICKED(IDC_BUTTON_INIT, &CDShowCaptureDlg::OnBnClickedButtonInit)

	ON_BN_CLICKED(IDC_BUTTON_PULLSTREAM, &CDShowCaptureDlg::OnBnClickedButtonPullstream)
END_MESSAGE_MAP()


// CDShowCaptureDlg 消息处理程序

BOOL CDShowCaptureDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		MessageBox(_T("网络初始化失败！"), _T("提示"));
	}

	m_pVideoCapture =new CVideoCapture();

	m_pRtpSender = new RTPSender();
	m_pRtpReceiver = new RTPReceiver();

	frame_queue_init(&m_stVDecQueue);
	frame_queue_start(&m_stVDecQueue);

	frame_queue_init(&m_stADecQueue);
	frame_queue_start(&m_stADecQueue);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDShowCaptureDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CDShowCaptureDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CDShowCaptureDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CDShowCaptureDlg::OnBnClickedButtonRecord()
{
	// TODO:  在此添加控件通知处理程序代码
/*	if (!m_bInit)
	{
		MessageBox(_T("请先初始化！"), _T("提示"));
		return;
	}

	if (!m_bIsVideoOpen)
	{
		MessageBox(_T("请打开采集！"), _T("提示"));
		return;
	}

	if(m_bIsRecord == FALSE)
	{
		m_bIsRecord = TRUE;
		CTime time = CTime::GetCurrentTime();	
		CString szTime = time.Format("%Y%m%d_%H%M%S");
		CString filename =  _T("");
		CString filepath = _T("D:\\Record");

		filename.Format(_T("%s\\%s.mp4"), filepath, szTime);
		ENCODE_PARAMS rParams;
		rParams.stVidParams.nWidth = m_nWidth;
		rParams.stVidParams.nHeight = m_nHeight;
		rParams.stVidParams.nBitRate = 1024 * 1000;
		rParams.stVidParams.nFrameRate = 30;
		rParams.stAudParams.nBitRate = 64000;
		rParams.stAudParams.nSampleRate = 96000;
		USES_CONVERSION;
		int nRet = m_pMp4Record->InitRecord(T2A(filename), rParams);

		nRet = m_pMp4Record->StartRecord(&m_stVideoQueue, &m_stAudioQueue);
		SetDlgItemText(IDC_BUTTON_RECORD, _T("停止"));

	}
	else
	{
		m_bIsRecord = FALSE;
		int nRet = m_pMp4Record->StopRecord();
		SetDlgItemText(IDC_BUTTON_RECORD, _T("录像"));
	}
	*/
}


void CDShowCaptureDlg::OnBnClickedButtonCapture()
{
	// TODO:  在此添加控件通知处理程序代码
	if (!m_bInit)
	{
		MessageBox(_T("请先初始化！"), _T("提示"));
		return;
	}

	if (m_bIsVideoOpen == TRUE)
	{
		m_pVideoCapture->StopCapture();
		SetDlgItemText(IDC_BUTTON_CAPTURE, _T("采集"));
		m_bIsVideoOpen = FALSE;
		return;
	}

	//选择一个分辨率
	int nSel = m_cbxVideoResList.GetCurSel();
	if (nSel < 0)
	{
		MessageBox(_T("请选择一个分辨率"), _T("提示"));
		return;
	}
	CString strResolution = _T("");
	m_cbxVideoResList.GetLBText(nSel, strResolution);
	if (strResolution.IsEmpty())
	{
		MessageBox(_T("请选择一个有值分辨率"), _T("提示"));
		return;
	}

	int nSetWidth = -1;
	int nSetHeight = -1;
	int nFindValue = strResolution.Find(_T("*"));
	if (nFindValue > 0)
	{
		nSetWidth = _ttoi(strResolution.Left(nFindValue));

		CString strTempValue = strResolution.Mid(nFindValue + 1);
		nFindValue = strTempValue.Find(_T(","));

		nSetHeight = _ttoi(strTempValue.Left(nFindValue));
	}
	int nResolutionIndex = 0; //默认为0，即当前默认分辨率
	for (int i = 0; i < m_arrCamResolutionArr.GetSize(); i++)
	{
		CamResolutionInfo camInfo = m_arrCamResolutionArr.GetAt(i);
	//	TRACE("nWidth:%d, nHeight:%d, lSampleRate:%ld\n", camInfo.nWidth, camInfo.nHeight, camInfo.lSampleRate);
		if (camInfo.nWidth = nSetWidth && camInfo.nHeight == nSetHeight)
		{
			TRACE("selected lSampleRate: %d\n", camInfo.lSampleRate);
			m_nSampleRate = camInfo.lSampleRate;
			nResolutionIndex = camInfo.nResolutionIndex;
			break;
		}
	}
	m_nWidth = nSetWidth;
	m_nHeight = nSetHeight;
	
	HWND h_wnd = GetDlgItem(IDC_STATIC_PREVIEW)->m_hWnd;


	m_pVideoCapture->StartCapture(nResolutionIndex, h_wnd, m_nWidth, m_nHeight);

	SetDlgItemText(IDC_BUTTON_CAPTURE, _T("停止"));
	m_bIsVideoOpen = TRUE;
//	GetDlgItem(IDC_BUTTON_CAPTURE)->SetDlgItemText(_T("停止"));
}


void CDShowCaptureDlg::OnBnClickedButtonGetdevices()
{
	// TODO:  在此添加控件通知处理程序代码
	ASImgDeviceInfoArray VidDevInfo;
	m_pVideoCapture->ListVideoCaptureDevices(VidDevInfo);

	for (int i = 0; i < VidDevInfo.GetSize(); i++)
	{
		ImgDeviceInfo sImgDevInfo = VidDevInfo.GetAt(i);
		m_cbxVideoDevList.AddString(sImgDevInfo.strDeviceName);
	}

	if (m_cbxVideoDevList.GetCount() > 0)
	{
		m_cbxVideoDevList.SetCurSel(0);
	}

	ASImgDeviceInfoArray AudDevInfo;
	m_pVideoCapture->ListAudioCaptureDevices(AudDevInfo);

	for (int i = 0; i < AudDevInfo.GetSize(); i++)
	{
		ImgDeviceInfo sImgDevInfo = AudDevInfo.GetAt(i);
		m_cbxAudioDevList.AddString(sImgDevInfo.strDeviceName);
	}

	if (m_cbxAudioDevList.GetCount() > 0)
	{
		m_cbxAudioDevList.SetCurSel(0);
	}

}




void CDShowCaptureDlg::OnBnClickedButtonCameraset()
{
	// TODO:  在此添加控件通知处理程序代码
}


void CDShowCaptureDlg::OnBnClickedButtonInit()
{
	// TODO:  在此添加控件通知处理程序代码
	CString strVidDevName, strAudDevName;

	if (m_bInit)
	{
		return;
	}

	int nVideoIndex = m_cbxVideoDevList.GetCurSel();
	if (nVideoIndex < 0)
	{
		MessageBox(_T("请选择视频设备"), _T("提示"));
		return;
	}

	int nAudioIndex = m_cbxAudioDevList.GetCurSel();
	if (nAudioIndex < 0)
	{
		MessageBox(_T("请选择音频设备"), _T("提示"));
		return;
	}

	m_cbxVideoDevList.GetLBText(nVideoIndex, strVidDevName);
	m_cbxAudioDevList.GetLBText(nAudioIndex, strAudDevName);

	if (strVidDevName.IsEmpty())
	{
		MessageBox(_T("获取视频设备名称为空"), _T("提示"));
		return;
	}

	if (strAudDevName.IsEmpty())
	{
		MessageBox(_T("获取音频设备名称为空"), _T("提示"));
		return;
	}

	frame_queue_init(&m_stVideoQueue);
	frame_queue_start(&m_stVideoQueue);

	frame_queue_init(&m_stAudioQueue);
	frame_queue_start(&m_stAudioQueue);

	m_pVideoCapture->InitCapture(strVidDevName, strAudDevName, &m_stVideoQueue, &m_stAudioQueue);

	ASCamResolutionInfoArray VidResolution;
	m_pVideoCapture->GetVideoResolution(VidResolution);
	for (int i = 0; i < VidResolution.GetSize(); i++)
	{
		CamResolutionInfo sVidResInfo = VidResolution.GetAt(i);
		CString strFormat = _T("");
		strFormat.Format(_T("%d * %d , %s, 采样率:%ld"), sVidResInfo.nWidth, sVidResInfo.nHeight, sVidResInfo.strSubType, sVidResInfo.lSampleRate);
		m_cbxVideoResList.AddString(strFormat);
	}

	m_arrCamResolutionArr.Copy(VidResolution);
	if (m_cbxVideoResList.GetCount() > 0)
	{
		m_cbxVideoResList.SetCurSel(0);
	}

	m_bInit = TRUE;
}


void CDShowCaptureDlg::OnBnClickedButtonPushstream()
{
	// TODO:  在此添加控件通知处理程序代码
	if (!m_bInit)
	{
		MessageBox(_T("请先初始化！"), _T("提示"));
		return;
	}

	if (!m_bIsVideoOpen)
	{
		MessageBox(_T("请打开采集！"), _T("提示"));
		return;
	}

	if (m_bIsPush == FALSE)
	{
		ENCODE_PARAMS rParams;
		rParams.stVidParams.nWidth = m_nWidth;
		rParams.stVidParams.nHeight = m_nHeight;
		rParams.stVidParams.nBitRate = 1024 * 1000;
		rParams.stVidParams.nFrameRate = 25;
		rParams.stAudParams.nBitRate = 64000;
		rParams.stAudParams.nSampleRate = 96000;

		SetDlgItemText(IDC_BUTTON_PUSHSTREAM, _T("停止"));
		//int nRet = m_pRtspServer->Start(rParams, &m_stVideoQueue, &m_stAudioQueue);
		int nRet = m_pRtpSender->StartRTPSender(rParams, &m_stVideoQueue, &m_stAudioQueue);
		m_bIsPush = TRUE;
	}
	else
	{
		SetDlgItemText(IDC_BUTTON_PUSHSTREAM, _T("推流"));
		m_bIsPush = FALSE;
	}
}

void CDShowCaptureDlg::OnBnClickedButtonPullstream()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_bIsPull == FALSE)
	{
		ENCODE_PARAMS rParams;
		rParams.stVidParams.nWidth = m_nWidth;
		rParams.stVidParams.nHeight = m_nHeight;
		rParams.stVidParams.nFrameRate = 25;
		CDirectDraw *m_pDDraw = new CDirectDraw();
		HWND h_wnd = GetDlgItem(IDC_STATIC_REMOTE)->m_hWnd;

		m_pDDraw->InitDirectDraw(h_wnd, m_nWidth, m_nHeight);
		int nRet = m_pRtpReceiver->StartRTPReceiver(m_pDDraw, rParams, &m_stVDecQueue, &m_stADecQueue);
		SetDlgItemText(IDC_BUTTON_PULLSTREAM, _T("停止"));
		m_bIsPull = TRUE;
	}
	else
	{
		SetDlgItemText(IDC_BUTTON_PULLSTREAM, _T("拉流"));
		m_bIsPull = FALSE;
	}

	//	
}
