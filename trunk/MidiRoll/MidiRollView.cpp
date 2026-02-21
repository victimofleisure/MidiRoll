// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda
 
		revision history:
		rev		date	comments
		00		23jul25	initial version
		01		21jan26	add channel and track selection
 
*/

// MidiRollView.cpp : implementation of the CMidiRollView class
//

#include "stdafx.h"
#include "MidiRoll.h"
#include "MidiRollDoc.h"
#include "MidiRollView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMidiRollView

#include "RecordDlg.h"
#include "FolderDialog.h"
#include "D2DImageWriter.h"
#include "PathStr.h"
#include "ProgressDlg.h"
#include <math.h>
#include "Note.h"
#include "SaveObj.h"
#include "MainFrm.h"

#define RK_EXPORT_FOLDER _T("ExportFolder")
#define RK_EXPORT_DURATION _T("ExportDuration")

#define REMAP_CHANNELS 0
#define CHANNEL_PALETTE 1	// 0 == MIDITrail, 1 == Paul Tol, 2 == Glasbey

IMPLEMENT_DYNCREATE(CMidiRollView, CScrollView)

#define ID_VIEW_COLOR_SCHEME_FIRST ID_VIEW_COLOR_SCHEME_1
#define ID_VIEW_COLOR_SCHEME_LAST ID_VIEW_COLOR_SCHEME_2
#define ID_VIEW_ALPHA_PRESET_FIRST ID_VIEW_ALPHA_PRESET_1
#define ID_VIEW_ALPHA_PRESET_LAST ID_VIEW_ALPHA_PRESET_3

// CMidiRollView construction/destruction

#define DTF(x) static_cast<float>(x)

const CMidiRollView::COLOR_SCHEME CMidiRollView::m_arrColorScheme[COLOR_SCHEMES] = {
	{	// CS_LIGHT
		D2D1::ColorF(1, 1, 1),
		D2D1::ColorF(0.9f, 0.9f, 0.9f),
		D2D1::ColorF(0, 0, 0)
	},
	{	// CS_DARK
		D2D1::ColorF(0.1f, 0.1f, 0.1f),
		D2D1::ColorF(0, 0, 0),
		D2D1::ColorF(1, 1, 1)
	},
};

const float CMidiRollView::m_arrAlphaPreset[ALPHA_PRESETS] = {1, 0.5, -1};

#define RK_COLOR_SCHEME _T("ColorScheme")
#define RK_ALPHA_PRESET _T("AlphaPreset")

CMidiRollView::CMidiRollView()
{
	m_szDPI = CD2DSizeF(0, 0);
	m_szView = CD2DSizeF(0, 0);
	m_nBeatQuant = 120;
	m_bUsingD2D = false;
	m_bIsPlaying = false;
	m_bIsExporting = false;
	m_bIsRounded = false;
	m_nDeferredUpdate = 0;
	m_nPlayStartTime = 0;
	m_fPlayNowTime = 0;
	m_fSlackTimeOffset = 0;
	m_fSlackTimeOffset = -0.2;	// -200 ms for Facts in Dispute
//	m_fSlackTimeOffset = 0.3;	// +300 ms for Carved By Ear
//	m_fSlackTimeOffset = -0.1;	// for BalaGray videos
	m_ptInitZoom = DPoint(0.125, 1);
	m_ptZoom = m_ptInitZoom;
	m_ptScrollPos = DPoint(0, 0);
	m_ptScrollSize = DPoint(0, 0);
	m_ptZoomStep = DPoint(1.5, 1.5);
	m_nNowPos = -1;
	m_nNoteSpan = 0;
	m_iCurNote = -1;
	m_nRulerHeight = 20;
	m_szEdge = CSize(GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
	m_nSnapUnitShift = -2;	// sixteenth note
	ZeroMemory(m_arrChanColor, sizeof(m_arrChanColor));
	m_iColorScheme = theApp.GetProfileInt(REG_SETTINGS, RK_COLOR_SCHEME, CS_LIGHT);
	m_iColorScheme = CLAMP(m_iColorScheme, 0, COLOR_SCHEMES - 1);
	m_colorScheme = m_arrColorScheme[m_iColorScheme];
	m_iAlphaPreset = theApp.GetProfileInt(REG_SETTINGS, RK_ALPHA_PRESET, AP_OPAQUE);
	m_fAlpha = m_arrAlphaPreset[m_iAlphaPreset];
}

CMidiRollView::~CMidiRollView()
{
	theApp.WriteProfileInt(REG_SETTINGS, RK_COLOR_SCHEME, m_iColorScheme);
	theApp.WriteProfileInt(REG_SETTINGS, RK_ALPHA_PRESET, m_iAlphaPreset);
}

BOOL CMidiRollView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CScrollView::PreCreateWindow(cs);
}

inline DPoint CMidiRollView::GdiToDip(DPoint ptGdi) const
{
	return DPoint(ptGdi.x * (96.0 / m_szDPI.width), ptGdi.y * (96.0 / m_szDPI.height));
}

inline DPoint CMidiRollView::DipToGdi(DPoint ptDip) const
{
	return DPoint(ptDip.x * (m_szDPI.width / 96.0), ptDip.y * (m_szDPI.height / 96.0));
}

inline double CMidiRollView::DipToTick(double fDip) const
{
	return (fDip + m_ptScrollPos.x) / m_ptZoom.x;
}

inline double CMidiRollView::TickToDip(double fTick) const
{
	return (fTick * m_ptZoom.x) - m_ptScrollPos.x;
}

inline double CMidiRollView::DipToNote(double fDip) const
{
	return (fDip + m_ptScrollPos.y) / m_ptZoom.y;
}

inline double CMidiRollView::NoteToDip(double fNote) const
{
	return (fNote * m_ptZoom.x) - m_ptScrollPos.x;
}

void CMidiRollView::SetScrollPos(int iAxis, double fScrollPos)
{
	ASSERT(iAxis >= 0 && iAxis < AXES);
	float	fSize = iAxis == SB_HORZ ? m_szView.width : m_szView.height;
	double	fMaxScrollPos = max(m_ptScrollSize[iAxis] - fSize, 0);
	m_ptScrollPos[iAxis] = CLAMP(fScrollPos, 0, fMaxScrollPos);
}

void CMidiRollView::UpdateScrollPos(int iAxis)
{
	if (iAxis == SB_HORZ) {
		SetScrollPos(SB_HORZ, GdiToDip(DPoint(GetScrollPosition().x, 0)).x);
	} else {
		SetScrollPos(SB_VERT, GdiToDip(DPoint(0, GetScrollPosition().y)).y);
	}
}

void CMidiRollView::ScrollToPosition(POINT pt)    // logical coordinates
{
	// copied from base class method
	ASSERT(m_nMapMode > 0);     // not allowed for shrink to fit
	// now in device coordinates - limit if out of range
	int xMax = GetScrollLimit(SB_HORZ);
	int yMax = GetScrollLimit(SB_VERT);
	if (pt.x < 0)
		pt.x = 0;
	else if (pt.x > xMax)
		pt.x = xMax;
	if (pt.y < 0)
		pt.y = 0;
	else if (pt.y > yMax)
		pt.y = yMax;
	CScrollView::SetScrollPos(SB_HORZ, pt.x);
	CScrollView::SetScrollPos(SB_VERT, pt.y);
	// except don't call ScrollWindow, to avoid glitch;
	// entire client rectangle is always drawn anyway
	Invalidate();
}

BOOL CMidiRollView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll)
{
	// from Q166473: CScrollView Scroll Range Limited to 32K
	int	iAxis = 0;
	if (LOBYTE(nScrollCode) < SB_ENDSCROLL) {	// if horizontal scroll
		iAxis = SB_HORZ;
	} else if (HIBYTE(nScrollCode) < SB_ENDSCROLL) {	// else if vertical scroll
		iAxis = SB_VERT;
	} else {	// no scroll
		return false;
	}
	SCROLLINFO info;
	info.cbSize = sizeof(SCROLLINFO);
	info.fMask = SIF_TRACKPOS;
	GetScrollInfo(iAxis, &info);
	nPos = info.nTrackPos;	// override 16-bit position
	BOOL	bRetc = CScrollView::OnScroll(nScrollCode, nPos, bDoScroll);
	UpdateScrollPos(iAxis);
	return bRetc;
}

BOOL CMidiRollView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	BOOL	bRetc = CScrollView::OnScrollBy(sizeScroll, bDoScroll);
	if (sizeScroll.cx) {
		UpdateScrollPos(SB_HORZ);
		m_wndRuler.ScrollToPosition(GetScrollPosition().x - m_szEdge.cx);
		m_wndRuler.UpdateWindow();	// avoids partial paint when scrolling fast
	}
	if (sizeScroll.cy) {
		UpdateScrollPos(SB_VERT);
	}
	return bRetc;
}

void CMidiRollView::UpdateScrollSize()
{
	CSize	szScroll(0, 0);
	CMidiRollDoc	*pDoc = GetDocument();
	if (pDoc->m_nEndTime) {
		m_ptScrollSize.x = pDoc->m_nEndTime * m_ptZoom.x;
		DPoint	ptGdi(DipToGdi(DPoint(m_ptScrollSize.x, 0)));
		szScroll.cx = Round(ptGdi.x);
	}
	if (m_nNoteSpan) {
		m_ptScrollSize.y = m_nNoteSpan * GetKeyHeight() * m_ptZoom.y;
		DPoint	ptGdi(DipToGdi(DPoint(0, m_ptScrollSize.y)));
		szScroll.cy = Round(ptGdi.y);
	}
	// SetScrollSizes -> UpdateBars -> ScrollToDevicePosition -> ScrollWindow
	SetScrollSizes(MM_TEXT, szScroll);
}

void CMidiRollView::UpdateRuler()
{
	CMidiRollDoc	*pDoc = GetDocument();
	int	nPPQ = pDoc->m_nTimeDiv ? pDoc->m_nTimeDiv : 120;
	double	fZoom = 1 / (DipToGdi(DPoint(m_ptZoom.x, 0)).x * nPPQ);
	m_wndRuler.SetZoom(fZoom);
	m_wndRuler.SetScrollPosition(GetScrollPosition().x - m_szEdge.cx);
	CRulerCtrl::CTickArray	arrTick;
	m_wndRuler.CalcTextExtent(&arrTick);
}

void CMidiRollView::FitToView(int iAxis)
{
	ASSERT(iAxis < AXES);
	CMidiRollDoc	*pDoc = GetDocument();
	BYTE	nAxisMask = 0;
	if (iAxis == SB_HORZ || iAxis < 0) {
		m_ptZoom.x = m_szView.width / pDoc->m_nEndTime;
		m_ptScrollPos.x = 0;
		nAxisMask |= AXIS_MASK_X;
	}
	if (iAxis == SB_VERT || iAxis < 0) {
		m_ptZoom.y = 1;
		m_ptScrollPos.y = 0;
		nAxisMask |= AXIS_MASK_Y;
	}
	QueueDeferredUpdate(nAxisMask);
	UpdateScrollSize();
}

void CMidiRollView::ZoomPoint(int iAxis, double fZoom, double fOrigin)
{
	ASSERT(iAxis >= 0 && iAxis < AXES);
	QueueDeferredUpdate(1 << iAxis);
	double	fDeltaZoom = fZoom / m_ptZoom[iAxis];
	double	fOffset = (fOrigin + m_ptScrollPos[iAxis]) * (fDeltaZoom - 1);
	m_ptZoom[iAxis] = fZoom;
	UpdateScrollSize();	// order matters
	SetScrollPos(iAxis, m_ptScrollPos[iAxis] + fOffset);	// after updating scroll size
	ScrollToPosition(DipToGdi(m_ptScrollPos));
}

void CMidiRollView::ZoomCenter(int iAxis, double fZoom)
{
	float	fSize = iAxis == SB_HORZ ? m_szView.width : m_szView.height;
	ZoomPoint(iAxis, fZoom, fSize / 2);
}

void CMidiRollView::UpdateNowMarker()
{
	if (m_iterTempo.get() != NULL) {
		double	fTime = m_fPlayNowTime + m_fSlackTimeOffset;
		if (fTime >= 0) {
			m_nNowPos = Round(m_iterTempo->GetPosition(fTime));
			double	fNowX = m_nNowPos * m_ptZoom.x;
			double	fCenter = m_szView.width / 2;
			if (fNowX >= fCenter) {
				SetScrollPos(SB_HORZ, fNowX - fCenter);
				ScrollToPosition(DipToGdi(DPoint(m_ptScrollPos.x, 0)));
				if (!m_bIsExporting) {
					UpdateRuler();
				}
				fNowX = fCenter;
			}
		}
	}
}

int	CMidiRollView::SignedShift(int nVal, int nShift)
{
	if (nShift > 0)
		return nVal << nShift;
	if (nShift < 0)
		return nVal >> -nShift;
	return nVal;
}

void CMidiRollView::ResetTempoIter()
{
	CMidiRollDoc* pDoc = GetDocument();
	m_iterTempo.reset(new CTempoMapIter(pDoc->m_arrTempo, pDoc->m_nTimeDiv, pDoc->m_fTempo));
}

void CMidiRollView::SetNow(int nNowPos)
{
	if (m_nSnapUnitShift != INT_MAX) {
		int	nSnapQuant = SignedShift(GetDocument()->m_nTimeDiv, m_nSnapUnitShift);
		nNowPos = Round(double(nNowPos) / nSnapQuant) * nSnapQuant;
	}
	m_nNowPos = max(nNowPos, 0);
	Invalidate();
	ResetTempoIter();
	double	fTempo = m_iterTempo->FindTempo(m_nNowPos);
	UpdateTempo(fTempo);
}

void CMidiRollView::OnRulerClicked(CPoint pt)
{
	if (m_bIsPlaying)
		return;
	double x = GdiToDip(DPoint(pt.x - m_szEdge.cx, 0)).x;
	SetNow(Round(DipToTick(x)));
}

void CMidiRollView::OnRulerWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CRect	rRuler;
	m_wndRuler.GetWindowRect(rRuler);
	if (rRuler.PtInRect(pt)) {
		OnMouseWheel(nFlags, zDelta, pt);
	}
}

void CMidiRollView::UpdateTempo()
{
	double	fTempo;
	if (m_iterTempo.get() != NULL) {
		fTempo = m_iterTempo->GetTempo();
	} else {
		fTempo = GetDocument()->m_fTempo;
	}
	UpdateTempo(fTempo);
}

void CMidiRollView::UpdateTempo(double fTempo)
{
	CString	sText;
	sText.Format(_T("%.2f"), fTempo);
	theApp.GetMainFrame()->m_wndStatusBar.SetPaneText(2, sText);
}

// CMidiRollView drawing

void CMidiRollView::OnDraw(CDC* /*pDC*/)
{
	CMidiRollDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;
}

void CMidiRollView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	CScrollView::CalcWindowRect(lpClientRect, nAdjustType);
	lpClientRect->top += m_nRulerHeight;
}

bool CMidiRollView::QueueDeferredUpdate(BYTE nAxisMask)
{
	ASSERT(nAxisMask);
	if (m_nDeferredUpdate)
		return false;	// nesting not allowed
	m_nDeferredUpdate = nAxisMask;
	PostMessage(UWM_DEFERRED_UPDATE);
	SetRedraw(false);	// disable drawing; avoids glitches due to ScrollWindow
	return true;
}

static const int m_arrMidiTrailColor[MIDI_CHANNELS] = {	// MIDITrail's default palette
	0xEF7272,
	0x81EF72,
	0x7291EF,
	0xEFA272,
	0x72EF91,
	0x8372EF,
	0xEFD072,
	0x72EFC1,
	0xB072EF,
	0xDEEF72,
	0x72EFEF,
	0xE072EF,
	0xB0EF72,
	0x72BFEF,
	0xEF72D0,
	0xEF72A0,
};

static const POINT m_arrPaulTolColor[MIDI_CHANNELS] = {	// excerpt of Paul Tol's palette
	{0x4477AA, 0x8FADCC}, // medium blue
	{0xEE6677, 0xF5A3AD}, // salmon
	{0x228833, 0x7AB885}, // forest green
	{0xCCBB44, 0xE0D68F}, // olive yellow
	{0x66CCEE, 0xA3E0F5}, // sky blue
	{0xAA3377, 0xCC85AD}, // plum
	{0xBBBBBB, 0xD6D6D6}, // neutral grey
	{0xEE7733, 0xF5AD85}, // orange
	{0x0077BB, 0x66ADD6}, // cerulean
	{0x33BBEE, 0x85D6F5}, // turquoise
	{0x009988, 0x66C2B8}, // teal
	{0x44AA99, 0x8FCCC2}, // sea-foam
	{0xCC3311, 0xE08570}, // brick
	{0x117733, 0x70AD85}, // sap green
	{0xDDCC77, 0xEBE0AD}, // sand
	{0x332288, 0x857AB8}, // indigo
};

static const POINT m_arrGlasbeyColor[MIDI_CHANNELS] = {
	{0xD73510, 0xF28362}, 
	{0x009200, 0x77B966}, 
	{0x8259FF, 0xB692FF},
	{0x00B6CA, 0x84D0DC}, 
	{0xF38AE3, 0xFAB5ED}, 
	{0xE3A614, 0xF4C472}, 
	{0x75C69A, 0xA7DABC}, 
	{0xAEAEEF, 0xCBCAF5}, 
	{0xA6618E, 0xC697B4}, 
	{0x9E6D35, 0xC49E78}, 
	{0xFF8E7D, 0xFFB6A9}, 
	{0xCA31FB, 0xE387FE}, 
	{0x358A71, 0x7FB2A0}, 
	{0x0C82CE, 0x82ACE0}, 
	{0xFF398E, 0xFF8CB4}, 
	{0x69CE00, 0xA6E072},
};

void CMidiRollView::UpdateChannelColors()
{
	for (int iChan = 0; iChan < MIDI_CHANNELS; iChan++) {
		for (int iSelSt = 0; iSelSt < SELECTION_STATES; iSelSt++) {
#if CHANNEL_PALETTE == 0
			int	clrChan = m_arrMidiTrailColor[iChan];
#elif CHANNEL_PALETTE == 1
			int	clrChan = iSelSt ? m_arrPaulTolColor[iChan].y : m_arrPaulTolColor[iChan].x;
#elif CHANNEL_PALETTE == 2
			int	clrChan = iSelSt ? m_arrGlasbeyColor[iChan].y : m_arrGlasbeyColor[iChan].x;
#endif
			m_arrChanColor[iChan][iSelSt] = D2D1::ColorF(clrChan);
		}
	}
}

static const int m_arrChanMap[] = {6, 2, 0, 1, 3};

inline double CMidiRollView::GetKeyHeight() const
{
	return (m_szView.height - 1) / m_nNoteSpan;
}

LRESULT CMidiRollView::OnDrawD2D(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	if (m_nDeferredUpdate) {
		return 0;
	}
	CRenderTarget* pRT = reinterpret_cast<CRenderTarget*>(lParam);
	ASSERT_VALID(pRT);
	if (!pRT->IsValid()) {	// if valid render target
		return 0;
	}
	pRT->Clear(m_colorScheme.clrBkgnd);	// erase background
	CMidiRollDoc	*pDoc = GetDocument();
	const float fStroke = 0.5f;
	const float fHighlightStroke = 2;
	int	nHighlightDur = pDoc->m_nTimeDiv * 2;
	double	fKeyHeight = GetKeyHeight() * m_ptZoom.y;
	CD2DSolidColorBrush	brDraw(pRT, m_colorScheme.clrOutline);
	CD2DSolidColorBrush	brSharp(pRT, m_colorScheme.clrBkSharp);
	CD2DSolidColorBrush	brFill(pRT, 0);
	for (int iKey = 0; iKey < m_nNoteSpan; iKey++) {	// for each piano key
		float	fY1 = DTF((m_nNoteSpan - 1 - iKey) * fKeyHeight - m_ptScrollPos.y);
		float	fY2 = DTF(fY1 + fKeyHeight);
		if (fY2 >= 0 && fY1 <= m_szView.height) {	// do vertical clipping
			int	iMidiNote = iKey + pDoc->m_nLowNote;
			int iSemitone = iMidiNote % OCTAVE;
			UINT	nSemitoneBit = 1 << iSemitone;
			if (nSemitoneBit & BLACK_KEY_MASK) {	// if black key
				CD2DRectF	rect(0, fY1, m_szView.width, fY2);
				pRT->FillRectangle(rect, &brSharp);
			} else if (nSemitoneBit & WHITE_KEY_PAIR_MASK) {	// if pair of adjacent white keys
				pRT->DrawLine(CD2DPointF(0, fY1), CD2DPointF(m_szView.width, fY1), &brSharp);
			}
		}
	}
	int	nBeats = pDoc->m_nEndTime / m_nBeatQuant;
	double	fBeatWidth = m_nBeatQuant * m_ptZoom.x;
	const double	fBase = 2;	// powers of two
	const int	nGridSync = 5;	// in steps of powers of two
	double	fNearestExp = log(fBeatWidth) / log(fBase);
	int	nNearestIntExp = Trunc(fNearestExp);
	if (fNearestExp >= 0)	// if nearest exponent is positive
		nNearestIntExp++;	// chop it up; otherwise chop it down
	double	fGridScale = pow(fBase, nNearestIntExp - nGridSync);
	fBeatWidth /= fGridScale;
	nBeats = Round(nBeats * fGridScale);
	for (int iBeat = 0; iBeat <= nBeats; iBeat++) {
		double	x = iBeat * fBeatWidth - m_ptScrollPos.x;
		pRT->DrawLine(CD2DPointF(DTF(x), DTF(0)), CD2DPointF(DTF(x), DTF(m_szView.height)), &brSharp, fStroke);
	}
	CD2DSizeF szRounded;
	if (m_bIsRounded) {
		float	fRoundedRad = float(fKeyHeight / 4);
		szRounded = CD2DSizeF(fRoundedRad, fRoundedRad);
	}
	int	nNotes = pDoc->m_arrNote.GetSize();
	int	nLowVelo = pDoc->m_nLowVelo;
	int	nVeloRange = pDoc->m_nHighVelo - pDoc->m_nLowVelo + 1;
	const double	fVeloAlphaRange = 1.0 / 3.0;
	WORD	nChanMask = pDoc->m_nChanSelMask;
	for (int iNote = 0; iNote < nNotes; iNote++) {
		const CMidiRollDoc::CNoteEvent	note = pDoc->m_arrNote[iNote];
		double	x1 = note.m_nStartTime * m_ptZoom.x - m_ptScrollPos.x;
		double	x2 = x1 + note.m_nDuration * m_ptZoom.x;
		double	y2 = (m_nNoteSpan - (MIDI_P1(note.m_nMsg) - pDoc->m_nLowNote)) * fKeyHeight - m_ptScrollPos.y;
		double	y1 = y2 - fKeyHeight;
		if (x2 < 0 || y2 < 0 || x1 > m_szView.width || y1 > m_szView.height) {
			continue;	// cull
		}
		int	iTrack = note.m_iTrack;
		if (!pDoc->m_arrTrackSelect[iTrack])
			continue;
		int	iChan = MIDI_CHAN(note.m_nMsg);
		if (!((1 << iChan) & nChanMask))
			continue;
#if REMAP_CHANNELS
		if (iChan < _countof(m_arrChanMap)) {
			iChan = m_arrChanMap[iChan];
		}
#endif
		BOOL	iSelState = iNote == m_iCurNote;
		D2D1_COLOR_F	clrFill = m_arrChanColor[iChan][iSelState];
		if (m_fAlpha >= 0) {
			clrFill.a = m_fAlpha;
		} else {
			clrFill.a = DTF((double(MIDI_P2(note.m_nMsg) - nLowVelo)) / nVeloRange * fVeloAlphaRange + (1 - fVeloAlphaRange));
		}
		brFill.SetColor(clrFill);
		float	fOutlineStroke;
		if (m_nNowPos >= note.m_nStartTime && m_nNowPos < note.m_nStartTime + nHighlightDur) {
			float	fStrokeSpan = fHighlightStroke - fStroke;
			fOutlineStroke = fHighlightStroke - fStrokeSpan * (m_nNowPos - note.m_nStartTime) / nHighlightDur;
		} else {
			fOutlineStroke = fStroke;
		}
		CD2DRectF	rNote(DTF(x1), DTF(y1), DTF(x2), DTF(y2));
		if (szRounded.width) {
			pRT->FillRoundedRectangle(CD2DRoundedRect(rNote, szRounded), &brFill);
			pRT->DrawRoundedRectangle(CD2DRoundedRect(rNote, szRounded), &brDraw, fOutlineStroke);
		} else {
			pRT->FillRectangle(rNote, &brFill);
			pRT->DrawRectangle(rNote, &brDraw, fOutlineStroke);
		}
	}
	if (m_nNowPos >= 0) {
		double	fNowX = m_nNowPos * m_ptZoom.x - m_ptScrollPos.x;
		pRT->DrawLine(CD2DPointF(DTF(fNowX), DTF(0)), CD2DPointF(DTF(fNowX), DTF(m_szView.height)), &brDraw);
	}
	return 0;
}

// CMidiRollView diagnostics

#ifdef _DEBUG
void CMidiRollView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CMidiRollView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CMidiRollDoc* CMidiRollView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMidiRollDoc)));
	return (CMidiRollDoc*)m_pDocument;
}
#endif //_DEBUG

int WildcardDeleteFile(CString sPath)
{
	// Note that the destination path is double-null terminated. CString's
	// get buffer method allocates the specified number of characters plus
	// one for the null terminator, but we need space for two terminators,
	// hence we must increment nPathLen.
	int	nPathLen = sPath.GetLength();
	LPTSTR	pszPath = sPath.GetBufferSetLength(nPathLen + 1);
	pszPath[nPathLen + 1] = '\0';	// double-null terminated string
	SHFILEOPSTRUCT	SHFileOp;
	ZeroMemory(&SHFileOp, sizeof(SHFileOp));
	SHFileOp.wFunc = FO_DELETE;
	SHFileOp.pFrom = pszPath;
	SHFileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_FILESONLY | FOF_NORECURSION;
	return SHFileOperation(&SHFileOp);
}

bool CMidiRollView::ExportVideo(LPCTSTR pszFolderPath, CSize szFrame, double fFrameRate, int nDurationFrames)
{
	ASSERT(fFrameRate > 0);
	CMidiRollDoc	*pDoc = GetDocument();
	ASSERT(pDoc != NULL);
	if (pDoc == NULL)
		return false;
	CD2DImageWriter	imgWriter;
	if (!imgWriter.Create(szFrame))	// create image writer
		return false;
	double	fFramePeriod = (1 / fFrameRate);
	CPathStr	sFolderPath(pszFolderPath);
	sFolderPath.Append(_T("img"));
	CString	sFileExt(_T(".png"));
	CString	sFrameNum;
	sFrameNum.Format(_T("%05d"), 0);
	if (PathFileExists(sFolderPath + sFrameNum + sFileExt)) {	// if first frame already exists
		if (AfxMessageBox(IDS_PHASE_EXPORT_OVERWRITE_WARN, MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) != IDYES)
			return false;
	}
	WildcardDeleteFile(sFolderPath + '*' + sFileExt);	// delete previously existing frame files if any
	CProgressDlg	dlg;
	if (!dlg.Create(AfxGetMainWnd()))	// create progress dialog
		return false;
	CD2DSizeF	szNewView(imgWriter.m_rt.GetSize());
	CD2DSizeF	szNewDpi(imgWriter.m_rt.GetDpi());
	CSaveObj<bool> saveExporting(m_bIsExporting, true);
	double	fXScale = m_szDPI.width / szNewDpi.width;
	CSaveObj<CD2DSizeF>	saveSize(m_szView, szNewView);
	CSaveObj<CD2DSizeF>	saveDpi(m_szDPI, szNewDpi);
	CSaveObj<int>	savePos(m_nNowPos);
	CSaveObj<DPoint>	saveZoom(m_ptZoom, DPoint(m_ptZoom.x * fXScale, 1));
	CSaveObj<DPoint>	saveScrollPos(m_ptScrollPos, DPoint(0, 0));
	dlg.SetRange(0, nDurationFrames);
	m_nPlayStartTime = 0;
	ResetTempoIter();
	for (int iFrame = 0; iFrame < nDurationFrames; iFrame++) {	// for each frame
		dlg.SetPos(iFrame);
		if (dlg.Canceled())	// if user canceled
			return false;
		imgWriter.m_rt.BeginDraw();
		m_fPlayNowTime = iFrame * fFramePeriod;
		UpdateNowMarker();
		ValidateRect(NULL);
		OnDrawD2D(0, reinterpret_cast<LPARAM>(&imgWriter.m_rt));
		imgWriter.m_rt.EndDraw();
		sFrameNum.Format(_T("%05d"), iFrame);
		CString	sPath(sFolderPath + sFrameNum + sFileExt);
		if (!imgWriter.Write(sPath))	// write image
			return false;
	}
	return true;
}

void CMidiRollView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
//	printf("CMidiRollView::OnUpdate %x %d %x\n", pSender, lHint, pHint);
	Invalidate();
	theApp.GetMainFrame()->OnUpdate(pSender, lHint, pHint);	// notify main frame
}

// CMidiRollView message map

BEGIN_MESSAGE_MAP(CMidiRollView, CScrollView)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_REGISTERED_MESSAGE(AFX_WM_DRAW2D, OnDrawD2D)
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_COMMAND(ID_VIEW_PLAY, OnPlay)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PLAY, OnUpdatePlay)
	ON_COMMAND(ID_VIEW_RECORD, OnRecord)
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_MESSAGE(UWM_DEFERRED_UPDATE, OnDeferredUpdate)
	ON_COMMAND(ID_VIEW_ZOOM_IN, OnZoomIn)
	ON_COMMAND(ID_VIEW_ZOOM_OUT, OnZoomOut)
	ON_COMMAND(ID_VIEW_ZOOM_RESET, OnZoomReset)
	ON_COMMAND(ID_VIEW_ZOOM_VERT_IN, OnZoomVertIn)
	ON_COMMAND(ID_VIEW_ZOOM_VERT_OUT, OnZoomVertOut)
	ON_COMMAND(ID_VIEW_ZOOM_VERT_RESET, OnZoomVertReset)
	ON_WM_MOUSEWHEEL()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_COMMAND_RANGE(ID_VIEW_COLOR_SCHEME_FIRST, ID_VIEW_COLOR_SCHEME_LAST, OnColorScheme)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_COLOR_SCHEME_FIRST, ID_VIEW_COLOR_SCHEME_LAST, OnUpdateColorScheme)
	ON_COMMAND_RANGE(ID_VIEW_ALPHA_PRESET_FIRST, ID_VIEW_ALPHA_PRESET_LAST, OnAlphaPreset)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_ALPHA_PRESET_FIRST, ID_VIEW_ALPHA_PRESET_LAST, OnUpdateAlphaPreset)
	ON_COMMAND(ID_VIEW_REWIND, OnRewind)
	ON_COMMAND(ID_VIEW_ROUNDED, OnRounded)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROUNDED, OnUpdateRounded)
END_MESSAGE_MAP()

// CMidiRollView message handlers

int CMidiRollView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;
	CRect rInit(0, 0, 0, 0);
	DWORD	dwRulerStyle = WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | CRulerCtrl::NO_ACTIVATE;
	if (!m_wndRuler.Create(dwRulerStyle, rInit, theApp.GetMainFrame(), RULER_ID))
		return -1;
	m_wndRuler.SetUnit(CRulerCtrl::UNIT_MIDI);
	m_wndRuler.SendMessage(WM_SETFONT, WPARAM(GetStockObject(DEFAULT_GUI_FONT)));
	EnableD2DSupport();	// enable Direct2D for this window; takes around 60ms
	m_bUsingD2D = IsD2DSupportEnabled() != 0;	// cache to avoid performance hit
	UpdateChannelColors();
	return 0;
}

void CMidiRollView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	CMidiRollDoc	*pDoc = GetDocument();
	if (pDoc->m_nTimeDiv) {
		m_wndRuler.SetMidiParams(pDoc->m_nTimeDiv, pDoc->m_nMeter);
		m_nBeatQuant = pDoc->m_nTimeDiv;
	}
	m_ptZoom = m_ptInitZoom;
	m_ptScrollPos = DPoint(0, 0);
	m_nNoteSpan = pDoc->m_nHighNote - pDoc->m_nLowNote + 1;
	m_iCurNote = -1;
	m_nNowPos = -1;
	ScrollToPosition(CPoint(0, 0));
	QueueDeferredUpdate(AXIS_MASK_BOTH);
	UpdateTempo();
}

BOOL CMidiRollView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CMidiRollView::OnPlay()
{
	if (m_bIsPlaying) {
		KillTimer(TIMER_ID);
	} else {
		SetTimer(TIMER_ID, TIMER_PERIOD, NULL);
		CMidiRollDoc	*pDoc = GetDocument();
		ResetTempoIter();
		m_fPlayNowTime = 0;
		m_nPlayStartTime = GetTickCount();
		RedrawWindow();
		UpdateTempo();
	}
	m_bIsPlaying ^= 1;
}

void CMidiRollView::OnSize(UINT nType, int cx, int cy)
{
	CScrollView::OnSize(nType, cx, cy);
	if (cx && cy) {
		// main frame draws an edge around the view which must be accounted for
		CRect	rRuler(0, -m_nRulerHeight, cx, 0);
		rRuler.InflateRect(m_szEdge.cx, 0);
		rRuler.OffsetRect(0, -m_szEdge.cy);
		MapWindowPoints(theApp.GetMainFrame(), rRuler);
		m_wndRuler.MoveWindow(rRuler);
		CHwndRenderTarget *pRT = GetRenderTarget();
		m_szView = pRT->GetSize();	// get target size in DIPs
		m_szDPI = pRT->GetDpi();	// store DPI for mouse coordinate scaling
		UpdateScrollSize();	// order matters
		UpdateScrollPos(SB_HORZ);
		UpdateScrollPos(SB_VERT);
		Invalidate();
	}
}

LRESULT CMidiRollView::OnDeferredUpdate(WPARAM wParam, LPARAM lParam)
{
	UpdateScrollSize();
	SetRedraw(true);	// re-enable drawing
	Invalidate();
	if (m_nDeferredUpdate & AXIS_MASK_X) {
		UpdateRuler();
	}
	m_nDeferredUpdate = 0;
	return 0;
}

void CMidiRollView::OnTimer(UINT_PTR nIDEvent)
{
	ULONGLONG nTimeNow = GetTickCount64();
	m_fPlayNowTime = (nTimeNow - m_nPlayStartTime) / 1000.0;
	UpdateNowMarker();
	if (!m_bIsExporting) {
		RedrawWindow();
		UpdateTempo();
	}
	CScrollView::OnTimer(nIDEvent);
}

void CMidiRollView::OnUpdatePlay(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bIsPlaying);
}

void CMidiRollView::OnRecord()
{
	CMidiRollDoc	*pDoc = GetDocument();
	ASSERT(pDoc != NULL);
	if (pDoc == NULL)
		return;
	CString	sFolderPath(theApp.GetProfileString(REG_SETTINGS, RK_EXPORT_FOLDER));
	UINT	nFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
	// browse for folder
	CString	sTitle(LPCTSTR(ID_VIEW_RECORD));
	sTitle.Replace('\n', '\0');	// remove command title from prompt
	if (CFolderDialog::BrowseFolder(sTitle, sFolderPath, NULL, nFlags, sFolderPath)) {
		int	nSongLength = theApp.GetProfileInt(REG_SETTINGS, RK_EXPORT_DURATION, 60);
		CRecordDlg	dlg;
		dlg.m_nDurationSeconds = nSongLength;
		dlg.m_nDurationFrames = Round(dlg.m_fFrameRate * nSongLength);
		if (dlg.DoModal() == IDOK) {	// get video options
			theApp.WriteProfileString(REG_SETTINGS, RK_EXPORT_FOLDER, sFolderPath);
			theApp.WriteProfileInt(REG_SETTINGS, RK_EXPORT_DURATION, nSongLength);
			CSize	szFrame(dlg.m_nFrameWidth, dlg.m_nFrameHeight);
			ExportVideo(sFolderPath, szFrame, dlg.m_fFrameRate, dlg.m_nDurationFrames);
		}
	}
}

int CMidiRollView::FindNote(DPoint ptDip) const
{
	if (ptDip.y >= 0 && ptDip.y < m_szView.height) {
		CMidiRollDoc	*pDoc = GetDocument();
		double	fNote = DipToNote(ptDip.y);
		double	fKeyHeight = GetKeyHeight();
		int	iTargetNote = m_nNoteSpan - 1 - Trunc(fNote / fKeyHeight) + pDoc->m_nLowNote;
		iTargetNote = CLAMP(iTargetNote, pDoc->m_nLowNote, pDoc->m_nHighNote);
		int	nTime = Trunc(DipToTick(ptDip.x));
		int	nNotes = pDoc->m_arrNote.GetSize();
		// reverse iterate to find topmost matching note
		for (int iNote = nNotes - 1; iNote >= 0; iNote--) {
			const CMidiRollDoc::CNoteEvent	note = pDoc->m_arrNote[iNote];
			if (nTime >= note.m_nStartTime && nTime < note.m_nStartTime + note.m_nDuration 
			&& MIDI_P1(note.m_nMsg) == iTargetNote) {
				return iNote;
			}
		}
	}
	return -1;
}

void CMidiRollView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CMidiRollDoc	*pDoc = GetDocument();
	int	iNote = FindNote(GdiToDip(point));
	CString	sText;
	if (iNote >= 0) {
		CMidiRollDoc	*pDoc = GetDocument();
		const CMidiRollDoc::CNoteEvent	note = pDoc->m_arrNote[iNote];
		CString	sStartTime, sDuration;
		pDoc->ConvertPositionToString(note.m_nStartTime, sStartTime);
		pDoc->ConvertDurationToString(note.m_nDuration, sDuration);
		sText.Format(_T("%s ch%d %s %d %s\n"),
			sStartTime,
			MIDI_CHAN(note.m_nMsg) + 1, 
			CNote(MIDI_P1(note.m_nMsg)).MidiName(),
			MIDI_P2(note.m_nMsg),
			sDuration);
		if (nFlags & MK_CONTROL) {
			SetNow(note.m_nStartTime);
		}
	}
	if (iNote != m_iCurNote) {
		m_iCurNote = iNote;
		Invalidate();
		theApp.GetMainFrame()->m_wndStatusBar.SetPaneText(1, sText);
	}
	CScrollView::OnLButtonDown(nFlags, point);
}

void CMidiRollView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CMidiRollView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
}

void CMidiRollView::OnMouseMove(UINT nFlags, CPoint point)
{
//	printf("%f\n", GdiToDip(DPoint(point.x, 0)).x);
	CScrollView::OnMouseMove(nFlags, point);
}

BOOL CMidiRollView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	enum {
		MWF_VERT_SCROLL,
		MWF_HORZ_SCROLL,
		MWF_VERT_ZOOM,
		MWF_HORZ_ZOOM,
		MOUSE_WHEEL_FUNCTIONS
	};
	static const UINT arrModKeyCombo[MOUSE_WHEEL_FUNCTIONS] = {
		0,
		MK_SHIFT,
		MK_CONTROL | MK_SHIFT,
		MK_CONTROL,
	};
	UINT	nFuncMask = nFlags & (MK_CONTROL | MK_SHIFT);
	int iFunc;
	for (iFunc = 0; iFunc < MOUSE_WHEEL_FUNCTIONS; iFunc++) {
		if (nFuncMask == arrModKeyCombo[iFunc])
			break;
	}
	switch (iFunc) {
	case MWF_HORZ_SCROLL:
		{
			// base class DoMouseWheel only scrolls the horizontal axis
			// if vertical scroll is disabled, so temporarily disable it
			UINT	nStyle = GetStyle();
			SetWindowLongPtr(m_hWnd, GWL_STYLE, nStyle & ~WS_VSCROLL);	// disable vertical scroll 
			BOOL	bRet = DoMouseWheel(nFlags, zDelta, pt);
			SetWindowLongPtr(m_hWnd, GWL_STYLE, nStyle);	// restore previous style
			return bRet;
		}
	case MWF_VERT_ZOOM:
		{
			double	fDeltaZoom = pow(m_ptZoomStep.y, double(zDelta) / WHEEL_DELTA);
			double	fNewZoom = m_ptZoom.y * fDeltaZoom;
			ScreenToClient(&pt);
			ZoomPoint(SB_VERT, fNewZoom, GdiToDip(DPoint(0, pt.y)).y);
			return true;
		}
	case MWF_HORZ_ZOOM:
		{
			double	fDeltaZoom = pow(m_ptZoomStep.x, double(zDelta) / WHEEL_DELTA);
			double	fNewZoom = m_ptZoom.x * fDeltaZoom;
			ScreenToClient(&pt);
			ZoomPoint(SB_HORZ, fNewZoom, GdiToDip(DPoint(pt.x, 0)).x);
			return true;
		}
	}
	return CScrollView::OnMouseWheel(nFlags, zDelta, pt);
}

#define MAKESCROLLCODE(horz, vert) ((horz & 0xff) | ((vert & 0xff) << 8))

void CMidiRollView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	static const POINT arrScrollKey[] = {
		{VK_UP,		MAKESCROLLCODE(-1, SB_LINEUP)},
		{VK_DOWN,	MAKESCROLLCODE(-1, SB_LINEDOWN)},
		{VK_LEFT,	MAKESCROLLCODE(SB_LINELEFT, -1)},
		{VK_RIGHT,	MAKESCROLLCODE(SB_LINERIGHT, -1)},
		{VK_PRIOR,	MAKESCROLLCODE(-1, SB_PAGEUP)},
		{VK_NEXT,	MAKESCROLLCODE(-1, SB_PAGEDOWN)},
		{VK_HOME,	MAKESCROLLCODE(-1, SB_TOP)},
		{VK_END,	MAKESCROLLCODE(-1, SB_BOTTOM)},
	};
	for (int iKey = 0; iKey < _countof(arrScrollKey); iKey++) {
		if (nChar == arrScrollKey[iKey].x) {
			int	nScrollCode = arrScrollKey[iKey].y;
			if (iKey >= 4) {	// excluding arrow keys
				// if no vertical scroll bar or modifer key is pressed, swap axes
				if (!(GetStyle() & WS_VSCROLL) || (GetKeyState(VK_CONTROL) & GKS_DOWN)) {
					nScrollCode = MAKEWORD(HIBYTE(nScrollCode), LOBYTE(nScrollCode));
				}
			}
			OnScroll(nScrollCode, 0, true);
			return;
		}
	}
	CScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CMidiRollView::OnZoomIn()
{
	ZoomCenter(SB_HORZ, m_ptZoom.x * m_ptZoomStep.x);
}

void CMidiRollView::OnZoomOut()
{
	ZoomCenter(SB_HORZ, m_ptZoom.x / m_ptZoomStep.x);
}

void CMidiRollView::OnZoomReset()
{
	FitToView(SB_HORZ);
}

void CMidiRollView::OnZoomVertIn()
{
	ZoomCenter(SB_VERT, m_ptZoom.y * m_ptZoomStep.y);
}

void CMidiRollView::OnZoomVertOut()
{
	ZoomCenter(SB_VERT, m_ptZoom.y / m_ptZoomStep.y);
}

void CMidiRollView::OnZoomVertReset()
{
	FitToView(SB_VERT);
}

void CMidiRollView::OnColorScheme(UINT nID)
{
	int	iScheme = nID - ID_VIEW_COLOR_SCHEME_FIRST;
	ASSERT(iScheme >= 0 && iScheme < COLOR_SCHEMES);
	m_colorScheme = m_arrColorScheme[iScheme];
	m_iColorScheme = iScheme;
	Invalidate();
}

void CMidiRollView::OnUpdateColorScheme(CCmdUI *pCmdUI)
{
	int	iScheme = pCmdUI->m_nID - ID_VIEW_COLOR_SCHEME_FIRST;
	ASSERT(iScheme >= 0 && iScheme < COLOR_SCHEMES);
	pCmdUI->SetRadio(iScheme == m_iColorScheme);
}

void CMidiRollView::OnAlphaPreset(UINT nID)
{
	int	iPreset = nID - ID_VIEW_ALPHA_PRESET_FIRST;
	ASSERT(iPreset >= 0 && iPreset < ALPHA_PRESETS);
	m_fAlpha = m_arrAlphaPreset[iPreset];
	m_iAlphaPreset = iPreset;
	Invalidate();
}

void CMidiRollView::OnUpdateAlphaPreset(CCmdUI *pCmdUI)
{
	int	iPreset = pCmdUI->m_nID - ID_VIEW_ALPHA_PRESET_FIRST;
	ASSERT(iPreset >= 0 && iPreset < ALPHA_PRESETS);
	pCmdUI->SetRadio(iPreset == m_iAlphaPreset);
}

void CMidiRollView::OnRewind()
{
	m_nNowPos = -1;
	Invalidate();
}

void CMidiRollView::OnRounded()
{
	m_bIsRounded ^= 1;
}

void CMidiRollView::OnUpdateRounded(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bIsRounded);
	Invalidate();
}
