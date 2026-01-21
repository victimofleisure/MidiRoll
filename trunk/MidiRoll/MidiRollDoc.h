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

#include "Midi.h"
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

// Constants
	enum {	// edit hints
		HINT_NONE,
		HINT_TRACK_SEL,		// track selection change
		HINT_CHANNEL_SEL,	// channel selection change
	};
	enum {	// errors
		ET_NONE,
		ET_OVERLAP,
		ET_SPURIOUS,
		ET_HANGING,
		ERROR_TYPES,
	};
	static const LPCTSTR	m_arrErrorName[ERROR_TYPES];

// Types
	class CNoteEvent {
	public:
		int		m_nStartTime;	// ticks
		int		m_nDuration;	// ticks
		DWORD	m_nMsg;			// MIDI message
		bool operator<(const CNoteEvent& note) const {
			return m_nStartTime < note.m_nStartTime;
		}
		bool operator>(const CNoteEvent& note) const {
			return m_nStartTime > note.m_nStartTime;
		}
		int		m_iTrack;		// index of track within MIDI file
	};
	typedef CArrayEx<CNoteEvent, CNoteEvent&> CNoteEventArray;
	struct ERROR_INFO {
		int		nPos;	// position in ticks
		BYTE	nType;	// error type; see enum
		BYTE	nChan;	// zero-based MIDI channel
		BYTE	nNote;	// zero-based MIDI note
	};
	typedef CArrayEx<ERROR_INFO, ERROR_INFO&> CErrorInfoArray;

// Public data
	CMidiFile::CMidiEventArray	m_arrTempo;
	CMidiFile::TIME_SIGNATURE	m_sigTime;
	CMidiFile::KEY_SIGNATURE	m_sigKey;
	CNoteEventArray m_arrNote;
	CStringArrayEx	m_arrTrackName;
	CBoolArrayEx	m_arrTrackSelect;
	double	m_fTempo;		// BPM
	int		m_nLowNote;		// MIDI note
	int		m_nHighNote;	// MIDI note
	int		m_nLowVelo;		// MIDI velocity
	int		m_nHighVelo;	// MIDI velocity
	int		m_nEndTime;		// ticks
	int		m_nTimeDiv;		// ticks per quarter note
	int		m_nMeter;
	WORD	m_nChanUsedMask;	// bitmask of used MIDI channels
	WORD	m_nChanSelMask;		// bitmask of selected MIDI channels

// Attributes
public:
	int		GetTrackCount() const;
	bool	IsTrackSelected(int iTrack) const;
	void	GetSelectedTracks(CIntArrayEx& arrSel) const;
	void	SelectAllTracks();
	void	SelectTrack(int iTrack);
	void	GetUsedChannels(CIntArrayEx& arrUsed) const;
	int		GetUsedChannelCount() const;
	void	GetSelectedChannels(CIntArrayEx& arrSel) const;
	void	SelectAllChannels();
	void	SelectChannel(int iChan);
	bool	IsChannelUsed(int iChan) const;
	bool	IsChannelSelected(int iChan) const;
	void	SelectChannel(int iChan, bool bIsSel);

// Operations
public:
	bool	ReadMidiNotes(LPCTSTR pszMidiPath);
	void	ConvertPositionToBeat(int nPos, int& nMeasure, int& nBeat, int& nTick) const;
	void	ConvertPositionToString(int nPos, CString& sPos) const;
	void	ConvertDurationToString(int nPos, CString& sPos) const;
	static	WORD	MakeChannelMask(int iChan);
	template<class T> static void SetOrClearBit(T& nMask, T nBit, bool bEnable);
	void	ResetData();
	static	CString	GetKeySigName(int iSharpsOrFlats);

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

public:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);

	// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUpdateFileSave(CCmdUI *pCmdUI);
	afx_msg void OnFileInfo();
	afx_msg void OnUpdateFileInfo(CCmdUI *pCmdUI);
};

inline int CMidiRollDoc::GetTrackCount() const
{
	return m_arrTrackSelect.GetSize();
}

inline bool CMidiRollDoc::IsTrackSelected(int iTrack) const
{
	return m_arrTrackSelect[iTrack];
}

inline WORD CMidiRollDoc::MakeChannelMask(int iChan)
{
	ASSERT(iChan >= 0 && iChan < MIDI_CHANNELS);
	return 1 << iChan;
}

inline bool CMidiRollDoc::IsChannelUsed(int iChan) const
{
	return (m_nChanUsedMask & MakeChannelMask(iChan)) != 0;
}

inline bool CMidiRollDoc::IsChannelSelected(int iChan) const
{
	return (m_nChanSelMask & MakeChannelMask(iChan)) != 0;
}

template<class T> inline void CMidiRollDoc::SetOrClearBit(T& nMask, T nBit, bool bEnable)
{
	if (bEnable) {	// if enabling
		nMask |= nBit;	// set bit
	} else {	// disabling
		nMask &= ~nBit;	// clear bit
	}
}

inline void CMidiRollDoc::SelectChannel(int iChan, bool bIsSel)
{
	SetOrClearBit(m_nChanSelMask, MakeChannelMask(iChan), bIsSel);
}
