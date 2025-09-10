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

// MidiRollDoc.h : interface of the CMidiRollDoc class
//

#pragma once

#include "MidiFile.h"
class CTempoMapIter {
public:
	CTempoMapIter(const CMidiFile::CMidiEventArray& arrTempoMap, int nTimebase, double fInitTempo);
	void	Reset();
	double	GetPosition(double fTime);
	double	FindTempo(int nPos) const;
	double	GetTempo() const;
	double	PositionToSeconds(int nPos) const;
	double	SecondsToPosition(double fSecs) const;

protected:
	const CMidiFile::CMidiEventArray&	m_arrTempoMap;	// reference to array of tempo spans
	double	m_fStartTime;	// starting time of current span, in seconds
	double	m_fEndTime;		// ending time of current span, in seconds
	double	m_fCurTempo;	// tempo of current span, in beats per minute
	double	m_fInitTempo;	// song's initial tempo, in beats per minute
	int		m_nTimebase;	// song's timebase, in ticks per quarter note
	int		m_nStartPos;	// starting position of current span, in ticks
	int		m_iTempo;		// index of current span within tempo map
};

inline double CTempoMapIter::GetTempo() const
{
	return m_fCurTempo;
}

class CMidiRollDoc : public CDocument
{
protected: // create from serialization only
	CMidiRollDoc();
	DECLARE_DYNCREATE(CMidiRollDoc)

// Attributes
public:
	class CNoteEvent {
	public:
		int		m_nStartTime;	// ticks
		int		m_nDuration;	// ticks
		DWORD	m_nCmd;	// MIDI command
		bool operator<(const CNoteEvent& note) const {
			return m_nStartTime < note.m_nStartTime;
		}
		bool operator>(const CNoteEvent& note) const {
			return m_nStartTime > note.m_nStartTime;
		}
	};
	typedef CArrayEx<CNoteEvent, CNoteEvent&> CNoteEventArray;
	CMidiFile::CMidiEventArray	m_arrTempo;
	CMidiFile::TIME_SIGNATURE	m_sigTime;
	CMidiFile::KEY_SIGNATURE	m_sigKey;
	CNoteEventArray m_arrNote;
	double	m_fTempo;		// BPM
	int		m_nLowNote;		// MIDI note
	int		m_nHighNote;	// MIDI note
	int		m_nEndTime;		// ticks
	int		m_nTimeDiv;		// ticks per quarter note
	int		m_nMeter;
	void	ConvertPositionToBeat(int nPos, int& nMeasure, int& nBeat, int& nTick) const;
	void	ConvertPositionToString(int nPos, CString& sPos) const;
	void	ConvertDurationToString(int nPos, CString& sPos) const;

// Operations
public:
	bool	ReadMidiNotes(LPCTSTR pszMidiPath);

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CMidiRollDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
};
