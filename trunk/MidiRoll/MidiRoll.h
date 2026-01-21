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

// MidiRoll.h : main header file for the MidiRoll application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "WinAppCK.h"       // main symbols

// CMidiRollApp:
// See MidiRoll.cpp for the implementation of this class
//

class CMainFrame;
class CMidiRollDoc;

class CMidiRollApp : public CWinAppCK
{
public:
	CMidiRollApp();

	CMidiRollDoc	*m_pDoc;

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	CMainFrame	*GetMainFrame();

// Implementation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	LRESULT	OnTrackingHelp(WPARAM wParam, LPARAM lParam);

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CMidiRollApp theApp;

inline CMainFrame *CMidiRollApp::GetMainFrame()
{
	return reinterpret_cast<CMainFrame *>(m_pMainWnd);
}
