// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda
 
		revision history:
		rev		date	comments
		00		23jul25	initial version
 
*/

// MidiRoll.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "MidiRoll.h"
#include "MainFrm.h"

#include "MidiRollDoc.h"
#include "MidiRollView.h"
#include "AboutDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Win32Console.h"

// CMidiRollApp

BEGIN_MESSAGE_MAP(CMidiRollApp, CWinAppCK)
	ON_COMMAND(ID_APP_ABOUT, &CMidiRollApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppCK::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppCK::OnFileOpen)
END_MESSAGE_MAP()


// CMidiRollApp construction

CMidiRollApp::CMidiRollApp()
{
	m_bDeferShowOnFirstWindowPlacementLoad = true;	// eliminates startup flicker
	m_bHiColorIcons = TRUE;

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("MidiRoll.AppID.NoVersion"));

	m_pDoc = NULL;
	// Place all significant initialization in InitInstance
}

// The one and only CMidiRollApp object

CMidiRollApp theApp;


// CMidiRollApp initialization

BOOL CMidiRollApp::InitInstance()
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

	CWinAppCK::InitInstance();
#ifdef _DEBUG
	Win32Console::Create();
#endif

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Anal Software"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)


	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CMidiRollDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CMidiRollView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The stock code shows and updates the main window here, but this makes the
	// view flicker due to being painted twice; it's solved by moving show/update
	// to CMainFrame::OnDelayedCreate which runs after the window sizes stabilize
	return TRUE;
}

int CMidiRollApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	AfxOleTerm(FALSE);

	return CWinAppCK::ExitInstance();
}

// CMidiRollApp message handlers

void CMidiRollApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CMidiRollApp customization load/save methods

void CMidiRollApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void CMidiRollApp::LoadCustomState()
{
}

void CMidiRollApp::SaveCustomState()
{
}

LRESULT	CMidiRollApp::OnTrackingHelp(WPARAM wParam, LPARAM lParam)
{
	if (GetMainFrame()->IsTracking())
		return GetMainFrame()->SendMessage(WM_COMMANDHELP, wParam, lParam);
	return FALSE;
}

// CMidiRollApp message handlers



