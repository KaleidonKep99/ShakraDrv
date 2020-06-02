/*
Shakra Driver component for Windows
This .cpp file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#ifdef _WIN32

#include "WinError.h"

namespace WinDriver {
	void ErrorSystem::Warn(const wchar_t* Position, const wchar_t* Message) {
		wchar_t* Buf;

		Buf = (wchar_t*)malloc(sizeof(wchar_t) * 2048);
		swprintf_s(Buf, sizeof(Buf), L"%s: %s", Position, Message);

		OutputDebugString(Buf);

		free(Buf);
	}

	void ErrorSystem::ThrowError(const wchar_t* Position, const wchar_t* Error, bool IsSeriousError) {
		wchar_t* Buf;

		Buf = (wchar_t*)malloc(sizeof(wchar_t) * 2048);
		swprintf_s(Buf, sizeof(Buf), L"An error has occured at \"%s\"!\n\nError: %s", Position, Error);

		MessageBox(NULL, Buf, L"Shakra - Error", IsSeriousError ? MB_ICONERROR : MB_ICONWARNING | MB_OK | MB_SYSTEMMODAL);

		free(Buf);
	}

	void ErrorSystem::ThrowFatalError(const wchar_t* Error) {
		wchar_t* Buf;

		Buf = (wchar_t*)malloc(sizeof(wchar_t) * 2048);
		swprintf_s(Buf, sizeof(Buf), L"A fatal error has occured from which the driver is unable to recover!\n\nError: %s", Error);

		MessageBox(NULL, Buf, L"Shakra - FATAL ERROR", MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);

		free(Buf);

		throw (int)(-1);
	}
}

#endif