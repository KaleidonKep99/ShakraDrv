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

#define LOADLIBFUNCTION(l, f) *((void**)&f)=GetProcAddress(l,#f)

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

	bool LoadLib() {
#ifdef _WIN32

		wchar_t LibName[10] = L"\\bass.dll";
		bool Success = LibLoader.LoadLib(&BASSLib, LibName, CustomDir);
		if (Success && BASSLib != nullptr) {
			LOADLIBFUNCTION(BASSLib, BASS_ChannelFlags);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelGetAttribute);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelGetData);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelGetLevelEx);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelIsActive);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelPlay);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelRemoveFX);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelSeconds2Bytes);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelSeconds2Bytes);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelSetAttribute);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelSetDevice);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelSetFX);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelSetSync);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelStop);
			LOADLIBFUNCTION(BASSLib, BASS_ChannelUpdate);
			LOADLIBFUNCTION(BASSLib, BASS_ErrorGetCode);
			LOADLIBFUNCTION(BASSLib, BASS_FXSetParameters);
			LOADLIBFUNCTION(BASSLib, BASS_Free);
			LOADLIBFUNCTION(BASSLib, BASS_GetDevice);
			LOADLIBFUNCTION(BASSLib, BASS_GetDeviceInfo);
			LOADLIBFUNCTION(BASSLib, BASS_GetInfo);
			LOADLIBFUNCTION(BASSLib, BASS_IsStarted);
			LOADLIBFUNCTION(BASSLib, BASS_Init);
			LOADLIBFUNCTION(BASSLib, BASS_PluginFree);
			LOADLIBFUNCTION(BASSLib, BASS_PluginLoad);
			LOADLIBFUNCTION(BASSLib, BASS_SetConfig);
			LOADLIBFUNCTION(BASSLib, BASS_SetDevice);
			LOADLIBFUNCTION(BASSLib, BASS_SetVolume);
			LOADLIBFUNCTION(BASSLib, BASS_Stop);
			LOADLIBFUNCTION(BASSLib, BASS_StreamFree);
			LOADLIBFUNCTION(BASSLib, BASS_Update);

			if (BASS_Init)
				BLOG(L"Something's wrong! BASS_Init has no address.");

			BLOG(L"BASS is ready.");
			return true;
		}

		return false;

#elif __linux__

		// stub

#elif __APPLE__

		// stub

#endif
	}

	bool FreeLib() {
#ifdef _WIN32

		if (BASS_IsStarted()) {
			BERROR(L"You can't unload BASS while it's active!", false);
			return false;
		}

		return LibLoader.FreeLib(&BASSLib);

#elif __linux__

		// stub

#elif __APPLE__

		// stub

#endif
	}

public:
	// Init
	BASS(int OutputIdentifier, int AudioFrequency, int Flags) {
		Init(OutputIdentifier, AudioFrequency, Flags);
	}

	BASS(int AudioFrequency, int Flags) {
		Init(-1, AudioFrequency, Flags);
	}

	BASS() {
		// Nothingness
	}
	~BASS() {
		// Nothingness
	}

	void Init(int OutputIdentifier, int AudioFrequency, int Flags) {
		if (LoadLib()) {
			if (this->OutputID == 0 && this->AudioFreq == 0)
				BLOG(L"No audio frequency specified with NULL output. Defaulting to 44.1kHz.");

			this->OutputID = OutputIdentifier;
			this->AudioFreq = AudioFrequency ? AudioFrequency : 44100;
			this->Flgs = Flags;

			if (!BASS_Init(OutputID, AudioFreq, Flgs, 0, 0)) {
				BERROR(L"An error has occured during the initialization of BASS.", false);
				this->_Fail = true;
				return;
			}

			BLOG(L"BASS is ready for incoming streams.");
		}
		else {
			BERROR(L"Shakra failed to load BASS.DLL.", false);
			this->_Fail = true;
		}
	}

	void Free() {
		if (!BASSLib) {
			if (BASS_Free()) {
				BERROR(L"BASS encountered an error while trying to free its resources.", false);
				this->_Fail = true;
				return;
			}

			FreeLib();
			BLOG(L"BASS has been shutdown and is ready to be freed.");
			return;
		}

		BLOG(L"BASS isn't loaded, the app probably sent a midiOutClose/midiStreamClose message by accident, ignore.");
	}

	// Only used by apps that are bundled with the driver
	bool SetCustomLoadDir(wchar_t* Directory, size_t DirectoryBuf) {
		if (Directory == nullptr) {
			BERROR(L"No pointer has been passed to the function", false);
			return false;
		}

		if (DirectoryBuf == 0) {
			BERROR(L"No size has been passed to the function", false);
			return false;
		}

		if (wcslen(this->CustomDir) != 0) {
			BERROR(L"There's already a dir stored in memory, it will be overwrited...", false);
			free(this->CustomDir);
		}

		if (this->CustomDir != nullptr) {
			memset(this->CustomDir, 0, sizeof(wchar_t) * MAX_PATH);
			wcscpy_s(this->CustomDir, _countof(this->CustomDir), Directory);

			BLOG(L"Custom dir stored successfully into memory.");
			return true;
		}

		BERROR(L"The calloc() function failed to allocate memory for the custom dir.", false);
		return false;
	}

	// Only used by apps that are bundled with the driver
	bool Fail() const { return _Fail; }
};

#endif