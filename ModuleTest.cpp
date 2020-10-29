
// ModuleTest.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ModuleTest.h"
#include "ModuleTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

TCHAR	g_szModulePath[_MAX_PATH];

// CModuleTestApp

BEGIN_MESSAGE_MAP(CModuleTestApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CModuleTestApp construction

CModuleTestApp::CModuleTestApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CModuleTestApp object

CModuleTestApp theApp;


// CModuleTestApp initialization

BOOL CModuleTestApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	TCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szModulePath[_MAX_PATH];

	::GetModuleFileName(m_hInstance, szModulePath, sizeof(szModulePath));
#ifdef _UNICODE
	::_wsplitpath_s(szModulePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, NULL, 0, NULL, 0);
	::_wmakepath_s(g_szModulePath, _MAX_PATH, szDrive, szDir, NULL, NULL);
#else
	::_splitpath_s(szModulePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, NULL, 0, NULL, 0);
	::_makepath_s(g_szModulePath, _MAX_PATH, szDrive, szDir, NULL, NULL);
#endif

	CModuleTestDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

