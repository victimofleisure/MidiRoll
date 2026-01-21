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

#ifdef MAINDOCKBARDEF

// Don't remove or reorder entries! Append only to avoid incompatibility.

//			   name			width	height	style
MAINDOCKBARDEF(Channels,	200,	300,	dwBaseStyle | CBRS_RIGHT)
MAINDOCKBARDEF(Tracks,		200,	300,	dwBaseStyle | CBRS_LEFT)

// After adding a new dockable bar here:
// 1. Add a resource string IDS_BAR_Foo where Foo is the bar name.
// 2. Add a registry key RK_Foo for the bar in AppRegKey.h.
//
// Otherwise MidiRoll.cpp won't compile; it uses the resource strings
// in CreateDockingWindows and the registry keys in ResetWindowLayout.
// The docking bar IDs, member variables, and code to create and dock
// the bars are all generated automatically by the macros above.

#endif	// MAINDOCKBARDEF
#undef MAINDOCKBARDEF
