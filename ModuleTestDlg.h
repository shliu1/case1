
// ModuleTestDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CModuleTestDlg dialog
class CModuleTestDlg : public CDialog
{
// Construction
public:
	CModuleTestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MODULETEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	CBrush m_brush;
	CStatic m_Picture_NUVOTON;
	CBitmap m_Bmp_NUVOTON;
	CString	m_strDate, m_strPRD, m_strPARTNO, m_strLOTNO, m_strEmployee, m_strProcessNo;
	CString m_strTestFile;
	CString m_strFinalFile;

	int m_testflag;
	int m_nSelectTestItem;
	SYSTEMTIME systime;

	// Generated message map functions
	void InitComboForTestItem();
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnStnClickedStaticDate3();
	afx_msg void OnEnChangeEditDate();
	afx_msg void OnEnChangeEditPrd();
	afx_msg void OnEnChangeEditPartno();
	afx_msg void OnEnChangeEditLotno();
	afx_msg void OnEnChangeEditEmployee();
	afx_msg void OnBnClickedButtonTest();
	afx_msg void OnEnChangeEditProcessno();
	afx_msg void OnCbnSelchangeComboTestitem();
	CComboBox m_comboTestItem;
//	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
