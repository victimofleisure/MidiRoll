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

#pragma once

#include "MyDockablePane.h"

class CChannelsBar : public CMyDockablePane
{
	DECLARE_DYNAMIC(CChannelsBar)
// Construction
public:
	CChannelsBar();

// Attributes
public:

// Operations
public:
	void	OnUpdate(CView* pSender, LPARAM lHint = 0, CObject* pHint = NULL);

// Overrides

// Implementation
public:
	virtual ~CChannelsBar();

protected:
// Types

// Constants
	enum {
		IDC_LIST = 2115
	};

// Member data
	CCheckListBox	m_list;

// Helpers
	void	CreateItems();
	void	UpdateItems();

// Overrides
	virtual	void OnShowChanged(bool bShow);

// Generated message map functions
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnListCheckChange();
};

