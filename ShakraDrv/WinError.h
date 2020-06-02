/*
Shakra Driver component for Windows
This .h file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#pragma once

#include <Windows.h>
#include <string>

using namespace std;

namespace WinDriver {
	class ErrorSystem {
	public:
		void Warn(const wchar_t*, const wchar_t*);
		void ThrowError(const wchar_t*, const wchar_t*, bool);
		void ThrowFatalError(const wchar_t*);
	};
}