#pragma once
#include "CUartIO.h"
#include "afxwin.h"

// CResultDlg dialog

#define	N_STOP				0
#define	N_RUN				1
#define N_OK				2
#define N_FAIL				3
#define N_TIMEOUT			4

class CResultDlg : public CDialog
{
	DECLARE_DYNAMIC(CResultDlg)

public:
	CResultDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CResultDlg();

// Dialog Data
	enum { IDD = IDD_MODULETEST_RESULT_DIALOG };
	CString m_strTestFile;
	CString m_strFinalFile;
	CString m_strDate, m_strPRD, m_strPARTNO, m_strLOTNO, m_strEmployee, m_strProcessNo;
	TCHAR   m_wSerialList[256]; 
	int  m_SerialNo;
	CUartIO     m_comIO;

	void SaveTestResultFile();
	LRESULT UpdateUART( WPARAM  pos, LPARAM message);
	void DownloadProcess();
	BOOL GetCOMPort();
	void ShowUART(int val);

// TTY
	BOOL InitTTYInfo();
	void DestroyTTYInfo();
	void StartThreads(void);
	HANDLE SetupCommPort(CString strComNum);
	DWORD WaitForThreads(DWORD dwTimeout);
	BOOL BreakDownCommPort();
	void CheckModemStatus( BOOL bUpdateNow );
	void CheckComStat(BOOL bUpdateNow);
	static DWORD __stdcall ReaderAndStatusProc(LPVOID lpV);
	BOOL UpdateConnection();

protected:
	CBrush m_brush;
	CStatic m_Picture_NUVOTON;
	CBitmap m_Bmp_NUVOTON;
	CStatic m_Static_Passing;
	CBitmap m_Bmp_Pass_1,m_Bmp_Pass_2;
	CBitmap m_Bmp_Pass_3,m_Bmp_Pass_4;
	CStatic m_Static_Failing;
	CBitmap m_Bmp_Fail_1,m_Bmp_Fail_2;
	CBitmap m_Bmp_Fail_3,m_Bmp_Fail_4;
	CBitmap m_Bmp_Fail_5;
	CStatic m_Static_Rolling;
	CBitmap m_Bmp_Roll_0, m_Bmp_Roll_1;
	CBitmap m_Bmp_Roll_2, m_Bmp_Black;
	CString	m_strPASS, m_strFAIL, m_strYield;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    CBrush m_hYellowBrush, m_hWhiteBrush;
	HFONT m_hFont;




	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnPaint();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnEnChangeEditFail();
	afx_msg void OnBnClickedOk();
	afx_msg void OnEnChangeEditPass();
//	afx_msg void OnStnClickedStaticFailing();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();

};


