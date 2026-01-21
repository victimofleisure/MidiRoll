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

#include "MainFrm.h"
#include "PathStr.h"
#include "Note.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMidiRollDoc

IMPLEMENT_DYNCREATE(CMidiRollDoc, CDocument)

const LPCTSTR CMidiRollDoc::m_arrErrorName[ERROR_TYPES] = {
	_T(""),
	_T("note overlap"),
	_T("spurious off"),
	_T("hanging note"),
};

// CMidiRollDoc construction/destruction

CMidiRollDoc::CMidiRollDoc()
{
	ASSERT(theApp.m_pDoc == NULL);
	theApp.m_pDoc = this;
	ResetData();
}

CMidiRollDoc::~CMidiRollDoc()
{
}

BOOL CMidiRollDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	ResetData();

	return TRUE;
}

void CMidiRollDoc::ResetData()
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
	m_nChanUsedMask = 0;
	m_nChanSelMask = 0;
	m_arrNote.RemoveAll();
	m_arrTrackName.RemoveAll();
	m_arrTempo.RemoveAll();
	m_arrTrackSelect.RemoveAll();
}

bool CMidiRollDoc::ReadMidiNotes(LPCTSTR pszMidiPath)
{
	CMidiFile	fMidi;
	fMidi.Open(pszMidiPath, CFile::modeRead);
	CMidiFile::CMidiTrackArray	arrTrack;
	UINT	nTempo = 0;
	m_arrTempo.RemoveAll();
	USHORT	nPPQ;
	fMidi.ReadTracks(arrTrack, m_arrTrackName, nPPQ, &nTempo, &m_sigTime, &m_sigKey, &m_arrTempo);
	if (arrTrack.GetSize() && m_arrTrackName[0].IsEmpty())
		m_arrTrackName[0].LoadString(IDS_TEMPO_TRACK_NAME);
	m_nTimeDiv = nPPQ;
	m_fTempo = double(CMidiFile::MICROS_PER_MINUTE) / nTempo;
	m_nMeter = m_sigTime.Numerator;
	m_arrNote.RemoveAll();
	m_nLowNote = MIDI_NOTE_MAX;
	m_nHighNote = 0;
	m_nLowVelo = MIDI_NOTE_MAX;
	m_nHighVelo = 0;
	m_nEndTime = 0;
	m_nChanUsedMask = 0;
	m_nChanSelMask = 0;
	int	nErrors[ERROR_TYPES] = {0};
	CErrorInfoArray	arrError;
	int	nTracks = arrTrack.GetSize();
	m_arrTrackSelect.FastSetSize(nTracks);
	memset(m_arrTrackSelect.GetData(), true, nTracks * sizeof(bool));
	for (int iTrack = 1; iTrack < nTracks; iTrack++) {
		const CMidiFile::CMidiEventArray	arrEvt = arrTrack[iTrack];
		int	nEvts = arrEvt.GetSize();
//		printf("track %d: %d events\n", iTrack, nEvts);
		int	arrNoteStart[MIDI_CHANNELS][MIDI_NOTES] = {0};
		BYTE	arrNoteVel[MIDI_CHANNELS][MIDI_NOTES] = {0};
		int	nCurTime = 0;
		for (int iEvt = 0; iEvt < nEvts; iEvt++) {
			DWORD	nMsg = arrEvt[iEvt].Msg;
			DWORD	nDeltaT = arrEvt[iEvt].DeltaT;
			nCurTime += nDeltaT;
//			printf("%d %d %d %d %d\n", nCurTime, MIDI_CMD_IDX(nMsg), MIDI_CHAN(nMsg), MIDI_P1(nMsg), MIDI_P2(nMsg));
			BYTE	nCmd = MIDI_CMD(nMsg);
			if (nCmd == NOTE_ON || nCmd == NOTE_OFF) {	// if note command
				int	iNote = MIDI_P1(nMsg);
				int	nVel = MIDI_P2(nMsg);
				int	iChan = MIDI_CHAN(nMsg);
				bool	bNoteOn = (nCmd == NOTE_ON && nVel != 0);
				bool	bOrigNoteOn = bNoteOn;
				int	nErrType = ET_NONE;
				if (bNoteOn) {	// if note on event
					if (arrNoteStart[iChan][iNote]) {	// if note is active
						nErrType = ET_OVERLAP;	// error: note overlap
						bNoteOn = false;	// retrigger note
					}
				} else {	// note off event
					if (!arrNoteStart[iChan][iNote]) {	// if note is passive
						nErrType = ET_SPURIOUS;	// error: spurious off
						bNoteOn = true;	// suppress note off
					}
				}
				if (nErrType > ET_NONE) {	// if error occurred
					ERROR_INFO	info = {nCurTime, nErrType, iChan, iNote};
					arrError.Add(info);	// log error
					nErrors[nErrType]++;	// bump error type count
				}
				if (!bNoteOn) {	// if note is complete
					int	nStartTime = arrNoteStart[iChan][iNote] - 1;
					int	nDuration = nCurTime - nStartTime;
					int	nStartVel = arrNoteVel[iChan][iNote];
					CNoteEvent	note;
					note.m_nStartTime = nStartTime;
					note.m_nDuration = nDuration;
					note.m_nMsg = nMsg | (nStartVel << 16);
					note.m_iTrack = iTrack;
					m_arrNote.Add(note);
					if (iNote < m_nLowNote) {
						m_nLowNote = iNote;
					}
					if (iNote > m_nHighNote) {
						m_nHighNote = iNote;
					}
					arrNoteStart[iChan][iNote] = 0;
					arrNoteVel[iChan][iNote] = 0;
					int	nEndTime = nStartTime + nDuration;
					if (nEndTime > m_nEndTime) {
						m_nEndTime = nEndTime;
					}
					m_nChanUsedMask |= MakeChannelMask(iChan);
				}
				if (bOrigNoteOn) {	// if note on event
					arrNoteStart[iChan][iNote] = nCurTime + 1;
					arrNoteVel[iChan][iNote] = nVel;
					if (nVel < m_nLowVelo) {
						m_nLowVelo = nVel;
					}
					if (nVel > m_nHighVelo) {
						m_nHighVelo = nVel;
					}
				}
			}
		}
		for (int iChan = 0; iChan < MIDI_CHANNELS; iChan++) {	// for each channel
			for (int iNote = 0; iNote < MIDI_NOTES; iNote++) {	// for each note
				if (arrNoteStart[iChan][iNote]) {	// if note still active
					ERROR_INFO	info = {arrNoteStart[iChan][iNote], ET_HANGING, iTrack, iNote};
					arrError.Add(info);	// log error
					nErrors[ET_HANGING]++;	// bump error type count
				}
			}
		}
	}
	m_arrNote.Sort();	// sort notes by start position
	m_nChanSelMask = m_nChanUsedMask;	// select all used channels
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
					fOut.WriteString(sText + CString(m_arrErrorName[info.nType]) + '\n');
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

void CMidiRollDoc::GetSelectedTracks(CIntArrayEx& arrSel) const
{
	int	nTracks = m_arrTrackSelect.GetSize();
	arrSel.SetSize(nTracks);
	arrSel.FastRemoveAll();
	for (int iTrack = 0; iTrack < nTracks; iTrack++) {
		if (m_arrTrackSelect[iTrack])
			arrSel.Add(iTrack);
	}
}

void CMidiRollDoc::SelectAllTracks()
{
	memset(m_arrTrackSelect.GetData(), true, m_arrTrackSelect.GetSize() * sizeof(bool));
	UpdateAllViews(NULL, HINT_TRACK_SEL);
}

void CMidiRollDoc::SelectTrack(int iTrack)
{
	ASSERT(iTrack < m_arrTrackSelect.GetSize());
	ZeroMemory(m_arrTrackSelect.GetData(), m_arrTrackSelect.GetSize() * sizeof(BYTE));
	if (iTrack >= 0)
		m_arrTrackSelect[iTrack] = true;
	UpdateAllViews(NULL, HINT_TRACK_SEL);
}

void CMidiRollDoc::GetUsedChannels(CIntArrayEx& arrUsed) const
{
	arrUsed.SetSize(MIDI_CHANNELS);
	arrUsed.FastRemoveAll();
	for (int iChan = 0; iChan < MIDI_CHANNELS; iChan++) {
		if (IsChannelUsed(iChan))
			arrUsed.Add(iChan);
	}
}

int CMidiRollDoc::GetUsedChannelCount() const
{
	int	nUsed = 0;
	for (int iChan = 0; iChan < MIDI_CHANNELS; iChan++) {
		if (IsChannelUsed(iChan))
			nUsed++;
	}
	return nUsed;
}

void CMidiRollDoc::GetSelectedChannels(CIntArrayEx& arrSel) const
{
	arrSel.SetSize(MIDI_CHANNELS);
	arrSel.FastRemoveAll();
	for (int iChan = 0; iChan < MIDI_CHANNELS; iChan++) {
		if (IsChannelSelected(iChan))
			arrSel.Add(iChan);
	}
}

void CMidiRollDoc::SelectAllChannels()
{
	m_nChanSelMask = m_nChanUsedMask;
	UpdateAllViews(NULL, HINT_CHANNEL_SEL);
}

void CMidiRollDoc::SelectChannel(int iChan)
{
	ASSERT(iChan < MIDI_CHANNELS);
	if (iChan >= 0) {	// if valid channel index
		m_nChanSelMask = MakeChannelMask(iChan);
	} else {	// no selection
		m_nChanSelMask = 0;
	}
	UpdateAllViews(NULL, HINT_CHANNEL_SEL);
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
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_FILE_INFO, OnFileInfo)
	ON_UPDATE_COMMAND_UI(ID_FILE_INFO, OnUpdateFileInfo)
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

void CMidiRollDoc::OnUpdateFileSave(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(false);	// disable save
}

CString CMidiRollDoc::GetKeySigName(int iSharpsOrFlats)
{
	static const LPCTSTR arrKeySigName[] = {
		_T("Cb"), _T("Gb"), _T("Db"), _T("Ab"), _T("Eb"), _T("Bb"), _T("F"),
		_T("C"), _T("G"), _T("D"), _T("A"), _T("E"), _T("B"), _T("F#"), _T("C#")
	};
	CString	sKeySig;
	int	iKey = iSharpsOrFlats + 7;
	if (iKey >= 0 && iKey < _countof(arrKeySigName))
		sKeySig = arrKeySigName[iKey];
	return sKeySig;
}

void CMidiRollDoc::OnFileInfo()
{
	if (m_arrTrackName.IsEmpty())
		return;
	CString	sTimebase;
	sTimebase.Format(_T("%d"), m_nTimeDiv);
	CString	sTempo;
	sTempo.Format(_T("%.2f"), m_fTempo);
	CString	sDuration;
	ConvertDurationToString(m_nEndTime, sDuration);
	CString	sTimeSig;
	if (m_sigTime.Numerator)
		sTimeSig.Format(_T("%d/%d"), m_sigTime.Numerator, 1 << m_sigTime.Denominator);
	CString	sKeySig(GetKeySigName(m_sigKey.SharpsOrFlats));
	CString	sTracks;
	sTracks.Format(_T("%d"), m_arrTrackName.GetSize());
	CNote	noteLow(m_nLowNote);
	CNote	noteHigh(m_nHighNote);
	CNote	noteKey(m_sigKey.SharpsOrFlats * 7);
	noteKey.Normalize();
	CString	sNoteRange('(' + noteLow.MidiName(noteKey) 
		+ _T(", ") + noteHigh.MidiName(noteKey) + ')');
	CString	sVeloRange;
	sVeloRange.Format(_T("(%d, %d)"), m_nLowVelo, m_nHighVelo);
	CString	sMsg = 
		_T("TPQN:\t") + sTimebase + '\n' +
		_T("Tempo:\t") + sTempo + '\n' +
		_T("Length:\t") + sDuration + '\n' +
		_T("Meter:\t") + sTimeSig + '\n' +
		_T("Key:\t") + sKeySig + '\n' +
		_T("Tracks:\t") + sTracks + '\n' +
		_T("Note R:\t") + sNoteRange + '\n' +
		_T("Velo R:\t") + sVeloRange;
	AfxMessageBox(sMsg, MB_ICONINFORMATION);
}

void CMidiRollDoc::OnUpdateFileInfo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_arrTrackName.IsEmpty());
}
