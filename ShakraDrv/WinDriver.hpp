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
#include "WinError.hpp"

using namespace std;

namespace WinDriver {
	typedef VOID(CALLBACK* WMMC)(HMIDIOUT, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR);

	class WinLib
	{
	public:
		HMODULE Lib = nullptr;
		bool AppOwnDLL = false;
	};

	class LibLoader {
	private:
		ErrorSystem::WinErr LibErr;

	public:
		bool LoadLib(WinLib*, wchar_t*, wchar_t*);
		bool FreeLib(WinLib*);
	};

	class DriverMask {
	private:
		wstring Name = L"Shakra Driver\0";

		unsigned short ManufacturerID = 0xFFFF;
		unsigned short ProductID = 0xFFFF;
		unsigned short Technology = MOD_SWSYNTH;
		unsigned short Support = MIDICAPS_VOLUME;

		ErrorSystem::WinErr MaskErr;

	public:
		// Change settings
		bool ChangeName(const wchar_t*);
		bool ChangeSettings(short, short, short, short);
		unsigned long GiveCaps(PVOID, DWORD);
	};

	class DriverCallback {

	private:
		HMIDI WMMHandle = nullptr;
		DWORD CallbackMode = 0;
		DWORD_PTR Callback = 0;
		DWORD_PTR Instance = 0;

		ErrorSystem::WinErr CallbackErr;

	public:
		// Callbacks
		bool PrepareCallbackFunction(MIDIOPENDESC*);
		bool ClearCallbackFunction();
		void CallbackFunction(DWORD, DWORD, DWORD);

	};

	class DriverComponent {

	private:
		HDRVR DrvHandle = nullptr;
		HMODULE LibHandle = nullptr;

		ErrorSystem::WinErr DrvErr;

	public:

		// Opening and closing the driver
		bool SetDriverHandle(HDRVR);
		bool UnsetDriverHandle();

		// Setting the driver's pointer for the app
		bool OpenDriver(MIDIOPENDESC*, DWORD, DWORD_PTR);
		bool CloseDriver();

	};

	class Blacklist {
	private:

	};
}

#endif