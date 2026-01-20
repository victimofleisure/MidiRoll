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

// MidiRollDoc.cpp : implementation of the CMidiRollDoc class
//

#include "stdafx.h"
#include "MidiRoll.h"
#include "MidiRollDoc.h"

#include "Midi.h"
#include "PathStr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMidiRollDoc

IMPLEMENT_DYNCREATE(CMidiRollDoc, CDocument)

// CMidiRollDoc construction/destruction

CMidiRollDoc::CMidiRollDoc()
{
	ZeroMemory(&m_sigTime, sizeof(m_sigTime));
	ZeroMemory(&m_sigKey, sizeof(m_sigKey));
	m_fTempo = 0;
	m_nLowNote = 0;
	m_nHighNote = 0;
	m_nLowVelo = 0;
	m_nHighVelo = 0;
	m_nEndTime = 0;
	m_nTimeDiv = 0;
	m_nMeter = 0;
}

CMidiRollDoc::~CMidiRollDoc()
{
}

BOOL CMidiRollDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

bool CMidiRollDoc::ReadMidiNotes(LPCTSTR pszMidiPath)
{
	CMidiFile	mf;
	mf.Open(pszMidiPath, CFile::modeRead);
	CMidiFile::CMidiTrackArray	arrTrack;
	CStringArrayEx	arrTrackName;
	UINT	nTempo = 0;
	m_arrTempo.RemoveAll();
	USHORT	nPPQ;
	mf.ReadTracks(arrTrack, arrTrackName, nPPQ, &nTempo, &m_sigTime, &m_sigKey, &m_arrTempo);
	m_nTimeDiv = nPPQ;
	m_fTempo = double(CMidiFile::MICROS_PER_MINUTE) / nTempo;
	m_nMeter = m_sigTime.Numerator;
	m_arrNote.RemoveAll();
	m_nLowNote = MIDI_NOTE_MAX;
	m_nHighNote = 0;
	m_nLowVelo = MIDI_NOTE_MAX;
	m_nHighVelo = 0;
	m_nEndTime = 0;
	enum {
		ET_OVERLAP,
		ET_SPURIOUS,
		ET_HANGING,
		ERROR_TYPES
	};
	int	nErrors[ERROR_TYPES] = {0};
	struct ERROR_INFO {
		int		nPos;	// position in ticks
		BYTE	nType;	// error type; see enum
		BYTE	nChan;	// zero-based MIDI channel
		BYTE	nNote;	// zero-based MIDI note
	};
	static const LPCTSTR	arrErrorName[ERROR_TYPES] = {
		_T("note overlap"),
		_T("spurious off"),
		_T("hanging note"),
	};
	CArrayEx<ERROR_INFO, ERROR_INFO&> arrError;
	int	nTracks = arrTrack.GetSize();
	for (int iTrack = 1; iTrack < nTracks; iTrack++) {
		const CMidiFile::CMidiEventArray	arrEvt = arrTrack[iTrack];
		int	nEvts = arrEvt.GetSize();
//		printf("track %d: %d events\n", iTrack, nEvts);
		int	arrNoteStart[MIDI_NOTES] = {0};
		BYTE	arrNoteVel[MIDI_NOTES] = {0};
		int	nCurTime = 0;
		for (int iEvt = 0; iEvt < nEvts; iEvt++) {
			DWORD	nMsg = arrEvt[iEvt].Msg;
			DWORD	nDeltaT = arrEvt[iEvt].DeltaT;
			nCurTime += nDeltaT;
//			printf("%d %d %d %d %d\n", nCurTime, MIDI_CMD_IDX(nMsg), MIDI_CHAN(nMsg), MIDI_P1(nMsg), MIDI_P2(nMsg));
			BYTE	nCmd = MIDI_CMD(nMsg);
			if (nCmd == NOTE_ON || nCmd == NOTE_OFF) {
				int	iNote = MIDI_P1(nMsg);
				int	nVel = MIDI_P2(nMsg);
				bool	bNoteOn = (nCmd == NOTE_ON && nVel != 0);
				bool	bSaveNoteOn = bNoteOn;
				int	nErrType = -1;
				if (bNoteOn) {	// if note on event
					if (arrNoteStart[iNote]) {	// if note is active
						nErrType = ET_OVERLAP;	// error: note overlap
						bNoteOn = false;	// retrigger
					}
				} else {	// note off event
					if (!arrNoteStart[iNote]) {	// if note is passive
						nErrType = ET_SPURIOUS;	// error: spurious off
						bNoteOn = true;	// suppress
					}
				}
				if (nErrType >= 0) {	// if error occurred
					ERROR_INFO	info = {nCurTime, nErrType, MIDI_CHAN(nMsg), iNote};
					arrError.Add(info);	// log error
					nErrors[nErrType]++;	// bump error type count
				}
				if (!bNoteOn) {	// if note is complete
					int	nStartTime = arrNoteStart[iNote] - 1;
					int	nDuration = nCurTime - nStartTime;
					int	nStartVel = arrNoteVel[iNote];
					CNoteEvent	note;
					note.m_nStartTime = nStartTime;
					note.m_nDuration = nDuration;
					note.m_nMsg = nMsg | (nStartVel << 16);
					m_arrNote.Add(note);
					if (iNote < m_nLowNote) {
						m_nLowNote = iNote;
					}
					if (iNote > m_nHighNote) {
						m_nHighNote = iNote;
					}
					arrNoteStart[iNote] = 0;
					arrNoteVel[iNote] = 0;
					int	nEndTime = nStartTime + nDuration;
					if (nEndTime > m_nEndTime) {
						m_nEndTime = nEndTime;
					}
				}
				if (bSaveNoteOn) {	// if note on event
					arrNoteStart[iNote] = nCurTime + 1;
					arrNoteVel[iNote] = nVel;
					if (nVel < m_nLowVelo) {
						m_nLowVelo = nVel;
					}
					if (nVel > m_nHighVelo) {
						m_nHighVelo = nVel;
					}
				}
			}
		}
		for (int iNote = 0; iNote < MIDI_NOTES; iNote++) {
			if (arrNoteStart[iNote]) {	// if note still active
				ERROR_INFO	info = {arrNoteStart[iNote], ET_HANGING, iTrack, iNote};
				arrError.Add(info);	// log error
				nErrors[ET_HANGING]++;	// bump error type count
			}
		}
	}
	m_arrNote.Sort();	// sort notes by start position
	if (arrError.GetSize()) {	// if errors occurred
		TRY {
			CPathStr	sPath;
			if (theApp.GetAppDataFolder(sPath) && theApp.CreateFolder(sPath)) {
				sPath.Append(_T("MidiErrs.log"));
				CStdioFile	fOut(sPath, CFile::modeCreate | CFile::modeWrite);	// open log file
				fOut.WriteString(CString(pszMidiPath) + '\n');
				fOut.WriteString(_T("Pos\tChan\tNote\tError\n"));	// write header
				for (int iErr = 0; iErr < arrError.GetSize(); iErr++) {	// for each error
					const ERROR_INFO& info = arrError[iErr];
					CString	sText;
					sText.Format(_T("%d\t%d\t%d\t"), info.nPos, info.nChan + 1, info.nNote);
					fOut.WriteString(sText + CString(arrErrorName[info.nType]) + '\n');
				}
			}
		}
		CATCH (CException, e) {
			e->ReportError();
		}
		END_CATCH
		CString	sMsg;
		sMsg.Format(_T("%d note overlaps\n%d spurious offs\n%d hanging notes"),
			nErrors[ET_OVERLAP], nErrors[ET_SPURIOUS], nErrors[ET_HANGING]);
		AfxMessageBox(sMsg + _T("\n\nSee MidiErrs.log for details."));	// display error message
	}
#if 0
	for (int iNote = 0; iNote < m_arrNote.GetSize(); iNote++) {
		const CNoteEvent& note = m_arrNote[iNote];
		printf("%d\t%d\t%d\t%d\t%d\t%d\n", iNote, note.m_nStartTime, note.m_nDuration,
			MIDI_CHAN(note.m_nMsg), MIDI_P1(note.m_nMsg), MIDI_P2(note.m_nMsg));
	}
#endif
	return true;
}

inline double CTempoMapIter::PositionToSeconds(int nPos) const
{
	return static_cast<double>(nPos) / m_nTimebase / m_fCurTempo * 60;
}

inline double CTempoMapIter::SecondsToPosition(double fSecs) const
{
	return fSecs / 60 * m_fCurTempo * m_nTimebase;
}

CTempoMapIter::CTempoMapIter(const CMidiFile::CMidiEventArray& arrTempoMap, int nTimebase, double fInitTempo) :
	m_arrTempoMap(arrTempoMap)
{
	m_nTimebase = nTimebase;
	m_fInitTempo = fInitTempo;
	Reset();
}

void CTempoMapIter::Reset()
{
	m_fStartTime = 0;
	m_fEndTime = 0;
	m_fCurTempo = m_fInitTempo;
	m_nStartPos = 0;
	m_iTempo = 0;
	if (m_arrTempoMap.GetSize()) {
		m_fEndTime = PositionToSeconds(m_arrTempoMap[0].DeltaT);
	}
}

double CTempoMapIter::GetPosition(double fTime)	// input times must be in ascending order
{
	while (fTime >= m_fEndTime && m_iTempo < m_arrTempoMap.GetSize()) {	// if end of span and spans remain
		m_fStartTime = m_fEndTime;
		m_nStartPos += m_arrTempoMap[m_iTempo].DeltaT;
		m_fCurTempo = static_cast<double>(CMidiFile::MICROS_PER_MINUTE) / m_arrTempoMap[m_iTempo].Msg;
		m_iTempo++;
		if (m_iTempo < m_arrTempoMap.GetSize())	// if spans remain
			m_fEndTime += PositionToSeconds(m_arrTempoMap[m_iTempo].DeltaT);
		else	// out of spans
			m_fEndTime = INT_MAX;
	}
	ASSERT(fTime >= m_fStartTime);	// input time can't precede start of current span
	return m_nStartPos + SecondsToPosition(fTime - m_fStartTime);
}

double CTempoMapIter::FindTempo(int nPos) const
{
	int	nEvts = m_arrTempoMap.GetSize();
	int	nStartPos = 0;
	int	iEvt;
	for (iEvt = 0; iEvt < nEvts; iEvt++) {
		nStartPos += m_arrTempoMap[iEvt].DeltaT;
		if (nStartPos > nPos)	// if map item starts beyond target
			break;
	}
	iEvt--;	// use previous entry
	if (iEvt >= 0)
		return static_cast<double>(CMidiFile::MICROS_PER_MINUTE) / m_arrTempoMap[iEvt].Msg;
	return m_fInitTempo;
}

void CMidiRollDoc::ConvertPositionToBeat(int nPos, int& nMeasure, int& nBeat, int& nTick) const
{
	nBeat = nPos / m_nTimeDiv;
	nTick = nPos % m_nTimeDiv;
	if (nTick < 0) {	// if negative tick
		nBeat--;	// compensate beat
		nTick += m_nTimeDiv;	// wrap tick to make it positive
	}
	if (m_nMeter > 1) {	// if valid meter
		nMeasure = nBeat / m_nMeter;
		nBeat = nBeat % m_nMeter;
		if (nBeat < 0) {	// if negative beat
			nMeasure--;	// compensate measure
			nBeat += m_nMeter;	// wrap beat to make it positive
		}
	} else	// measure doesn't apply
		nMeasure = -1;
}

void CMidiRollDoc::ConvertPositionToString(int nPos, CString& sPos) const
{
	int	nMeasure, nBeat, nTick;
	ConvertPositionToBeat(nPos, nMeasure, nBeat, nTick);
	// convert measure and beat from zero-origin to one-origin
	if (m_nMeter > 1)	// if valid meter
		sPos.Format(_T("%d:%d:%03d"), nMeasure + 1, nBeat + 1, nTick);
	else	// measure doesn't apply
		sPos.Format(_T("%d:%03d"), nBeat + 1, nTick);
}

void CMidiRollDoc::ConvertDurationToString(int nPos, CString& sPos) const
{
	int	nMeasure, nBeat, nTick;
	ConvertPositionToBeat(nPos, nMeasure, nBeat, nTick);
	// convert measure and beat from zero-origin to one-origin
	if (m_nMeter > 1)	// if valid meter
		sPos.Format(_T("%d:%d:%03d"), nMeasure, nBeat, nTick);
	else	// measure doesn't apply
		sPos.Format(_T("%d:%03d"), nBeat, nTick);
}

// CMidiRollDoc serialization

void CMidiRollDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

// CMidiRollDoc diagnostics

#ifdef _DEBUG
void CMidiRollDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMidiRollDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

// CMidiRollDoc message map

BEGIN_MESSAGE_MAP(CMidiRollDoc, CDocument)
END_MESSAGE_MAP()

// CMidiRollDoc commands

BOOL CMidiRollDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	if (!ReadMidiNotes(lpszPathName)) 
		return false;

	return TRUE;
}
