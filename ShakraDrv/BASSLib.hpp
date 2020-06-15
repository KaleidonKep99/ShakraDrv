/*
Shakra Driver component
This .cpp file contains the required code to run the driver.

This file is required for both Windows and Linux.
*/

#pragma once

#ifndef BASS_HPP
#define BASS_HPP

#ifdef _WIN32

#define MAXDIRLEN	MAX_PATH
#define BASSDEF(f)	(WINAPI *f)

#include <Windows.h>
#include <tchar.h>
#include "inc/win32/bass.h"
#include "WinDriver.hpp"
#include "WinError.hpp"

#define BERROR(x, y)		BASSErr.ThrowError(x, FU, FI, LI, y)
#define BFERROR(x)			BASSErr.ThrowFatalError(x)
#define BLOG(x)				BASSErr.Log(x, FU, FI, LI)

#elif __linux__

// stub

#elif __APPLE__

// stub

#endif

class BASS {
private:
	wchar_t CustomDir[MAXDIRLEN] = { 0 };
	int OutputID = 0;
	int AudioFreq = 0;
	int Flgs = NULL;
	bool _Fail = false;

#ifdef _WIN32
	WinDriver::LibLoader LibLoader;
	HMODULE BASSLib = nullptr;
#elif __linux__
	// stub
#elif __APPLE__
	// stub
#endif

	ErrorSystem BASSErr;

	bool LoadLib();
	bool FreeLib();

public:
	// Only used by apps that are bundled with the driver
	bool Fail() const { return _Fail; }

	// Only used by apps that are bundled with the driver
	bool SetCustomLoadDir(wchar_t*, size_t);

	// Init
	BASS();
	BASS(int, int);
	BASS(int, int, int);
	~BASS();
};

#endif