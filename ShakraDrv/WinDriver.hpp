/*
Shakra Driver component for Windows
This .h file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#pragma once

#ifndef WINDRIVER_H

#define WINDRIVER_H

#include <Windows.h>
#include <ShlObj_core.h>
#include <mmddk.h>
#include <assert.h>
#include <tchar.h>
#include <string>
#include "WinError.hpp"

// Debug, don't touch
#define S2(x)	#x						// Convert to string
#define S1(x)	S2(x)					// Convert to string
#define FU		_T(__FUNCTION__)		// Function
#define LI		_T(S1(__LINE__))		// Line
#define FI		_T(__FILE__)			// File

// Debug
#define DERROR(x, y, z)			x.ThrowError(y, FU, FI, LI, z)
#define DFERROR(x, y)			x.ThrowFatalError(y)
#define DLOG(x, y)				x.Log(y, FU, FI, LI)

using namespace std;

namespace WinDriver {
	typedef VOID(CALLBACK* WMMC)(HMIDIOUT, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR);

	class LibLoader {
	private:
		ErrorSystem LibErr;

	public:
		bool LoadLib(HMODULE*, wchar_t*, wchar_t*);
		bool FreeLib(HMODULE*);
	};

	class DriverMask {
	private:
		wstring Name = L"Shakra Driver\0";

		unsigned short ManufacturerID = 0xFFFF;
		unsigned short ProductID = 0xFFFF;
		unsigned short Technology = MOD_SWSYNTH;
		unsigned short Support = MIDICAPS_VOLUME;

		ErrorSystem MaskErr;

	public:
		// Change settings
		bool ChangeName(const wchar_t*);
		bool ChangeSettings(short, short, short, short);
		unsigned long GiveCaps(PVOID, DWORD);
	};

	class DriverComponent {
	private:
		HMIDI WMMHandle = nullptr;
		HDRVR DrvHandle = nullptr;

		DWORD CallbackMode = 0;
		DWORD_PTR Callback = 0;
		DWORD_PTR Instance = 0;

		ErrorSystem DrvErr;
		DriverMask MaskSystem;

	public:
		HMODULE LibHandle = nullptr;

		// Opening and closing the driver
		bool SetDriverHandle(HDRVR);
		bool UnsetDriverHandle();

		// Setting the driver's pointer for the app
		bool OpenDriver(MIDIOPENDESC*, DWORD, DWORD_PTR);
		bool CloseDriver();

		// Callbacks
		void CallbackFunction(DWORD, DWORD_PTR, DWORD_PTR);
	};

	class Blacklist {
	private:

	};
}

#endif