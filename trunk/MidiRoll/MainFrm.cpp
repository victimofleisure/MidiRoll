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

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "MidiRoll.h"

#include "MainFrm.h"
#include "MidiRollDoc.h"
#include "MidiRollView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CUR_NOTE,
	ID_INDICATOR_CUR_TEMPO,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

enum {	// application looks; alpha order to match corresponding resource IDs
	APPLOOK_OFF_2003,
	APPLOOK_OFF_2007_AQUA,
	APPLOOK_OFF_2007_BLACK,
	APPLOOK_OFF_2007_BLUE,
	APPLOOK_OFF_2007_SILVER,
	APPLOOK_OFF_XP, 
	APPLOOK_VS_2005,
	APPLOOK_VS_2008,
	APPLOOK_WINDOWS_7,
	APPLOOK_WIN_2000,
	APPLOOK_WIN_XP,
	APP_LOOKS
};

#define ID_VIEW_APPLOOK_FIRST ID_VIEW_APPLOOK_OFF_2003
#define ID_VIEW_APPLOOK_LAST ID_VIEW_APPLOOK_WIN_XP

const UINT CMainFrame::m_arrDockingBarNameID[DOCKING_BARS] = {
	#define MAINDOCKBARDEF(name, width, height, style) IDS_BAR_##name,
	#include "MainDockBarDef.h"	// generate docking bar names
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	theApp.m_pMainWnd = this;
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), APPLOOK_VS_2008);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bNameValid;

	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(theApp.m_bHiColorIcons ? IDR_MAINFRAME_256 : IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));
	m_wndStatusBar.SetPaneText(1, _T(""));
	m_wndStatusBar.SetPaneText(2, _T(""));

	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);
	DockPane(&m_wndToolBar);

	// create docking windows
	if (!CreateDockingWindows())
	{
		TRACE0("Failed to create docking windows\n");
		return -1;
	}

	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);
	// set the visual manager and style based on persisted value
	OnApplicationLook(theApp.m_nAppLook + ID_VIEW_APPLOOK_FIRST);

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	DragAcceptFiles();
	PostMessage(UWM_DELAYED_CREATE);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG

BOOL CMainFrame::CreateDockingWindows()
{
	CString sTitle;
	DWORD	dwBaseStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CBRS_FLOAT_MULTI;
	#define MAINDOCKBARDEF(name, width, height, style) \
		sTitle.LoadString(IDS_BAR_##name); \
		if (!m_wnd##name##Bar.Create(sTitle, this, CRect(0, 0, width, height), TRUE, ID_BAR_##name, style)) {	\
			TRACE("Failed to create %s bar\n", #name);	\
			return FALSE; \
		} \
		m_wnd##name##Bar.EnableDocking(CBRS_ALIGN_ANY); \
		DockPane(&m_wnd##name##Bar);
	#include "MainDockBarDef.h"	// generate code to create docking windows
//	SetDockingWindowIcons(theApp.m_bHiColorIcons);
	return TRUE;
}

void CMainFrame::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
//	printf("CMainFrame::OnUpdate pSender=%Ix lHint=%Id pHint=%Ix\n", pSender, lHint, pHint);
	switch (lHint) {
	case CMidiRollDoc::HINT_NONE:
		PopulateTracksCombo();
		PopulateChannelsCombo();
		break;
	case CMidiRollDoc::HINT_TRACK_SEL:
		UpdateTracksCombo();
		break;
	case CMidiRollDoc::HINT_CHANNEL_SEL:
		UpdateChannelsCombo();
		break;
	}
	// relay update to visible bars
	if (m_wndTracksBar.FastIsVisible())
		m_wndTracksBar.OnUpdate(pSender, lHint, pHint);
	if (m_wndChannelsBar.FastIsVisible())
		m_wndChannelsBar.OnUpdate(pSender, lHint, pHint);
}

CMFCToolBarComboBoxButton* CMainFrame::GetToolbarCombo(int nID)
{
	int iBtn = m_wndToolBar.CommandToIndex(nID);
    if (iBtn < 0)
		return NULL;
    return DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, m_wndToolBar.GetButton(iBtn));
}

bool CMainFrame::PopulateTracksCombo()
{
	CMFCToolBarComboBoxButton* pCombo = GetToolbarCombo(ID_TRACKS_COMBO);
	if (pCombo == NULL)
		return false;
	CMidiRollDoc	*pDoc = theApp.m_pDoc;
	pCombo->RemoveAllItems();
	int	nTracks = pDoc->m_arrTrackName.GetSize();
	if (nTracks) {
		pCombo->AddItem(LDS(IDS_ALL));
		for (int iTrack = 0; iTrack < nTracks; iTrack++) {
			pCombo->AddItem(pDoc->m_arrTrackName[iTrack]);
		}
	}
	UpdateTracksCombo();
	return true;
}

bool CMainFrame::UpdateTracksCombo()
{
	CMFCToolBarComboBoxButton* pCombo = GetToolbarCombo(ID_TRACKS_COMBO);
	if (pCombo == NULL)
		return false;
	CMidiRollDoc	*pDoc = theApp.m_pDoc;
	CIntArrayEx	arrSel;
	pDoc->GetSelectedTracks(arrSel);
	int	iItem = GetComboItem(pCombo, arrSel);
	pCombo->SelectItem(iItem);
	m_wndToolBar.Invalidate();
	return true;
}

bool CMainFrame::PopulateChannelsCombo()
{
	CMFCToolBarComboBoxButton* pCombo = GetToolbarCombo(ID_CHANNELS_COMBO);
	if (pCombo == NULL)
		return false;
	CMidiRollDoc	*pDoc = theApp.m_pDoc;
	CIntArrayEx	arrUsed;
	pDoc->GetUsedChannels(arrUsed);
	pCombo->RemoveAllItems();
	int	nUsedChans = arrUsed.GetSize();
	if (nUsedChans) {
		pCombo->AddItem(LDS(IDS_ALL));
		CString	sChan;
		for (int iUsed = 0; iUsed < nUsedChans; iUsed++) {
			int	iChan = arrUsed[iUsed];
			sChan.Format(_T("%d"), iChan + 1);
			pCombo->AddItem(sChan);
		}
	}
	UpdateChannelsCombo();
	return true;
}

bool CMainFrame::UpdateChannelsCombo()
{
	CMFCToolBarComboBoxButton* pCombo = GetToolbarCombo(ID_CHANNELS_COMBO);
	if (pCombo == NULL)
		return false;
	CMidiRollDoc	*pDoc = theApp.m_pDoc;
	CIntArrayEx	arrSel;
	pDoc->GetSelectedChannels(arrSel);
	int	iItem = GetComboItem(pCombo, arrSel);
	pCombo->SelectItem(iItem);
	m_wndToolBar.Invalidate();
	return true;
}

int CMainFrame::GetComboItem(CMFCToolBarComboBoxButton* pCombo, const CIntArrayEx& arrSel)
{
	int	nSels = arrSel.GetSize();
	int	iItem;
	if (nSels == 1) {	// if single item selected
		iItem = arrSel[0] + 1;	// skip "all" item
	} else if (nSels == pCombo->GetCount() - 1) {	// if all items selected
		iItem = 0;	// select "all" item
	} else {	// nothing selected
		iItem = -1;
	}
	return iItem;
}

// CMainFrame message map

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_FIRST, ID_VIEW_APPLOOK_LAST, OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_FIRST, ID_VIEW_APPLOOK_LAST, OnUpdateApplicationLook)
	ON_MESSAGE(UWM_DELAYED_CREATE, OnDelayedCreate)
	ON_NOTIFY(CRulerCtrl::RN_CLICKED, CMidiRollView::RULER_ID, OnRulerClicked)
	ON_WM_MOUSEWHEEL()
	ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, OnToolbarReset)
	ON_COMMAND(ID_TRACKS_COMBO, OnTracksCombo)
	ON_COMMAND(ID_CHANNELS_COMBO, OnTracksCombo)
	ON_CBN_SELENDOK(ID_TRACKS_COMBO, OnSelTracks)
	ON_CBN_SELENDOK(ID_CHANNELS_COMBO, OnSelChannels)
END_MESSAGE_MAP()

// CMainFrame message handlers

void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
	// base class does the real work

	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}
	bool	bResetCustomizations = true;
	if (bResetCustomizations) {	// if resetting UI to its default state
#if _MFC_VER < 0xb00
		m_wndMenuBar.RestoreOriginalstate();
		m_wndToolBar.RestoreOriginalstate();
#else	// MS fixed typo
		m_wndMenuBar.RestoreOriginalState();
		m_wndToolBar.RestoreOriginalState();
#endif
		theApp.GetKeyboardManager()->ResetAll();
		theApp.GetContextMenuManager()->ResetState();
	}
	return TRUE;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id - ID_VIEW_APPLOOK_FIRST;

	switch (theApp.m_nAppLook)
	{
	case APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	UINT	nAppLook = pCmdUI->m_nID - ID_VIEW_APPLOOK_FIRST;
	pCmdUI->SetRadio(theApp.m_nAppLook == nAppLook);
}

LRESULT	CMainFrame::OnDelayedCreate(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	// The main window has been initialized, so show and update it
	ShowWindow(theApp.m_nCmdShow);
	UpdateWindow();
	return 0;
}

void CMainFrame::OnRulerClicked(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pResult);
	CMidiRollView	*pView = STATIC_DOWNCAST(CMidiRollView, GetActiveView());
	if (pView != NULL) {
		CRulerCtrl::NMRULER	*pNMRuler = static_cast<CRulerCtrl::NMRULER *>(pNMHDR);
		pView->OnRulerClicked(pNMRuler->ptCursor);
	}
}

BOOL CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CMidiRollView	*pView = STATIC_DOWNCAST(CMidiRollView, GetActiveView());
	if (pView != NULL) {
		pView->OnRulerWheel(nFlags, zDelta, pt);
	}
	return CFrameWndEx::OnMouseWheel(nFlags, zDelta, pt);
}

#define MAINDOCKBARDEF(name, width, height, style) \
	void CMainFrame::OnViewBar##name() \
	{ \
		m_wnd##name##Bar.ToggleShowPane(); \
	} \
	void CMainFrame::OnUpdateViewBar##name(CCmdUI *pCmdUI) \
	{ \
		pCmdUI->SetCheck(m_wnd##name##Bar.IsVisible()); \
	}
#include "MainDockBarDef.h"	// generate docking bar message handlers

LRESULT CMainFrame::OnToolbarReset(WPARAM wParam, LPARAM lParam)
{
	UINT uiToolBarId = (UINT) wParam;
	if (uiToolBarId == m_wndToolBar.GetResourceID()) {
		const int	arrComboID[2] = {ID_TRACKS_COMBO, ID_CHANNELS_COMBO};
		const int	arrComboSize[2] = {0, 75};
		for (int iCombo = 0; iCombo < _countof(arrComboID); iCombo++) {
			int	nID = arrComboID[iCombo];
			int	iBtn = m_wndToolBar.CommandToIndex(nID);
			if (iBtn >= 0) {
				CMFCToolBarComboBoxButton comboBtn(nID, -1, CBS_DROPDOWNLIST, arrComboSize[iCombo]);
				m_wndToolBar.ReplaceButton(nID, comboBtn);
			}
		}
    }
	return 0;
}

void CMainFrame::OnSelTracks()
{
	CMFCToolBarComboBoxButton* pCombo = GetToolbarCombo(ID_TRACKS_COMBO);
	if (pCombo != NULL) {
		int	iCurSel = pCombo->GetCurSel();
		if (iCurSel >= 0) {
			CMidiRollDoc	*pDoc = theApp.m_pDoc;
			if (iCurSel) {
				pDoc->SelectTrack(iCurSel - 1);	// account for all item
			} else {
				pDoc->SelectAllTracks();
			}
		}
	}
}

void CMainFrame::OnSelChannels()
{
	CMFCToolBarComboBoxButton* pCombo = GetToolbarCombo(ID_CHANNELS_COMBO);
	if (pCombo != NULL) {
		int	iCurSel = pCombo->GetCurSel();
		if (iCurSel >= 0) {
			CMidiRollDoc	*pDoc = theApp.m_pDoc;
			if (iCurSel) {
				pDoc->SelectChannel(iCurSel - 1);	// account for all item
			} else {
				pDoc->SelectAllChannels();
			}
		}
	}
}

void CMainFrame::OnTracksCombo()
{
}
