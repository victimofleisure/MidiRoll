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

// MidiRollView.h : interface of the CMidiRollView class
//

#pragma once

#include <memory>	// for auto_ptr
#include "DPoint.h"
#include "Midi.h"
#include "RulerCtrl.h"

class CMidiRollView : public CScrollView
{
protected: // create from serialization only
	CMidiRollView();
	DECLARE_DYNCREATE(CMidiRollView)

// Constants
	enum {
		RULER_ID = 3000,
	};

// Attributes
public:
	CMidiRollDoc* GetDocument() const;

// Operations
public:
	void	OnRulerClicked(CPoint pt);
	void	OnRulerWheel(UINT nFlags, short zDelta, CPoint pt);

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

// Implementation
public:
	virtual ~CMidiRollView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
// Types
	struct COLOR_SCHEME {
		D2D1_COLOR_F	clrBkgnd;
		D2D1_COLOR_F	clrBkSharp;
		D2D1_COLOR_F	clrOutline;
	};

// Constants
	enum {
		BLACK_KEY_MASK = 0x54a,		// for each semitone, non-zero if black key
		WHITE_KEY_PAIR_MASK = 0x810,	// for each semitone, non-zero if pair of adjacent white keys
	};
	enum {
		TIMER_ID = 2010,
		TIMER_PERIOD = 40,
		OCTAVE = 12,
		SELECTION_STATES = 2,
		AXES = 2,
	};
	enum {
		AXIS_MASK_X = 0x01,
		AXIS_MASK_Y = 0x02,
		AXIS_MASK_BOTH = 0x03,
	};
	enum {	// color schemes
		CS_DARK,
		CS_LIGHT,
		COLOR_SCHEMES
	};
	enum {	// alpha presets
		AP_OPAQUE,
		AP_HALF,
		AP_VELOCITY,
		ALPHA_PRESETS,
	};

	static const COLOR_SCHEME m_arrColorScheme[COLOR_SCHEMES];
	static const float m_arrAlphaPreset[ALPHA_PRESETS];

// Member data
	CD2DSizeF	m_szDPI;	// DPI of each axis
	CD2DSizeF	m_szView;	// view size in DIPs
	int		m_nBeatQuant;	// beat quantization in ticks
	bool	m_bUsingD2D;	// true if using Direct2D
	bool	m_bIsPlaying;	// true if playing
	bool	m_bIsExporting;	// true if exporting
	bool	m_bIsRounded;	// true if drawing notes with rounded corners
	BYTE	m_nDeferredUpdate;	// non-zero if update pending
	ULONGLONG	m_nPlayStartTime;	// in milliseconds
	double	m_fPlayNowTime;	// in seconds
	std::auto_ptr<CTempoMapIter>	m_iterTempo;	// tempo iterator
	double	m_fSlackTimeOffset;	// account for slack at start of audio file; in milliseconds
	DPoint	m_ptZoomStep;	// zoom step size
	DPoint	m_ptInitZoom;	// initial zoom
	DPoint	m_ptZoom;	// x is DIPs per tick, y is relative to fit height
	DPoint	m_ptScrollPos;	// in DIPs
	DPoint	m_ptScrollSize;	// in DIPs
	int		m_nNowPos;	// in ticks
	int		m_nNoteSpan;	// in semitones
	int		m_iCurNote;	// index of current note, or -1 if none
	D2D1_COLOR_F m_arrChanColor[MIDI_CHANNELS][SELECTION_STATES];
	CRulerCtrl	m_wndRuler;		// ruler control
	int		m_nRulerHeight;	// in client coords
	CSize	m_szEdge;	// in client coords
	int		m_nSnapUnitShift;	// 0 = 1/4, 1 = 1/2, -1 = 1/8, etc. or no snap if INT_MAX
	int		m_iColorScheme;
	COLOR_SCHEME	m_colorScheme;	// color scheme
	float	m_fAlpha;	// normalized alpha
	int		m_iAlphaPreset;	// index of alpha preset

// Helpers
	bool	ExportVideo(LPCTSTR pszFolderPath, CSize szFrame, double fFrameRate, int nDurationFrames);
	DPoint	GdiToDip(DPoint ptGdi) const;
	DPoint	DipToGdi(DPoint ptDip) const;
	double	DipToTick(double fDip) const;
	double	TickToDip(double fTick) const;
	double	DipToNote(double fDip) const;
	double	NoteToDip(double fNote) const;
	void	UpdateScrollSize();
	void	ZoomPoint(int iAxis, double fZoom, double fOrigin);
	void	ZoomCenter(int iAxis, double fZoom);
	void	ScrollToPosition(POINT pt);
	void	FitToView(int iAxis = -1);
	void	SetScrollPos(int iAxis, double fScrollPos);
	void	UpdateScrollPos(int iAxis);
	void	UpdateNowMarker();
	double	GetKeyHeight() const;
	int		FindNote(DPoint ptDip) const;
	bool	QueueDeferredUpdate(BYTE nAxisMask);
	void	UpdateChannelColors();
	void	UpdateRuler();
	void	UpdateTempo();
	void	UpdateTempo(double fTempo);
	void	ResetTempoIter();
	void	SetNow(int nNowPos);
	void	SetBackgroundType(bool bDark);
	static int	SignedShift(int nVal, int nShift);

// Overrides
	virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll);
	virtual void CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType = adjustBorder);
	virtual void OnUpdate(CView* pSender, LPARAM lHint = 0, CObject* pHint = NULL);

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	LRESULT		OnDrawD2D(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPlay();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnUpdatePlay(CCmdUI *pCmdUI);
	afx_msg void OnRecord();
	virtual void OnInitialUpdate();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnDeferredUpdate(WPARAM wParam, LPARAM lParam);
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnZoomReset();
	afx_msg void OnZoomVertIn();
	afx_msg void OnZoomVertOut();
	afx_msg void OnZoomVertReset();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnColorScheme(UINT nID);
	afx_msg void OnUpdateColorScheme(CCmdUI *pCmdUI);
	afx_msg void OnAlphaPreset(UINT nID);
	afx_msg void OnUpdateAlphaPreset(CCmdUI *pCmdUI);
	afx_msg void OnRewind();
	afx_msg void OnRounded();
	afx_msg void OnUpdateRounded(CCmdUI *pCmdUI);
};

#ifndef _DEBUG  // debug version in MidiRollView.cpp
inline CMidiRollDoc* CMidiRollView::GetDocument() const
   { return reinterpret_cast<CMidiRollDoc*>(m_pDocument); }
#endif

