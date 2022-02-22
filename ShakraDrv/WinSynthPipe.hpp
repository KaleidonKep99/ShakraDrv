/*
Shakra Driver component for Windows
This .h file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#pragma once


#ifndef WINSYNTH_H

#define WINSYNTH_H

#include "WinError.hpp"
#include "WinVars.hpp"
#include <windows.h>
#include <ShlObj_core.h>
#include <tlhelp32.h>
#include <mmddk.h>
#include <AclAPI.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <new>
#include <cstdio>
#include <random>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <functional>

/*

	Why this, instead of using a normal DWORD array?

	My friend SonoSooS did some research, and he found that
	CPUs seem to read/write events faster if they're aligned
	to 32-bit registers.

*/

typedef struct {
	DWORD Event;		// The actual event
	DWORD Align[15];	// Dummy data needed to align the event to 32-bit registers
} ShortEvent, ShortEv, *PShortEv, SE, *PSE;

typedef struct {
	char Event[MAX_MIDIHDR_BUF];	// The long data buffer
	int EventLength;				// The length of the data that needs to be used (can be less than the data stored)
} LongEvent, LongEv, *PLongEv, LE, *PLE;

/*
	
	ReadHead = The current position of the read head in the buffer
	WriteHead = The current position of the write head in the buffer
	Buf = THe actual buffer

	We use volatile integers as read/write heads, to avoid deadlocks.

*/

typedef struct {
	volatile int ReadHead;		
	volatile int WriteHead;		
	PSE Buf;
} ShortEventsBuffer, ShortEvBuf, *PShortEvBuf, SEB, *PSEB;

typedef struct {
	volatile int ReadHead;
	volatile int WriteHead;
	PLE Buf;
} LongEventsBuffer, LongEvBuf, *PLongEvBuf, LEB, *PLEB;

namespace WinDriver {
	class SynthPipe {
	private:
		ErrorSystem::WinErr SynthErr;

		// Pipe names
		const wchar_t* FileMappingTemplate = L"Local\\Shakra%s%s\0";
		const wchar_t* SEvLabel = L"SEv";
		const wchar_t* LEvLabel = L"LEv";

		int EvBufSize = 32768;

		// R/W heads
		PShortEvBuf DrvShortEvBuf = nullptr;
		PLongEvBuf DrvLongEvBuf = nullptr;
		HANDLE PDrvShortEvBuf = nullptr;
		HANDLE PDrvLongEvBuf = nullptr;

		std::wstring GenerateID();
		bool PrepareArrays();

	public:
		bool OpenSynthHost(const wchar_t* Target);
		bool PrepareFileMappings(const wchar_t* Pipe, bool Create, int Size);
		bool ClosePipe();
		bool PerformBufferCheck();
		void ResetReadHeadsIfNeeded();
		int GetReadHeadPos();
		int GetWriteHeadPos();
		unsigned int ParseShortEvent();
		unsigned int ParseLongEvent(BYTE* PEvent);
		void SaveShortEvent(unsigned int Event);
		unsigned int SaveLongEvent(LPMIDIHDR Event);
		unsigned int PrepareLongEvent(LPMIDIHDR Event);
		unsigned int UnprepareLongEvent(LPMIDIHDR Event);
	};
}

#endif