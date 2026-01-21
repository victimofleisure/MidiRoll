// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda
 
		revision history:
		rev		date	comments
		00		23jul25	initial version
 		01		20jan26	add channels and tracks panes and combos

*/

// MainFrm.h : interface of the CMainFrame class
//

#pragma once

#include "TracksBar.h"
#include "ChannelsBar.h"

// docking bar IDs are relative to AFX_IDW_CONTROLBAR_FIRST
enum {	// docking bar IDs; don't change, else bar placement won't be restored
	ID_APP_DOCKING_BAR_START = AFX_IDW_CONTROLBAR_FIRST + 40,
	#define MAINDOCKBARDEF(name, width, height, style) ID_BAR_##name,
	#include "MainDockBarDef.h"	// generate docking bar IDs
	ID_APP_DOCKING_BAR_END,
	ID_APP_DOCKING_BAR_FIRST = ID_APP_DOCKING_BAR_START + 1,
	ID_APP_DOCKING_BAR_LAST = ID_APP_DOCKING_BAR_END - 1,
};

class CMainFrame : public CFrameWndEx
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

public:
// Constants
	enum {	// enumerate docking bars
		#define MAINDOCKBARDEF(name, width, height, style) DOCKING_BAR_##name,
		#include "MainDockBarDef.h"	// generate docking bar enumeration
		DOCKING_BARS
	};
	static const UINT m_arrDockingBarNameID[DOCKING_BARS];	// array of docking bar name IDs

// Attributes
public:

// Operations
public:
	void	OnUpdate(CView* pSender, LPARAM lHint = 0, CObject* pHint = NULL);

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:  // control bar embedded members
	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	#define MAINDOCKBARDEF(name, width, height, style) C##name##Bar m_wnd##name##Bar;
	#include "MainDockBarDef.h"	// generate docking bar members

protected:
// Helpers
	BOOL	CreateDockingWindows();
	CMFCToolBarComboBoxButton*	GetToolbarCombo(int nID);
	bool	PopulateTracksCombo();
	bool	UpdateTracksCombo();
	bool	PopulateChannelsCombo();
	bool	UpdateChannelsCombo();
	int		GetComboItem(CMFCToolBarComboBoxButton* pCombo, const CIntArrayEx& arrSel);

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg LRESULT	OnDelayedCreate(WPARAM wParam, LPARAM lParam);
	afx_msg void OnRulerClicked(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	#define MAINDOCKBARDEF(name, width, height, style) \
		afx_msg void OnViewBar##name(); \
		afx_msg void OnUpdateViewBar##name(CCmdUI *pCmdUI);
	#include "MainDockBarDef.h"	// generate docking bar message handlers
	afx_msg void OnTracksCombo();
	afx_msg LRESULT OnToolbarReset(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSelTracks();
	afx_msg void OnSelChannels();
};
