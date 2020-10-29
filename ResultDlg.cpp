// ResultDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ModuleTest.h"
#include "ResultDlg.h"
#include "afxdialogex.h"
#include <locale>
#include "EnumSerial.h"
#include "ttyinfo.h"

#define ID_MY_TIMER		100
#define WM_UART_ACCESS	WM_USER+100
#define PASS			1
#define FAIL			2
#define TIMER_OUT		3
#define OBSOLETE		0xF0

extern TCHAR	g_szModulePath[_MAX_PATH];
char g_UartBuf[32*1024];
char g_UID[5000][32];
char g_result[5000];   
int m_start, m_testno;
int   m_toggle;
CRITICAL_SECTION gStatusCritical;
int g_repeat;
// CResultDlg dialog
int g_bresult;
int g_bbegin;
int count,total_pass, total_fail, total_timerout;
BOOL bResult;
int errorno;
char *pSrc;
CResultDlg *pResdlg;
float total_yield;
int timer_no;
int g_run;

IMPLEMENT_DYNAMIC(CResultDlg, CDialog)

CResultDlg::CResultDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CResultDlg::IDD, pParent)
{
	m_strPASS = _T("0");
	m_strFAIL = _T("0");
	m_strYield = _T("0 ");
}

CResultDlg::~CResultDlg()
{
}

void CResultDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PICTURE_NUVOTON, m_Picture_NUVOTON);
	DDX_Control(pDX, IDC_STATIC_PASSING, m_Static_Passing);
	DDX_Control(pDX, IDC_STATIC_FAILING, m_Static_Failing);
	DDX_Text(pDX, IDC_EDIT_PASS, m_strPASS);
	DDX_Text(pDX, IDC_EDIT_FAIL, m_strFAIL);
	DDX_Text(pDX, IDC_EDIT_YIELD, m_strYield);
	DDX_Control(pDX, IDC_STATIC_ROLLING, m_Static_Rolling);
}


BEGIN_MESSAGE_MAP(CResultDlg, CDialog)
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
	ON_WM_SYSCOMMAND()
	ON_EN_CHANGE(IDC_EDIT_FAIL, &CResultDlg::OnEnChangeEditFail)
	ON_BN_CLICKED(IDOK, &CResultDlg::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT_PASS, &CResultDlg::OnEnChangeEditPass)
//	ON_STN_CLICKED(IDC_STATIC_FAILING, &CResultDlg::OnStnClickedStaticFailing)
	ON_WM_TIMER()
	ON_MESSAGE(WM_UART_ACCESS,UpdateUART)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CResultDlg message handlers


BOOL CResultDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	DWORD dwThreadID = -1;

	m_brush.CreateSolidBrush(RGB(255,255,255));
	m_Bmp_NUVOTON.LoadBitmap(IDB_BITMAP_NUVOTON);
	m_Picture_NUVOTON.GetDlgItem(IDC_PICTURE_NUVOTON);
	m_Bmp_Pass_1.LoadBitmap(IDB_BITMAP_PASS_1);
	m_Bmp_Pass_2.LoadBitmap(IDB_BITMAP_PASS_2);
	m_Bmp_Pass_3.LoadBitmap(IDB_BITMAP_PASS_3);
	m_Bmp_Pass_4.LoadBitmap(IDB_BITMAP_PASS_4);
	m_Static_Passing.GetDlgItem(IDC_STATIC_PASSING);
	m_Bmp_Fail_1.LoadBitmap(IDB_BITMAP_FAIL_1);
	m_Bmp_Fail_2.LoadBitmap(IDB_BITMAP_FAIL_2);
	m_Bmp_Fail_3.LoadBitmap(IDB_BITMAP_FAIL_3);
	m_Bmp_Fail_4.LoadBitmap(IDB_BITMAP_FAIL_4);
	m_Bmp_Fail_5.LoadBitmap(IDB_BITMAP_FAIL_5);
	m_Static_Failing.GetDlgItem(IDC_STATIC_FAILING);
	m_Static_Rolling.GetDlgItem(IDC_STATIC_ROLLING);
	m_Bmp_Roll_0.LoadBitmap(IDB_BITMAP_ROLL_0);
	m_Bmp_Roll_1.LoadBitmap(IDB_BITMAP_ROLL_1);
	m_Bmp_Roll_2.LoadBitmap(IDB_BITMAP_ROLL_2);
	m_Bmp_Black.LoadBitmap(IDB_BITMAP_BLACK);

	CFont* font, newFont;
	font=GetDlgItem(IDC_STATIC_TESTINFO)->GetFont();
	LOGFONT lf;
	font->GetLogFont(&lf);//获取LOGFONT结构体
	lf.lfHeight= 14; //-20;    //修改字体大小
	lf.lfWeight=100;   //修改字体的粗细
	newFont.CreateFontIndirect(&lf);//创建一个新的字体
	GetDlgItem(IDC_STATIC_TESTINFO)->SetFont(&newFont);
	GetDlgItem(IDC_STATIC_VERSION)->SetFont(&newFont);
#ifdef _UNICODE
	GetDlgItem(IDOK)->SetWindowText(_T("結束"));
	GetDlgItem(IDC_STATIC_TESTINFO)->SetWindowText(_T("測試狀態"));
	GetDlgItem(IDC_STATIC_VERSION)->SetWindowText(_T("Ver:1.00"));
	GetDlgItem(IDC_STATIC_YIELD)->SetWindowText(_T("良率  "));
#endif
	m_hYellowBrush.CreateSolidBrush(RGB(255,255,0));
	m_hWhiteBrush.CreateSolidBrush(RGB(255,255,255));
	m_hFont = CreateFont(20,20,0,0,FW_NORMAL,FALSE,FALSE,FALSE,OEM_CHARSET,OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN ,TEXT("Courier"));

	SetTimer(ID_MY_TIMER, 500, NULL);
	m_toggle = 0;
	m_start = N_STOP;
	m_testno = 0;
	g_repeat = 0;

	GetCOMPort();

	InitializeCriticalSection(&gStatusCritical);

	InitTTYInfo();
	ghThreadExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ghThreadExitEvent == NULL)
		AfxMessageBox(_T("CreateEvent (Thread exit event)"));   
	SetupCommPort(m_wSerialList);
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


HBRUSH CResultDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
//	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	HBRUSH hbr;

	// TODO:  Change any attributes of the DC here
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_TESTINFO) 
	{
		pDC->SetTextColor(RGB(0,0,0)); 
		pDC->SetBkColor(RGB(247,150,70)); // 背景變紅色
		hbr=CreateSolidBrush(RGB(247,150,70));
		return hbr;
	}
	else if (pWnd->GetDlgCtrlID() == IDC_STATIC_VERSION) 
	{
		pDC->SetTextColor(RGB(255,255,255)); 
		pDC->SetBkColor(RGB(79,129,189)); // 背景變紅色
		hbr=CreateSolidBrush(RGB(79,129,189));
		return hbr;
	}
	// TODO:  Return a different brush if the default is not desired
//	return hbr;
	return m_brush;
}


void CResultDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your messacge handler code here
	// Do not call CDialog::OnPaint() for painting messages

	m_Picture_NUVOTON.SetBitmap(m_Bmp_NUVOTON);

}


void CResultDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnSysCommand(nID, lParam);
}


void CResultDlg::OnEnChangeEditFail()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CResultDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CDialog::OnOK();
	SaveTestResultFile();

	BreakDownCommPort();
	KillTimer(ID_MY_TIMER);
	if ( ghThreadExitEvent != NULL )
	{
		CloseHandle(ghThreadExitEvent);
		ghThreadExitEvent = NULL;
	}
}


void CResultDlg::OnEnChangeEditPass()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}





void CResultDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if ( nIDEvent == ID_MY_TIMER )
	{
		switch (m_start)
		{
			case N_STOP:
				PostMessage(WM_UART_ACCESS,0,N_STOP);;
				break;
			case N_RUN:
				if ( m_toggle % 2 == 0 )
					PostMessage(WM_UART_ACCESS,0,N_RUN);
				else
					PostMessage(WM_UART_ACCESS,1,N_RUN);
				m_toggle++;
				break;
			case N_OK:
				PostMessage(WM_UART_ACCESS,total_pass,N_OK);
				break;
			case N_FAIL:
				PostMessage(WM_UART_ACCESS,total_fail,N_FAIL);
				break;
			case N_TIMEOUT:
				PostMessage(WM_UART_ACCESS,total_fail,N_TIMEOUT);
				break;

		}
	}
    EnterCriticalSection(&gStatusCritical);
	if ( m_start == N_RUN )
	{

		timer_no++;
		if ( timer_no >= 40 )  // no response for 20 seconds
		{
			ShowUART(1);
		}
		else
			ShowUART(0);
	}
	LeaveCriticalSection(&gStatusCritical);
	CDialog::OnTimer(nIDEvent);
}

void CResultDlg::SaveTestResultFile()
{	
	CString strTestFile;
    CString Path = g_szModulePath;
	CStdioFile fileIni;
	CStdioFile FinalfileIni;
	CString strFinalFile;
	CString strTemp;
	int i;
	char buf[80];
	TCHAR wbuf[80];
	int pass=0, fail=0, final_pass=0, final_fail=0;
	float yield=0, final_yield;
	int timerout=0, final_timerout=0;
	CString strYield;

	char* old_locale = _strdup( setlocale(LC_CTYPE,NULL) );
 
	setlocale( LC_CTYPE, "cht" );//设定

	strTestFile = Path + m_strTestFile;
	if(!fileIni.Open(strTestFile, CFile::modeCreate | CFile::modeWrite | CFile::typeText ))
		return;

	strFinalFile = Path + m_strFinalFile;
	if ( !FinalfileIni.Open(strFinalFile, CFile::modeCreate | CFile::modeWrite | CFile:: typeText))
		return;


	fileIni.WriteString((CString)"日期 " + m_strDate  + (CString)"\r\n");
	fileIni.WriteString((CString)"PRD " + m_strPRD  + (CString)"\r\n");
	fileIni.WriteString((CString)"PART No " + m_strPARTNO  + (CString)"\r\n");
	fileIni.WriteString((CString)"LOT No " + m_strLOTNO  + (CString)"\r\n");
	fileIni.WriteString((CString)"作業員工號 " + m_strEmployee  + (CString)"\r\n");
	fileIni.WriteString((CString)"流程卡號 " + m_strProcessNo  + (CString)"\r\n");
	
	FinalfileIni.WriteString((CString)"日期 " + m_strDate  + (CString)"\r\n");
	FinalfileIni.WriteString((CString)"PRD " + m_strPRD  + (CString)"\r\n");
	FinalfileIni.WriteString((CString)"PART No " + m_strPARTNO  + (CString)"\r\n");
	FinalfileIni.WriteString((CString)"LOT No " + m_strLOTNO  + (CString)"\r\n");
	FinalfileIni.WriteString((CString)"作業員工號 " + m_strEmployee  + (CString)"\r\n");
	FinalfileIni.WriteString((CString)"流程卡號 " + m_strProcessNo  + (CString)"\r\n");
	fileIni.WriteString(_T("===========================================================\r\n"));
	FinalfileIni.WriteString(_T("===========================================================\r\n"));
	for (i=0; i<m_testno; i++)
	{
		if ( (g_result[i] & 0x0F ) == PASS )
		{
			pass++;
			sprintf_s(buf,sizeof(buf), "%s	PASS\r\n",g_UID[i]);
		}
		else 
		{
			if ( g_result[i] == TIMER_OUT )
			{
				timerout++;
				sprintf_s(buf, sizeof(buf),"%s	TIMER OUT %d\r\n",g_UID[i],timerout);
			}
			else 
				sprintf_s(buf, sizeof(buf),"%s	FAIL\r\n",g_UID[i]);
			fail++;
		}
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)buf, 50, (LPWSTR) wbuf, 50);
		fileIni.WriteString(wbuf);
		if ( g_repeat > 0 )
		{
			if ( (g_result[i] &  OBSOLETE ) == 0 )
			{
				if ( g_result[i] == PASS )
				{
					sprintf_s(buf,sizeof(buf), "%s	PASS\r\n",g_UID[i]);
					final_pass++;
				}
				else 
				{
					if ( g_result[i] == TIMER_OUT )
					{
						final_timerout++;
						sprintf_s(buf, sizeof(buf),"%s	TIMER OUT %d\r\n",g_UID[i], final_timerout);
					}
					else
						sprintf_s(buf, sizeof(buf),"%s	FAIL\r\n",g_UID[i]);
					final_fail++;
				}
				MultiByteToWideChar(CP_ACP, 0, (LPCSTR)buf, 50, (LPWSTR) wbuf, 50);
				FinalfileIni.WriteString(wbuf);
			}
		}
		else
		{
			if ( g_result[i] == PASS )
			{
				sprintf_s(buf,sizeof(buf), "%s	PASS\r\n",g_UID[i]);
				final_pass++;
			}
			else 
			{
				if ( g_result[i] == TIMER_OUT )
				{
					final_timerout++;
					sprintf_s(buf, sizeof(buf),"%s	TIMER OUT\r\n",g_UID[i], final_timerout);
				}
				else
					sprintf_s(buf, sizeof(buf),"%s	FAIL\r\n",g_UID[i]);
				final_fail++;
			}
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)buf, 50, (LPWSTR) wbuf, 50);
			FinalfileIni.WriteString(wbuf);
		}
	}

	fileIni.WriteString(_T("===========================================================\r\n"));
	FinalfileIni.WriteString(_T("===========================================================\r\n"));

	sprintf_s(buf,sizeof(buf), "Pass No. = %4d,	Fail No. = %4d\r\n", pass, fail);
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)buf, 80, (LPWSTR) wbuf, 80);
	fileIni.WriteString(wbuf);
	yield = pass*100/(pass+fail);
	strYield.Format(_T("%5.2f "),yield);
	fileIni.WriteString((CString)"良率 = " + strYield  + (CString)"%\r\n");

	sprintf_s(buf,sizeof(buf), "Pass No. = %4d,	Fail No. = %4d\r\n", final_pass, final_fail);
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)buf, 80, (LPWSTR) wbuf, 80);
	FinalfileIni.WriteString(wbuf);
	final_yield = final_pass*100/(final_pass+final_fail);
	strYield.Format(_T("%5.2f "),final_yield);
	FinalfileIni.WriteString((CString)"良率 = " + strYield  + (CString)"%\r\n");

	FinalfileIni.Close();
	fileIni.Close();
	setlocale( LC_CTYPE, old_locale );
 
	free( old_locale );
}

LRESULT CResultDlg::UpdateUART( WPARAM  par1, LPARAM par2)
{
    EnterCriticalSection(&gStatusCritical);
	if ( par2 == N_STOP )
	{
		if ( g_bbegin == 0 )
		{
			m_Static_Passing.SetBitmap(m_Bmp_Black);
			m_Static_Failing.SetBitmap(m_Bmp_Black);
			m_Static_Rolling.SetBitmap(m_Bmp_Roll_0);
			g_bbegin = 1;
		}
	}
	else
	{
		if ( par2 == N_RUN )
		{
			if ( g_bbegin == 1 )
			{
				m_Static_Passing.SetBitmap(m_Bmp_Black);
				m_Static_Failing.SetBitmap(m_Bmp_Black);
				g_bbegin = 0;
			}
			if ( par1 == 0 )
			{
				m_Static_Rolling.SetBitmap(m_Bmp_Roll_1);
			}
			else
			{
				m_Static_Rolling.SetBitmap(m_Bmp_Roll_2);
			}
		}
		else if ( par2 == N_OK )
		{
			if ( g_bresult == 0 )
			{
				m_strPASS.Format(_T("%d"), par1);
				m_strYield.Format(_T("%5.2f  "),total_yield);
				UpdateData(FALSE);
				m_Static_Passing.SetBitmap(m_Bmp_Pass_3);
				m_Static_Rolling.SetBitmap(m_Bmp_Roll_0);
			}
			g_bresult = 1;
			g_bbegin= 1;
			g_run = 0;
		}
		else if (  par2 == N_FAIL )
		{ 
			if ( g_bresult == 0 )
			{
				m_strFAIL.Format(_T("%d"), par1);
				m_strYield.Format(_T("%5.2f  "),total_yield);
				UpdateData(FALSE);
				m_Static_Failing.SetBitmap(m_Bmp_Fail_3);
				m_Static_Rolling.SetBitmap(m_Bmp_Roll_0);
			}
			g_bresult = 1;
			g_bbegin= 1;
			g_run = 0;
		}
		else if (  par2 == N_TIMEOUT )
		{ 
			if ( g_bresult == 0 )
			{
				m_strFAIL.Format(_T("%d"), par1);
				m_strYield.Format(_T("%5.2f  "),total_yield);
				UpdateData(FALSE);
				m_Static_Failing.SetBitmap(m_Bmp_Fail_5);
				m_Static_Rolling.SetBitmap(m_Bmp_Roll_0);
			}
			g_bresult = 1;
			g_bbegin= 1;
			g_run = 0;
		}
	}
	LeaveCriticalSection(&gStatusCritical);
	return 0;
}

BOOL CResultDlg::GetCOMPort()   
{
#if 1
 int i = 0; 
 TCHAR Name[25]; 
 UCHAR szPortName[25]; 
 LONG Status; 
 DWORD dwIndex = 0; 
 DWORD dwName; 
 DWORD dwSizeofPortName; 
 DWORD Type;
 HKEY hKey; 

 LPCTSTR data_Set= _T("HARDWARE\\DEVICEMAP\\SERIALCOMM\\");

 //long ret0 = (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, data_Set, 0, KEY_READ, &hKey)); 
 long ret0 = RegOpenKeyEx(HKEY_LOCAL_MACHINE, data_Set, 0, KEY_READ, &hKey); //打?一?制定的注?表?,成功返回ERROR_SUCCESS即“0”值
 m_SerialNo = 0;
 if (ret0 == ERROR_SUCCESS) 
 {

//  do 
	for (i=0; i<10; i++)
  { 
	dwName = sizeof(Name); 
    dwSizeofPortName = sizeof(szPortName);
 //  Status = RegEnumValue(hKey, dwIndex++, Name, &dwName, NULL, &Type, szPortName, &dwSizeofPortName);
   Status = RegEnumValue(hKey, i, Name, &dwName, NULL, &Type, szPortName, &dwSizeofPortName);
   if((Status == ERROR_SUCCESS)||(Status == ERROR_MORE_DATA)) 
   { 
//	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szPortName, 50, (LPWSTR) swPortName, 50);
	if ( wcsstr((TCHAR *)szPortName, (TCHAR *)_T("COM1")) )
	{
	//	AfxMessageBox(_T("COM1"));
		continue;

	}

	if ( wcsstr((TCHAR *)szPortName, (TCHAR *)_T("COM2")))
	{
	//	AfxMessageBox(_T("COM2"));
		continue;

	}
    wcscpy_s(m_wSerialList,50 ,(TCHAR *)szPortName) ;     
//	AfxMessageBox((LPCTSTR)m_wSerialList);
 //   printf("serial:%s\//n",strSerialList[i]); 
 //   i++;
	m_SerialNo++;
	break;
   } 
//   dwName = sizeof(Name); 
//   dwSizeofPortName = sizeof(szPortName); 
//  } while((Status == ERROR_SUCCESS)||(Status == ERROR_MORE_DATA)); 
	}
 
  RegCloseKey(hKey); 
 }
#else

	int m_nSerialPortNum;// 串口计数 
	CString str;
    CString          strSerialList[256];  // 临时定义 256 个字符串组 
     CArray<SSerInfo,SSerInfo&> asi; 
     EnumSerialPorts(asi,TRUE);
     m_nSerialPortNum = asi.GetSize(); 
       for (int i=0; i<asi.GetSize(); i++) 
       { 
              str = asi[i].strFriendlyName; 
           printf("serialinfo:%s\n",str);
       }
#endif
	return true;
}


BOOL CResultDlg::InitTTYInfo()
{
    //
    // initialize generial TTY info
    //
    COMDEV( TTYInfo )        = NULL ;
    CONNECTED( TTYInfo )     = FALSE ;
    LOCALECHO( TTYInfo )     = FALSE ;
    CURSORSTATE( TTYInfo )   = CS_HIDE ;
    PORT( TTYInfo )          = 1 ;
    BAUDRATE( TTYInfo )      = 115200 ;
    BYTESIZE( TTYInfo )      = 8 ;
    PARITY( TTYInfo )        = NOPARITY ;
    STOPBITS( TTYInfo )      = ONESTOPBIT ;
    AUTOWRAP( TTYInfo )      = TRUE;
    NEWLINE( TTYInfo )       = FALSE;
    XSIZE( TTYInfo )         = 0 ;
    YSIZE( TTYInfo )         = 0 ;
    XSCROLL( TTYInfo )       = 0 ;
    YSCROLL( TTYInfo )       = 0 ;
    COLUMN( TTYInfo )        = 0 ;
    ROW( TTYInfo )           = MAXROWS - 1 ;
    DISPLAYERRORS( TTYInfo ) = TRUE ;

    //
    // timeouts
    //
    TIMEOUTSNEW( TTYInfo )   = gTimeoutsDefault;

    //
    // read state and status events
    //
    gdwReceiveState            = RECEIVE_TTY;
    EVENTFLAGS( TTYInfo )    = EVENTFLAGS_DEFAULT;
    FLAGCHAR( TTYInfo )      = FLAGCHAR_DEFAULT;

    //
    // Flow Control Settings
    //
    DTRCONTROL( TTYInfo )    = DTR_CONTROL_ENABLE;
    RTSCONTROL( TTYInfo )    = RTS_CONTROL_ENABLE;
    XONCHAR( TTYInfo )       = ASCII_XON;
    XOFFCHAR( TTYInfo )      = ASCII_XOFF;
    XONLIMIT( TTYInfo )      = 0;
    XOFFLIMIT( TTYInfo )     = 0;
    CTSOUTFLOW( TTYInfo )    = FALSE;
    DSROUTFLOW( TTYInfo )    = FALSE;
    DSRINFLOW( TTYInfo )     = FALSE;
    XONXOFFOUTFLOW(TTYInfo)  = FALSE;
    XONXOFFINFLOW(TTYInfo)   = FALSE;
    TXAFTERXOFFSENT(TTYInfo) = FALSE;

    NOREADING(TTYInfo)       = FALSE;
    NOWRITING(TTYInfo)       = FALSE;
    NOEVENTS(TTYInfo)        = FALSE;
    NOSTATUS(TTYInfo)        = FALSE;
    SHOWTIMEOUTS(TTYInfo)    = FALSE;

    //
    // setup default font information
    // 
    LFTTYFONT( TTYInfo ).lfHeight =         12 ;
    LFTTYFONT( TTYInfo ).lfWidth =          0 ;
    LFTTYFONT( TTYInfo ).lfEscapement =     0 ;
    LFTTYFONT( TTYInfo ).lfOrientation =    0 ;
    LFTTYFONT( TTYInfo ).lfWeight =         0 ;
    LFTTYFONT( TTYInfo ).lfItalic =         0 ;
    LFTTYFONT( TTYInfo ).lfUnderline =      0 ;
    LFTTYFONT( TTYInfo ).lfStrikeOut =      0 ;
    LFTTYFONT( TTYInfo ).lfCharSet =        OEM_CHARSET ;
    LFTTYFONT( TTYInfo ).lfOutPrecision =   OUT_DEFAULT_PRECIS ;
    LFTTYFONT( TTYInfo ).lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
    LFTTYFONT( TTYInfo ).lfQuality =        DEFAULT_QUALITY ;
    LFTTYFONT( TTYInfo ).lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
    _tcscpy_s( LFTTYFONT( TTYInfo ).lfFaceName, 32, _T("FixedSys")) ;
    return ( TRUE ) ;
}

/*-----------------------------------------------------------------------------

FUNCTION: DestroyTTYInfo

PURPOSE: Frees objects associated with the TTYInfo structure

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it
            2/14/96   AllenD      Removed npTTYInfo

-----------------------------------------------------------------------------*/
void CResultDlg::DestroyTTYInfo()
{
    DeleteObject(HTTYFONT(TTYInfo));
}

/*-----------------------------------------------------------------------------

FUNCTION: StartThreads

PURPOSE: Creates the Reader/Status and Writer threads

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it

-----------------------------------------------------------------------------*/
void CResultDlg::StartThreads(void)
{
    DWORD dwReadStatId;
    DWORD dwWriterId;


	READSTATTHREAD(TTYInfo) =
            CreateThread( NULL, 
                          0,
                          (LPTHREAD_START_ROUTINE) ReaderAndStatusProc,
                          (LPVOID) this, 
                          0, 
                          &dwReadStatId);

    if (READSTATTHREAD(TTYInfo) == NULL)
        AfxMessageBox(_T("CreateThread(Reader/Status)"));

    return;
}

/*-----------------------------------------------------------------------------

FUNCTION: SetupCommPort( void )

PURPOSE: Setup Communication Port with our settings

RETURN: 
    Handle of comm port is successful
    NULL is error occurs

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it

-----------------------------------------------------------------------------*/
HANDLE CResultDlg::SetupCommPort(CString strComNum)
{
   
    //
    // open communication port handle
    //
    COMDEV( TTYInfo ) = CreateFile( strComNum.GetBuffer(strComNum.GetLength()),  
                                      GENERIC_READ | GENERIC_WRITE, 
                                      0, 
                                      0, 
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                      0);

    if (COMDEV(TTYInfo) == INVALID_HANDLE_VALUE) {   
        AfxMessageBox(_T("Create File fails "));
        return NULL;
    }

    //
    // Save original comm timeouts and set new ones
    //
    if (!GetCommTimeouts( COMDEV(TTYInfo), &(TIMEOUTSORIG(TTYInfo))))
        AfxMessageBox(_T("GetCommTimeouts"));

    //
    // Set port state
    //
    UpdateConnection();

    //
    // set comm buffer sizes
    //
    SetupComm(COMDEV(TTYInfo), MAX_READ_BUFFER, MAX_WRITE_BUFFER);

    //
    // raise DTR
    //
    if (!EscapeCommFunction(COMDEV(TTYInfo), SETDTR))
        AfxMessageBox(_T("EscapeCommFunction (SETDTR)"));

    //
    // start threads and set initial thread state to not done
    //
    StartThreads();

    //
    // set overall connect flag
    //
    CONNECTED( TTYInfo ) = TRUE ;

    return COMDEV(TTYInfo);
}

/*-----------------------------------------------------------------------------

FUNCTION: WaitForThreads(DWORD)

PURPOSE: Waits a specified time for the worker threads to exit

PARAMETERS:
    dwTimeout - milliseconds to wait until timeout

RETURN:
    WAIT_OBJECT_0 - successful wait, threads are not running
    WAIT_TIMEOUT  - at least one thread is still running
    WAIT_FAILED   - failure in WaitForMultipleObjects

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it

----------------------------------------------------------------------------*/
DWORD CResultDlg::WaitForThreads(DWORD dwTimeout)
{	
    HANDLE hThreads[2];
    DWORD  dwRes;

    hThreads[0] = READSTATTHREAD(TTYInfo);

    //
    // set thread exit event here
    //
    SetEvent(ghThreadExitEvent);

    dwRes = WaitForMultipleObjects(1, hThreads, TRUE, dwTimeout);
    switch(dwRes)
    {
        case WAIT_OBJECT_0:
        case WAIT_OBJECT_0 + 1: 
            dwRes = WAIT_OBJECT_0;
            break;

        case WAIT_TIMEOUT:
            
            if (WaitForSingleObject(READSTATTHREAD(TTYInfo), 0) == WAIT_TIMEOUT)
                AfxMessageBox(_T("Reader/Status Thread didn't exit.\n\r"));
            break;

        default:
            AfxMessageBox(_T("WaitForMultipleObjects"));
            break;
    }

    //
    // reset thread exit event here
    //
    ResetEvent(ghThreadExitEvent);

    return dwRes;
}

/*-----------------------------------------------------------------------------

FUNCTION: BreakDownCommPort

PURPOSE: Closes a connection to a comm port

RETURN:
    TRUE  - successful breakdown of port
    FALSE - port isn't connected

COMMENTS: Waits for threads to exit,
          clears DTR, restores comm port timeouts, purges any i/o
          and closes all pertinent handles

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it

-----------------------------------------------------------------------------*/
BOOL CResultDlg::BreakDownCommPort()
{
    if (!CONNECTED(TTYInfo))
        return FALSE;

    CONNECTED( TTYInfo ) = FALSE;

    //
    // wait for the threads for a small period
    //
    if (WaitForThreads(20000) != WAIT_OBJECT_0)
        /*
            if threads haven't exited, then they will
            interfere with a new connection.  I must abort
            the entire program.
        */
        AfxMessageBox(_T("Error closing port."));

    //
    // lower DTR
    //
    if (!EscapeCommFunction(COMDEV(TTYInfo), CLRDTR))
        AfxMessageBox(_T("EscapeCommFunction(CLRDTR)"));

    //
    // restore original comm timeouts
    //
    if (!SetCommTimeouts(COMDEV(TTYInfo),  &(TIMEOUTSORIG(TTYInfo))))
        AfxMessageBox(_T("SetCommTimeouts (Restoration to original)"));

    //
    // Purge reads/writes, input buffer and output buffer
    //
    if (!PurgeComm(COMDEV(TTYInfo), PURGE_FLAGS))
        AfxMessageBox(_T("PurgeComm"));
	if ( COMDEV(TTYInfo) != INVALID_HANDLE_VALUE )
	{
		CloseHandle(COMDEV(TTYInfo));
		COMDEV(TTYInfo) = INVALID_HANDLE_VALUE;
	}
	if ( READSTATTHREAD(TTYInfo) != INVALID_HANDLE_VALUE )
	{
		CloseHandle(READSTATTHREAD(TTYInfo));
		READSTATTHREAD(TTYInfo) = INVALID_HANDLE_VALUE;
	}


    return TRUE;
}
 
/*-----------------------------------------------------------------------------

FUNCTION: CheckModemStatus(BOOL)

PURPOSE: Check new status and possibly report it

PARAMETERS:
    bUpdateNow - if TRUE, update should be done
                 if FALSE, update is done only if status is different

COMMENTS: Reports status if bUpdateNow is true or
          new status is different from old status

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it

-----------------------------------------------------------------------------*/
void CResultDlg::CheckModemStatus( BOOL bUpdateNow )
{
    //
    // dwOldStatus needs to be static so that it is maintained 
    // between function calls by the same thread.
    // It also needs to be __declspec(thread) so that it is 
    // initialized when a new thread is created.
    //

    __declspec(thread) static DWORD dwOldStatus = 0;

    DWORD dwNewModemStatus;

    if (!GetCommModemStatus(COMDEV(TTYInfo), &dwNewModemStatus))
        AfxMessageBox(_T("GetCommModemStatus"));

    //
    // Report status if bUpdateNow is true or status has changed
    //
    if (bUpdateNow || (dwNewModemStatus != dwOldStatus)) {
        AfxMessageBox(dwNewModemStatus);
        dwOldStatus = dwNewModemStatus;
    }

    return;
}


/*-----------------------------------------------------------------------------

FUNCTION: CheckComStat(BOOL)

PURPOSE: Calls ClearCommError and reports the results.

PARAMETERS:
    bUpdateNow - If TRUE, then ReportComStat is called with new results.
                 if FALSE, then ReportComStat is called only if
                 new results differ from old.

COMMENTS: Called when the ReaderAndStatusProc times out after waiting on
          its event handles.  Also called from ReportStatusEvent.

HISTORY:   Date:      Author:     Comment:
           12/18/95   AllenD      Wrote it

-----------------------------------------------------------------------------*/
void CResultDlg::CheckComStat(BOOL bUpdateNow)
{
    COMSTAT ComStatNew;
    DWORD dwErrors;

    __declspec(thread) static COMSTAT ComStatOld = {0};
    __declspec(thread) static DWORD dwErrorsOld = 0;

    BOOL bReport = bUpdateNow;

    if (!ClearCommError(COMDEV(TTYInfo), &dwErrors, &ComStatNew))
        AfxMessageBox(_T("ClearCommError"));

    if (dwErrors != dwErrorsOld) {
        bReport = TRUE;
        dwErrorsOld = dwErrors;
    }

    if (memcmp(&ComStatOld, &ComStatNew, sizeof(COMSTAT))) {
        bReport = TRUE;
        ComStatOld = ComStatNew;
    }
    
//   if (bReport)
//        ReportComStat(ComStatNew);

    ComStatOld = ComStatNew;

    return;
}

/*-----------------------------------------------------------------------------

FUNCTION: ReaderAndStatusProc(LPVOID)

PURPOSE: Thread function controls comm port reading, comm port status 
         checking, and status messages.

PARMATERS:
    lpV - 4 byte value contains the tty child window handle

RETURN: always 1

COMMENTS: Waits on various events in the applications to handle
          port reading, status checking, status messages and modem status.

HISTORY:   Date:      Author:     Comment:
           10/27/95   AllenD      Wrote it
           11/21/95   AllenD      Incorporated status message heap

-----------------------------------------------------------------------------*/
DWORD __stdcall CResultDlg::ReaderAndStatusProc(LPVOID lpV)
{

    OVERLAPPED osReader = {0};  // overlapped structure for read operations
    OVERLAPPED osStatus = {0};  // overlapped structure for status operations
    HANDLE     hArray[NUM_READSTAT_HANDLES];
    DWORD      dwStoredFlags = 0xFFFFFFFF;      // local copy of event flags
    DWORD      dwCommEvent;     // result from WaitCommEvent
    DWORD      dwOvRes;         // result from GetOverlappedResult
    DWORD 	   dwRead;          // bytes actually read
    DWORD      dwRes;           // result from WaitForSingleObject
    BOOL       fWaitingOnRead = FALSE;
    BOOL       fWaitingOnStat = FALSE;
    BOOL       fThreadDone = FALSE;
    char   	   lpBuf[AMOUNT_TO_READ];



    //
    // create two overlapped structures, one for read events
    // and another for status events
    //
	CResultDlg *pdlg = (CResultDlg *) lpV;
	pResdlg = pdlg;

    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osReader.hEvent == NULL)
        AfxMessageBox(_T("CreateEvent (Reader Event)"));

    osStatus.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osStatus.hEvent == NULL)
        AfxMessageBox(_T("CreateEvent (Status Event)"));

    //
    // We want to detect the following events:
    //   Read events (from ReadFile)
    //   Status events (from WaitCommEvent)
    //   Status message events (from our UpdateStatus)
    //   Thread exit evetns (from our shutdown functions)
    //
    hArray[0] = osReader.hEvent;
    hArray[1] = osStatus.hEvent;
    hArray[2] = ghThreadExitEvent;

    //
    // initial check, forces updates
    //
//    CheckModemStatus(TRUE);
//    CheckComStat(TRUE);
	total_pass=0;
	total_fail = 0;
	total_timerout = 0;
	pSrc = g_UartBuf;
	memset(g_UartBuf, 0,32*1024);
	count= 0;
	bResult = TRUE;
	g_bresult = 0;
	g_bbegin = 0;
	g_run = 0;
    while ( !fThreadDone ) {

        //
        // If no reading is allowed, then set flag to
        // make it look like a read is already outstanding.
        //
        if (NOREADING( TTYInfo ))
            fWaitingOnRead = TRUE;
        
        //
        // if no read is outstanding, then issue another one
        //
        if (!fWaitingOnRead) {
            if (!ReadFile(COMDEV(TTYInfo), lpBuf, AMOUNT_TO_READ, &dwRead, &osReader)) {
                if (GetLastError() != ERROR_IO_PENDING)	  // read not delayed?
                    AfxMessageBox(_T("ReadFile in ReaderAndStatusProc"));

                fWaitingOnRead = TRUE;
            }
            else {    // read completed immediately
                if ((dwRead != MAX_READ_BUFFER) && SHOWTIMEOUTS(TTYInfo))
                    AfxMessageBox(_T("Read timed out immediately.\r\n"));
				        if (dwRead)
						{
							memcpy(pSrc, lpBuf, dwRead);
							pSrc += dwRead;
							count += dwRead;
//							if ( count <= AMOUNT_TO_READ )
							if ( g_run == 0 )
							{
								if ( strstr(g_UartBuf,"#Start Test%") != NULL )
								{
									g_bresult = 0;
									g_bbegin = 1;
									g_run = 1;

									if ( strstr(g_UartBuf,"mismatch") != NULL )
									{
										bResult = FALSE;
#if 0
										if (strstr(g_UartBuf,"MAP0 - 0x0") != NULL )
											errorno = 920;
										else
											errorno = 540; //630;
#endif
									}
									else

									{
										bResult = TRUE;
									}
									m_start = N_RUN;
									timer_no = 0;
									//pdlg->PostMessage(WM_UART_ACCESS,0,1);
									//m_toggle++;
								}
								else
								{
									pSrc = g_UartBuf;
									memset(g_UartBuf, 0,32*1024);
									count= 0;
								}
							}

						}
            }
        }

        //
        // If status flags have changed, then reset comm mask.
        // This will cause a pending WaitCommEvent to complete
        // and the resultant event flag will be NULL.
        //
        if (dwStoredFlags != EVENTFLAGS(TTYInfo)) {
            dwStoredFlags = EVENTFLAGS(TTYInfo);
            if (!SetCommMask(COMDEV(TTYInfo), dwStoredFlags))
                AfxMessageBox(_T("SetCommMask"));
        }

        //
        // If event checks are not allowed, then make it look
        // like an event check operation is outstanding
        //
        if (NOEVENTS(TTYInfo))
            fWaitingOnStat = TRUE;
        //
        // if no status check is outstanding, then issue another one
        //
        if (!fWaitingOnStat) {
            if (NOEVENTS(TTYInfo))
                fWaitingOnStat = TRUE;
            else {
                if (!WaitCommEvent(COMDEV(TTYInfo), &dwCommEvent, &osStatus)) {
                    if (GetLastError() != ERROR_IO_PENDING)	  // Wait not delayed?
                        AfxMessageBox(_T("WaitCommEvent"));
                    else
                        fWaitingOnStat = TRUE;
                }
                //else
                    // WaitCommEvent returned immediately
                //    ReportStatusEvent(dwCommEvent);
            }
        }

        //
        // wait for pending operations to complete
        //
        if ( fWaitingOnStat && fWaitingOnRead ) {
            dwRes = WaitForMultipleObjects(NUM_READSTAT_HANDLES, hArray, FALSE, STATUS_CHECK_TIMEOUT);
            switch(dwRes)
            {
                //
                // read completed
                //
                case WAIT_OBJECT_0:
                    if (!GetOverlappedResult(COMDEV(TTYInfo), &osReader, &dwRead, FALSE)) {
                        if (GetLastError() == ERROR_OPERATION_ABORTED)
                            AfxMessageBox(_T("Read aborted\r\n"));
                        else
                            AfxMessageBox(_T("GetOverlappedResult (in Reader)"));
                    }
                    else {      // read completed successfully
                        if ((dwRead != MAX_READ_BUFFER) && SHOWTIMEOUTS(TTYInfo))
                            AfxMessageBox(_T("Read timed out overlapped.\r\n"));
                        if (dwRead)
						{
							memcpy(pSrc, lpBuf, dwRead);
							pSrc += dwRead;
							count += dwRead;

//							if ( count <= AMOUNT_TO_READ )
							if ( g_run == 0 )
							{
								if ( strstr(g_UartBuf,"#Start Test%") != NULL )
								{
									g_bresult = 0;
									g_bbegin = 1;
									g_run = 1;
									if ( strstr(g_UartBuf,"mismatch") != NULL )
									{
										bResult = FALSE;
#if 0
										if (strstr(g_UartBuf,"MAP0 - 0x0") != NULL )
											errorno = 920;
										else
											errorno = 540; //630;
#endif
									}
									else
									{
										bResult = TRUE;
									}
									m_start = N_RUN;
									timer_no = 0;
									//pdlg->PostMessage(WM_UART_ACCESS,0,1);
									//m_toggle++;
								}
								else
								{
									pSrc = g_UartBuf;
									memset(g_UartBuf, 0,32*1024);
									count= 0;
								}
							}


						}
 //                           OutputABuffer(hTTY, lpBuf, dwRead);
                    }

                    fWaitingOnRead = FALSE;
                    break;

                //
                // status completed
                //
                case WAIT_OBJECT_0 + 1: 
                    if (!GetOverlappedResult(COMDEV(TTYInfo), &osStatus, &dwOvRes, FALSE)) {
                        if (GetLastError() == ERROR_OPERATION_ABORTED)
                            AfxMessageBox(_T("WaitCommEvent aborted\r\n"));
                        else
                            AfxMessageBox(_T("GetOverlappedResult (in Reader)"));
                    }
                    //else       // status check completed successfully
                    //    ReportStatusEvent(dwCommEvent);

                    fWaitingOnStat = FALSE;
                    break;

                //
                // status message event
                //
              //  case WAIT_OBJECT_0 + 2:
                    //StatusMessage();
                    break;

                //
                // thread exit event
                //
                case WAIT_OBJECT_0 + 2:
                    fThreadDone = TRUE;
                    break;

                case WAIT_TIMEOUT:
                    //
                    // timeouts are not reported because they happen too often
                    // OutputDebugString("Timeout in Reader & Status checking\n\r");
                    //

                    //
                    // if status checks are not allowed, then don't issue the
                    // modem status check nor the com stat check
                    //
                    if (!NOSTATUS(TTYInfo)) {
//                        CheckModemStatus(FALSE);    // take this opportunity to do
//                        CheckComStat(FALSE);        //   a modem status check and
                                                    //   a comm status check
                    }

                    break;                       

                default:
                    AfxMessageBox(_T("WaitForMultipleObjects(Reader & Status handles)"));
                    break;
            }
        }
    }

    //
    // close event handles
    //
    CloseHandle(osReader.hEvent);
    CloseHandle(osStatus.hEvent);

    return 1;

}

BOOL CResultDlg::UpdateConnection()
{
    DCB dcb = {0};
    
    dcb.DCBlength = sizeof(dcb);

    //
    // get current DCB settings
    //
    if (!GetCommState(COMDEV(TTYInfo), &dcb))
	AfxMessageBox(_T("GetCommState"));

    //
    // update DCB rate, byte size, parity, and stop bits size
    //
    dcb.BaudRate = BAUDRATE(TTYInfo);
    dcb.ByteSize = BYTESIZE(TTYInfo);
    dcb.Parity   = PARITY(TTYInfo);
    dcb.StopBits = STOPBITS(TTYInfo);

    //
    // update event flags
    //
    if (EVENTFLAGS(TTYInfo) & EV_RXFLAG)
	dcb.EvtChar = FLAGCHAR(TTYInfo);      
    else
	dcb.EvtChar = '\0';

    //
    // update flow control settings
    //
    dcb.fDtrControl     = DTRCONTROL(TTYInfo);
    dcb.fRtsControl     = RTSCONTROL(TTYInfo);

    dcb.fOutxCtsFlow    = CTSOUTFLOW(TTYInfo);
    dcb.fOutxDsrFlow    = DSROUTFLOW(TTYInfo);
    dcb.fDsrSensitivity = DSRINFLOW(TTYInfo);
    dcb.fOutX           = XONXOFFOUTFLOW(TTYInfo);
    dcb.fInX            = XONXOFFINFLOW(TTYInfo);
    dcb.fTXContinueOnXoff = TXAFTERXOFFSENT(TTYInfo);
    dcb.XonChar         = XONCHAR(TTYInfo);
    dcb.XoffChar        = XOFFCHAR(TTYInfo);
    dcb.XonLim          = XONLIMIT(TTYInfo);
    dcb.XoffLim         = XOFFLIMIT(TTYInfo);

    //
    // DCB settings not in the user's control
    //
    dcb.fParity = TRUE;

    //
    // set new state
    //
    if (!SetCommState(COMDEV(TTYInfo), &dcb))
	AfxMessageBox(_T("SetCommState"));

    //
    // set new timeouts
    //
    if (!SetCommTimeouts(COMDEV(TTYInfo), &(TIMEOUTSNEW(TTYInfo))))
	AfxMessageBox(_T("SetCommTimeouts"));

    return TRUE;
}



void CResultDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: Add your message handler code here

	DeleteCriticalSection(&gStatusCritical);
}


void CResultDlg::ShowUART(int val)
{
	char *pDes, *pDes1;
	int no, i;
	int all;

	if ( val ==  1 )
	{
		for (i=0; i< no; i++)
		{
			g_UID[m_testno][i] = 0;
		}	
		g_result[m_testno] = TIMER_OUT;
		total_fail++;
		total_timerout++;
		m_start = N_TIMEOUT;  //Fail, time out
		total_yield = (float)total_pass*100/(total_pass+total_fail);
		//pResdlg->PostMessage(WM_UART_ACCESS,total_fail,3);
		pSrc = g_UartBuf;
		memset(g_UartBuf, 0,32*1024);
		count= 0;
		m_testno++;
	}
	if (bResult == TRUE )
	{
		{
			pDes = strstr(g_UartBuf, "#UID ");
			if ( pDes == NULL )
				return;
			pDes = strstr(g_UartBuf, "#UID ");
			if ( pDes != NULL )
			{
				pDes1 = strstr(pDes, "%");
				if ( pDes1 == NULL )
					return;
				pDes += 5;
				no = pDes1-5 - pDes;
				for (i=0; i< no; i++)
				{
					g_UID[m_testno][i] = *pDes++;
				}	
				g_UID[m_testno][i] = 0;
				for (i= m_testno-1; i>= 0; i--)
				{
					if ( strcmp(g_UID[m_testno], g_UID[i] ) == 0 )
					{
						g_repeat++;
						if ( g_result[i] == PASS )
							total_pass--;
						else if ( g_result[i] == FAIL )
							total_fail--;
						g_result[i] |= OBSOLETE;
						break;
					}
				}
				if ( strstr(pDes, "Pass") )
				{
					g_result[m_testno] = PASS;
					total_pass ++;
					m_start = N_OK;  // Pass
					total_yield = (float)total_pass*100/(total_pass+total_fail);
	//				pResdlg->PostMessage(WM_UART_ACCESS,total_pass,2);
				}
				else
				{
					g_result[m_testno] = FAIL;
					total_fail++;
					m_start = N_FAIL;  //Fail
					total_yield = (float)total_pass*100/(total_pass+total_fail);
//					all = total_pass+total_fail;
		//			pResdlg->PostMessage(WM_UART_ACCESS,total_fail,3);

				}
				pSrc = g_UartBuf;
				memset(g_UartBuf, 0,32*1024);
				count= 0;
				m_testno++;
			}
		}
	}
	else
	{
		{
			pDes = strstr(g_UartBuf, "pressed ...");
			if ( pDes == NULL )
				return;
			pDes = strstr(g_UartBuf, "#UID ");
			if ( pDes != NULL )
			{
				pDes1 = strstr(pDes, "%");
				if ( pDes1 == NULL )
					return;
				pDes += 5;
				no = pDes1-5 - pDes;
				for (i=0; i< no; i++)
				{
					g_UID[m_testno][i] = *pDes++;
				}	
				g_UID[m_testno][i] = 0;
				for (i= m_testno-1; i>= 0; i--)
				{
					if ( strcmp(g_UID[m_testno], g_UID[i] ) == 0 )
					{
						g_repeat++;
						if ( g_result[i] == PASS )
							total_pass--;
						else if ( g_result[i] == FAIL )
							total_fail--;
						g_result[i] |= OBSOLETE;
							break;
					}
				}
				if ( strstr(pDes, "Pass") )
				{
					g_result[m_testno] = PASS;
					total_pass ++;
					m_start = N_OK;  // Pass
					total_yield = (float)total_pass*100/(total_pass+total_fail);
//					all = total_pass+total_fail;
//					pResdlg->PostMessage(WM_UART_ACCESS,total_pass,2);
				}
				else
				{
					g_result[m_testno] = FAIL;
					total_fail++;
					m_start = N_FAIL;  //Fail
					total_yield = (float)total_pass*100/(total_pass+total_fail);
//					all = total_pass+total_fail;
//					pResdlg->PostMessage(WM_UART_ACCESS,total_fail,3);
				}
				pSrc = g_UartBuf;
				memset(g_UartBuf, 0,32*1024);
				count= 0;
				m_testno++;
			}
		}
	}
}