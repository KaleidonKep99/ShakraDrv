/*
Shakra Driver component for Windows
This .h file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#pragma once

#ifndef WINDRIVER_H
#define WINDRIVER_H

#include <Windows.h>
#include <mmddk.h>
#include <assert.h>
#include <tchar.h>
#include <string>
#include "WinError.h"

using namespace std;

namespace WinDriver {
	typedef VOID(CALLBACK* WMMC)(HMIDIOUT, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR);

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
		unsigned long GiveCaps(void*, DWORD);
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
		// Opening and closing the driver
		bool SetDriverHandle(HDRVR);
		bool UnsetDriverHandle();

		// Setting the driver's pointer for the app
		bool OpenDriver(MIDIOPENDESC*, DWORD, DWORD_PTR);
		bool CloseDriver();

		// Callbacks
		void CallbackFunction(DWORD, DWORD, DWORD);
	};

	class Blacklist {
	private:

	};
}

#endif