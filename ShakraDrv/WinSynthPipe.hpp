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

typedef struct {
	volatile int ShortReadHead;
	volatile int ShortWriteHead;
	volatile int LongReadHead;
	volatile int LongWriteHead;
} EvBufHeads, *PEvBufHeads, EBH, *PEBH;

typedef struct {
	char* LongBuf;
	int LongBufSize[MAX_MIDIHDR_BUF];
} LongEvBuf, *PLongEvBuf, LEB, *PLEB;

namespace WinDriver {
	class SynthPipe {
	private:
		ErrorSystem::WinErr SynthErr;

		// Pipe names
		const wchar_t* FileMappingTemplate = L"Local\\Shakra%sMem%u\0";
		const wchar_t* SEvLabel = L"SEv";
		const wchar_t* LEvLabel = L"LEv";
		const wchar_t* LEvLenLabel = L"LEvLen";
		const wchar_t* EvHeadsLabel = L"EvHeads";
		int EvBufSize = 16384;

		// R/W heads
		EvBufHeads** DrvEvBufHeads = nullptr;
		HANDLE* PDrvEvBufHeads = nullptr;

		// EvBuf file mapping handles
		HANDLE* PDrvLongEvBuf = nullptr;
		BYTE** DrvLongEvBuf;
		HANDLE* PDrvLongEvBufLen = nullptr;
		DWORD** DrvLongEvBufLen;

		HANDLE* PDrvEvBuf = nullptr;
		DWORD** DrvEvBuf;

		bool PrepareArrays();

	public:
		bool OpenSynthHost();
		bool PrepareFileMappings(unsigned short PipeID, bool Create, int Size);
		bool ClosePipe(unsigned short PipeID);
		bool PerformBufferCheck(unsigned short PipeID);
		void ResetReadHeadsIfNeeded(unsigned short PipeID);
		int GetShortReadHeadPos(unsigned short PipeID);
		int GetShortWriteHeadPos(unsigned short PipeID);
		unsigned int ParseShortEvent(unsigned short PipeID);
		unsigned int ParseLongEvent(unsigned short PipeID, BYTE* PEvent);
		void SaveShortEvent(unsigned short PipeID, unsigned int Event);
		unsigned int SaveLongEvent(unsigned short PipeID, LPMIDIHDR Event);
		unsigned int PrepareLongEvent(unsigned short PipeID, LPMIDIHDR Event);
		unsigned int UnprepareLongEvent(unsigned short PipeID, LPMIDIHDR Event);
	};
}

#endif