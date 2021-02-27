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
#include <new>
#include <cstdio>

namespace WinDriver {
	class SynthPipe {
	private:
		const wchar_t* PipeNameTemplate = L"\\\\.\\pipe\\ShakraDataPipe%u";
		ErrorSystem::WinErr SynthErr;
		HANDLE* DrvPipes = nullptr;

	public:
		bool OpenPipe(unsigned short PipeID);
		bool ClosePipe(unsigned short PipeID);
		void ParseShortEvent(unsigned short PipeID, unsigned int Event);
		void ParseLongEvent(unsigned short PipeID, LPMIDIHDR Event);
		unsigned int PrepareLongEvent(LPMIDIHDR Event);
		unsigned int UnprepareLongEvent(LPMIDIHDR Event);
	};
}

#endif