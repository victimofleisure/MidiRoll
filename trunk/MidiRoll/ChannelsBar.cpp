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
#include "ChannelsBar.h"
#include "MainFrm.h"
#include "MidiRollDoc.h"
#include "Midi.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CChannelsBar

IMPLEMENT_DYNAMIC(CChannelsBar, CMyDockablePane)

CChannelsBar::CChannelsBar()
{
}

CChannelsBar::~CChannelsBar()
{
}

void CChannelsBar::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
//	printf("CChannelsBar::OnUpdate %x %d %x\n", pSender, lHint, pHint);
	switch (lHint) {
	case CMidiRollDoc::HINT_NONE:
		CreateItems();
		break;
	case CMidiRollDoc::HINT_CHANNEL_SEL:
		UpdateItems();
		break;
	}
}

void CChannelsBar::CreateItems()
{
	CMidiRollDoc	*pDoc = theApp.m_pDoc;
	m_list.ResetContent();
	int	nChans = MIDI_CHANNELS;
	CString	sChan;
	for (int iChan = 0; iChan < nChans; iChan++) {	// for each MIDI channel
		if (pDoc->IsChannelUsed(iChan)) {	// if channel is used
			sChan.Format(_T("%d"), iChan + 1);	// make channel name
			int	iItem = m_list.AddString(sChan);	// add list item for channel
			bool	bIsSel = pDoc->IsChannelSelected(iChan);	// get channel's selected state
			m_list.SetCheck(iItem, bIsSel ? BST_CHECKED : BST_UNCHECKED);	// update item's checked state
			m_list.SetItemData(iItem, iChan);	// set item's data to channel index
		}
	}
}

void CChannelsBar::UpdateItems()
{
	CMidiRollDoc	*pDoc = theApp.m_pDoc;
	int	nItems = m_list.GetCount();
	for (int iItem = 0; iItem < nItems; iItem++) {	// for each list item
		int	iChan = static_cast<int>(m_list.GetItemData(iItem));	// get item's channel index
		bool	bIsSel = pDoc->IsChannelSelected(iChan) != 0;	// get channel's selected state
		m_list.SetCheck(iItem, bIsSel ? BST_CHECKED : BST_UNCHECKED);	// update item's checked state
	}
}

void CChannelsBar::OnShowChanged(bool bShow)
{
	if (bShow) {
		OnUpdate(NULL);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CChannelsBar message map

BEGIN_MESSAGE_MAP(CChannelsBar, CMyDockablePane)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_CLBN_CHKCHANGE(IDC_LIST, OnListCheckChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChannelsBar message handlers

int CChannelsBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMyDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	DWORD	dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL 
		| LBS_HASSTRINGS | LBS_OWNERDRAWFIXED | LBS_EXTENDEDSEL;
	if (!m_list.Create(dwStyle, CRect(0, 0, 0, 0), this, IDC_LIST))
		return -1;
	m_list.SendMessage(WM_SETFONT, WPARAM(GetStockObject(DEFAULT_GUI_FONT)));
	CreateItems();

	return 0;
}

void CChannelsBar::OnDestroy()
{
	CMyDockablePane::OnDestroy();
}

void CChannelsBar::OnSize(UINT nType, int cx, int cy)
{
	CMyDockablePane::OnSize(nType, cx, cy);
	if (m_list.m_hWnd)
		m_list.MoveWindow(0, 0, cx, cy);
}

void CChannelsBar::OnSetFocus(CWnd* pOldWnd)
{
	CMyDockablePane::OnSetFocus(pOldWnd);
}

void CChannelsBar::OnListCheckChange()
{
	int	nSels = m_list.GetSelCount();
	CIntArrayEx	arrSel;
	arrSel.SetSize(nSels);
	m_list.GetSelItems(nSels, arrSel.GetData());
	CMidiRollDoc	*pDoc = theApp.m_pDoc;
	for (int iSel = 0; iSel < nSels; iSel++) {	// for each selected list item
		int	iItem = arrSel[iSel];	// get item index
		int	iChan = static_cast<int>(m_list.GetItemData(iItem));	// get item's channel index
		bool	bIsChecked = m_list.GetCheck(iItem) == BST_CHECKED;	// get item's checked state
		pDoc->SelectChannel(iChan, bIsChecked);	// update channel's bit in selection mask
	}
	pDoc->UpdateAllViews(NULL, CMidiRollDoc::HINT_CHANNEL_SEL);
}
