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
#include <mmddk.h>
#include <AclAPI.h>
#include <new>
#include <cstdio>

typedef struct {
	volatile int ReadHead;
	volatile int WriteHead;
} EvBufHeads, *PEvBufHeads, EBH, *PEBH;

namespace WinDriver {
	class SynthPipe {
	private:
		const wchar_t* PipeNameTemplate = L"Local\\Shakra%sMem%u";
		ErrorSystem::WinErr SynthErr;
		int EvBufSize = 4096;
		HANDLE* PDrvEvBufHeads = nullptr;
		HANDLE* PDrvEvBuf = nullptr;

		// EvBuf
		EvBufHeads** DrvEvBufHeads = nullptr;
		DWORD** DrvEvBuf;
		bool PrepareArrays();

	public:
		bool CreatePipe(unsigned short PipeID, int Size);
		bool OpenPipe(unsigned short PipeID);
		bool ClosePipe(unsigned short PipeID);
		bool PerformBufferCheck(unsigned short PipeID);
		void ResetReadHeadIfNeeded(unsigned short PipeID);
		int GetReadHeadPos(unsigned short PipeID);
		int GetWriteHeadPos(unsigned short PipeID);
		unsigned int ParseShortEvent(unsigned short PipeID);
		void SaveShortEvent(unsigned short PipeID, unsigned int Event);
		void SaveLongEvent(unsigned short PipeID, LPMIDIHDR Event);
		unsigned int PrepareLongEvent(LPMIDIHDR Event);
		unsigned int UnprepareLongEvent(LPMIDIHDR Event);
	};
}

#endif