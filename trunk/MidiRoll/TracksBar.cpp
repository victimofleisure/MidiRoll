// Copyleft 2026 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		20jan26	initial version
		
*/

#include "stdafx.h"
#include "MidiRoll.h"
#include "TracksBar.h"
#include "MainFrm.h"
#include "MidiRollDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CTracksBar

IMPLEMENT_DYNAMIC(CTracksBar, CMyDockablePane)

CTracksBar::CTracksBar()
{
}

CTracksBar::~CTracksBar()
{
}

void CTracksBar::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
//	printf("CTracksBar::OnUpdate %x %d %x\n", pSender, lHint, pHint);
	switch (lHint) {
	case CMidiRollDoc::HINT_NONE:
		CreateItems();
		break;
	case CMidiRollDoc::HINT_TRACK_SEL:
		UpdateItems();
		break;
	}
}

void CTracksBar::CreateItems()
{
	CMidiRollDoc	*pDoc = theApp.m_pDoc;
	m_list.ResetContent();	// reset list
	int	nTracks = pDoc->m_arrTrackName.GetSize(); 
	for (int iTrack = 0; iTrack < nTracks; iTrack++) {	// for each track
		m_list.AddString(pDoc->m_arrTrackName[iTrack]);	// add list item for track
		bool	bIsSel = pDoc->m_arrTrackSelect[iTrack];	// get track's selected state
		m_list.SetCheck(iTrack, bIsSel ? BST_CHECKED : BST_UNCHECKED);	// set item's checked state
	}
}

void CTracksBar::UpdateItems()
{
	CMidiRollDoc	*pDoc = theApp.m_pDoc;
	int	nTracks = pDoc->m_arrTrackName.GetSize(); 
	for (int iTrack = 0; iTrack < nTracks; iTrack++) {	// for each track
		bool	bIsSel = pDoc->m_arrTrackSelect[iTrack];	// get track's selected state
		m_list.SetCheck(iTrack, bIsSel ? BST_CHECKED : BST_UNCHECKED);	// set item's checked state
	}
}

void CTracksBar::OnShowChanged(bool bShow)
{
	if (bShow) {
		OnUpdate(NULL);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTracksBar message map

BEGIN_MESSAGE_MAP(CTracksBar, CMyDockablePane)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_CLBN_CHKCHANGE(IDC_LIST, OnListCheckChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTracksBar message handlers

int CTracksBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMyDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	DWORD	dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL 
		| LBS_HASSTRINGS | LBS_OWNERDRAWFIXED | LBS_EXTENDEDSEL;
	if (!m_list.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_LIST))
		return -1;
	m_list.SendMessage(WM_SETFONT, WPARAM(GetStockObject(DEFAULT_GUI_FONT)));

	return 0;
}

void CTracksBar::OnDestroy()
{
	CMyDockablePane::OnDestroy();
}

void CTracksBar::OnSize(UINT nType, int cx, int cy)
{
	CMyDockablePane::OnSize(nType, cx, cy);
	if (m_list.m_hWnd)
		m_list.MoveWindow(0, 0, cx, cy);
}

void CTracksBar::OnSetFocus(CWnd* pOldWnd)
{
	CMyDockablePane::OnSetFocus(pOldWnd);
}

void CTracksBar::OnListCheckChange()
{
	int	nSels = m_list.GetSelCount();
	CIntArrayEx	arrSel;
	arrSel.SetSize(nSels);
	m_list.GetSelItems(nSels, arrSel.GetData());
	CMidiRollDoc	*pDoc = theApp.m_pDoc;
	for (int iSel = 0; iSel < nSels; iSel++) {	// for each selected check list item
		int	iItem = arrSel[iSel];	// get item index
		bool	bIsSel = m_list.GetCheck(iItem) == BST_CHECKED;	// get selected state
		pDoc->m_arrTrackSelect[iItem] = bIsSel;	// update track selection array element
	}
	pDoc->UpdateAllViews(NULL, CMidiRollDoc::HINT_TRACK_SEL);
}
