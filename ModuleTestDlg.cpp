
// ModuleTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ModuleTest.h"
#include "ModuleTestDlg.h"
//#include "afxdialogex.h"
#include "ResultDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CModuleTestDlg dialog




CModuleTestDlg::CModuleTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModuleTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strDate = _T("");
	m_strPRD = _T("");
	m_strPARTNO = _T("");
	m_strLOTNO = _T("");
	m_strEmployee = _T("");
	m_strProcessNo = _T("");
}

void CModuleTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PICTURE_NUVOTON, m_Picture_NUVOTON);
	DDX_Text(pDX, IDC_EDIT_DATE, m_strDate);
	DDX_Text(pDX, IDC_EDIT_PRD, m_strPRD);
	DDX_Text(pDX, IDC_EDIT_PARTNO, m_strPARTNO);
	DDX_Text(pDX, IDC_EDIT_LOTNO, m_strLOTNO);
	DDX_Text(pDX, IDC_EDIT_EMPLOYEE, m_strEmployee);
	DDX_Text(pDX, IDC_EDIT_PROCESSNO, m_strProcessNo);
	DDX_Control(pDX, IDC_COMBO_TESTITEM, m_comboTestItem);
}

BEGIN_MESSAGE_MAP(CModuleTestDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CTLCOLOR()
	ON_STN_CLICKED(IDC_STATIC_DATE3, &CModuleTestDlg::OnStnClickedStaticDate3)
	ON_EN_CHANGE(IDC_EDIT_DATE, &CModuleTestDlg::OnEnChangeEditDate)
	ON_EN_CHANGE(IDC_EDIT_PRD, &CModuleTestDlg::OnEnChangeEditPrd)
	ON_EN_CHANGE(IDC_EDIT_PARTNO, &CModuleTestDlg::OnEnChangeEditPartno)
	ON_EN_CHANGE(IDC_EDIT_LOTNO, &CModuleTestDlg::OnEnChangeEditLotno)
	ON_EN_CHANGE(IDC_EDIT_EMPLOYEE, &CModuleTestDlg::OnEnChangeEditEmployee)
	ON_BN_CLICKED(IDC_BUTTON_TEST, &CModuleTestDlg::OnBnClickedButtonTest)
	ON_EN_CHANGE(IDC_EDIT_PROCESSNO, &CModuleTestDlg::OnEnChangeEditProcessno)
	ON_CBN_SELCHANGE(IDC_COMBO_TESTITEM, &CModuleTestDlg::OnCbnSelchangeComboTestitem)
END_MESSAGE_MAP()


// CModuleTestDlg message handlers

BOOL CModuleTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.


	// IDM_ABOUTBOX must be in the system command range.
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	GetLocalTime(&systime);//本地时间

	m_strDate.Format(_T("%02d:%02d %02d-%02d-%04d"),systime.wHour,systime.wMinute, systime.wMonth, systime.wDay, systime.wYear);

	m_brush.CreateSolidBrush(RGB(255,255,255));

	m_Bmp_NUVOTON.LoadBitmap(IDB_BITMAP_NUVOTON);
	m_Picture_NUVOTON.GetDlgItem(IDC_PICTURE_NUVOTON);


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
	GetDlgItem(IDC_STATIC_DATE)->SetWindowText(_T("日期"));
	GetDlgItem(IDC_STATIC_EMPLOYEE)->SetWindowText(_T("作業員工號"));
	GetDlgItem(IDC_BUTTON_TEST)->SetWindowTextW(_T("開始測試"));
	GetDlgItem(IDC_STATIC_PROCESSNO)->SetWindowText(_T("流程卡號"));
	GetDlgItem(IDC_STATIC_TESTITEM)->SetWindowText(_T("測試項目"));
	GetDlgItem(IDC_STATIC_TESTINFO)->SetWindowText(_T("測試資訊設定"));
	GetDlgItem(IDC_STATIC_VERSION)->SetWindowText(_T("Ver:1.00"));
#endif
	m_testflag = 0;

	InitComboForTestItem();

	UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CModuleTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CModuleTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		m_Picture_NUVOTON.SetBitmap(m_Bmp_NUVOTON);
		this->GetDlgItem(IDC_EDIT_PRD)->SetFocus();
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CModuleTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



HBRUSH CModuleTestDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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
	return m_brush;
//	return hbr;
}


void CModuleTestDlg::OnStnClickedStaticDate3()
{
	// TODO: Add your control notification handler code here
}


void CModuleTestDlg::OnEnChangeEditDate()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CModuleTestDlg::OnEnChangeEditPrd()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CModuleTestDlg::OnEnChangeEditPartno()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CModuleTestDlg::OnEnChangeEditLotno()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CModuleTestDlg::OnEnChangeEditEmployee()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CModuleTestDlg::OnBnClickedButtonTest()
{
	// TODO: Add your control notification handler code here
	CString strTestFile;
	CString strFinalFile;

	UpdateData(TRUE);
	if ( m_strPRD.IsEmpty())
	{
		AfxMessageBox(_T("請輸入 PRD"));
		return;
	}
	if ( m_strPARTNO.IsEmpty())
	{
		AfxMessageBox(_T("請輸入 PART NO"));
		return;
	}
	if (m_strLOTNO.IsEmpty())
	{
		AfxMessageBox(_T("請輸入 LOT NO"));
		return;
	}
	if ( m_strEmployee.IsEmpty())
	{
		AfxMessageBox(_T("請輸入 作業員工號"));
		return;
	}
	if ( m_strProcessNo.IsEmpty())
	{
		AfxMessageBox(_T("請輸入 流程卡號"));
		return;
	}
	if ( m_testflag == 0 )
	{
#ifdef _UNICODE
		GetDlgItem(IDC_BUTTON_TEST)->SetWindowTextW(_T("結束"));
#endif
		m_testflag = 1;
		CResultDlg dlg;
		dlg.m_strDate = m_strDate;
		dlg.m_strPRD = m_strPRD;
		dlg.m_strPARTNO = m_strPARTNO;
		dlg.m_strLOTNO = m_strLOTNO;
		dlg.m_strEmployee = m_strEmployee;
		dlg.m_strProcessNo = m_strProcessNo;
//
		switch (m_nSelectTestItem)
		{
			case 0 :
				strTestFile.Format(_T("_Test_Correction_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				strFinalFile.Format(_T("_Final_Test_Correction_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				break;
			case 1 :
				strTestFile.Format(_T("_Test_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				strFinalFile.Format(_T("_Final_Test_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				break;
			case 2 :
				strTestFile.Format(_T("_Test__R1_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				strFinalFile.Format(_T("_Final_Test_R1_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				break;
			case 3 :
				strTestFile.Format(_T("_Test_R2_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				strFinalFile.Format(_T("_Final_Test_R2_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				break;
			case 4 :
				strTestFile.Format(_T("_Test__R3_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				strFinalFile.Format(_T("_Final_Test_R3_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				break;
			case 5 :
				strTestFile.Format(_T("_Test_R4_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				strFinalFile.Format(_T("_Final_Test_R4_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				break;
			default :
				strTestFile.Format(_T("_Test_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				strFinalFile.Format(_T("_Final_Test_%04d%02d%02d%02d%02d"),systime.wYear, systime.wMonth, systime.wDay, systime.wHour,systime.wMinute );
				break;
		}
		m_strTestFile = m_strProcessNo + strTestFile;
		m_strFinalFile = m_strProcessNo + strFinalFile;
		dlg.m_strTestFile = m_strTestFile;
		dlg.m_strFinalFile = m_strFinalFile;
		dlg.DoModal();
		
//	}
//	else
//	{
		CDialog::OnOK();
		m_testflag = 0;
	}
}





void CModuleTestDlg::OnEnChangeEditProcessno()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CModuleTestDlg::InitComboForTestItem()
{
	DWORD i;
#ifdef _UNICODE
	TCHAR   GetTargetKeyName[6][50] = {
		_T("Correction 驗證測試"),
		_T("一般測試"),
		_T("重測一"),
		_T("重測二"),
		_T("重測三"),
		_T("重測四")
	};
#endif



    for ( i =0; i < 6; i++)
	{
        m_comboTestItem.AddString(GetTargetKeyName[i]);
    }
    m_nSelectTestItem = 0;
	m_comboTestItem.SetCurSel(m_nSelectTestItem);

  	UpdateData(FALSE);

}

void CModuleTestDlg::OnCbnSelchangeComboTestitem()
{
	// TODO: Add your control notification handler code here
	DWORD dwIndex; 
    UpdateData(TRUE);

    dwIndex = m_comboTestItem.GetCurSel();
    m_nSelectTestItem = (BYTE)dwIndex;
}


//BOOL CModuleTestDlg::OnCommand(WPARAM wParam, LPARAM lParam)
//{
//	// TODO: Add your specialized code here and/or call the base class
//	UpdateData(TRUE);
//	return CDialog::OnCommand(wParam, lParam);
//}


BOOL CModuleTestDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN ))
	{
		if ( pMsg->hwnd == this->GetDlgItem(IDC_EDIT_PRD)->m_hWnd )
		{
			this->GetDlgItem(IDC_EDIT_PARTNO)->SetFocus();
		}
		else if ( pMsg->hwnd == this->GetDlgItem(IDC_EDIT_PARTNO)->m_hWnd )
		{
			this->GetDlgItem(IDC_EDIT_LOTNO)->SetFocus();
		}
		else if ( pMsg->hwnd == this->GetDlgItem(IDC_EDIT_LOTNO)->m_hWnd )
		{
			this->GetDlgItem(IDC_EDIT_EMPLOYEE)->SetFocus();
		}
		else if ( pMsg->hwnd == this->GetDlgItem(IDC_EDIT_EMPLOYEE)->m_hWnd )
		{
			this->GetDlgItem(IDC_EDIT_PROCESSNO)->SetFocus();
		}
		return TRUE;
	}
	return CDialog::PreTranslateMessage(pMsg);
}
