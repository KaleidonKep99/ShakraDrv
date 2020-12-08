/*
Shakra Driver component
This .cpp file contains the required code to run the driver.

This file is required for both Windows and Linux.
*/

#pragma once

#ifndef BASS_HPP
#define BASS_HPP

#ifdef _WIN32

#define MAXDIRLEN		MAX_PATH
#define BASSDEF(f)		(WINAPI *f)
#define BASSMIDIDEF(f)	(WINAPI *f)

#include <Windows.h>
#include <tchar.h>
#include "inc/win32/bass.h"
#include "inc/win32/bassmidi.h"
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
	int StreamAddress = 0;
	int Flgs = NULL;
	bool _Fail = false;

#ifdef _WIN32
	WinDriver::LibLoader LibLoader;
	HMODULE BLib = nullptr;
	HMODULE BMLib = nullptr;
#elif __linux__
	// stub
#elif __APPLE__
	// stub
#endif

	ErrorSystem BASSErr;

	bool LoadLib() {
#ifdef _WIN32

		bool Success = LibLoader.LoadLib(&BLib, (wchar_t*)L"bass.dll", CustomDir);
		if (LibLoader.LoadLib(&BLib, (wchar_t*)L"bass.dll", CustomDir) && 
			LibLoader.LoadLib(&BMLib, (wchar_t*)L"bassmidi.dll", CustomDir)) {

			// Load funcs from BASS
			LOADLIBFUNCTION(BLib, BASS_ChannelFlags);
			LOADLIBFUNCTION(BLib, BASS_ChannelGetAttribute);
			LOADLIBFUNCTION(BLib, BASS_ChannelGetData);
			LOADLIBFUNCTION(BLib, BASS_ChannelGetLevelEx);
			LOADLIBFUNCTION(BLib, BASS_ChannelIsActive);
			LOADLIBFUNCTION(BLib, BASS_ChannelPlay);
			LOADLIBFUNCTION(BLib, BASS_ChannelRemoveFX);
			LOADLIBFUNCTION(BLib, BASS_ChannelSeconds2Bytes);
			LOADLIBFUNCTION(BLib, BASS_ChannelSeconds2Bytes);
			LOADLIBFUNCTION(BLib, BASS_ChannelSetAttribute);
			LOADLIBFUNCTION(BLib, BASS_ChannelSetDevice);
			LOADLIBFUNCTION(BLib, BASS_ChannelSetFX);
			LOADLIBFUNCTION(BLib, BASS_ChannelSetSync);
			LOADLIBFUNCTION(BLib, BASS_ChannelStop);
			LOADLIBFUNCTION(BLib, BASS_ChannelUpdate);
			LOADLIBFUNCTION(BLib, BASS_ErrorGetCode);
			LOADLIBFUNCTION(BLib, BASS_Free);
			LOADLIBFUNCTION(BLib, BASS_FXSetParameters);
			LOADLIBFUNCTION(BLib, BASS_GetDevice);
			LOADLIBFUNCTION(BLib, BASS_GetDeviceInfo);
			LOADLIBFUNCTION(BLib, BASS_GetInfo);
			LOADLIBFUNCTION(BLib, BASS_Init);
			LOADLIBFUNCTION(BLib, BASS_IsStarted);
			LOADLIBFUNCTION(BLib, BASS_PluginFree);
			LOADLIBFUNCTION(BLib, BASS_PluginLoad);
			LOADLIBFUNCTION(BLib, BASS_SetConfig);
			LOADLIBFUNCTION(BLib, BASS_SetDevice);
			LOADLIBFUNCTION(BLib, BASS_SetVolume);
			LOADLIBFUNCTION(BLib, BASS_Stop);
			LOADLIBFUNCTION(BLib, BASS_StreamFree);
			LOADLIBFUNCTION(BLib, BASS_Update);

			// Load funcs from BASSMIDI
			LOADLIBFUNCTION(BMLib, BASS_MIDI_StreamCreate);
			LOADLIBFUNCTION(BMLib, BASS_MIDI_StreamEvent);
			LOADLIBFUNCTION(BMLib, BASS_MIDI_StreamEvents);
			LOADLIBFUNCTION(BMLib, BASS_MIDI_StreamLoadSamples);
			LOADLIBFUNCTION(BMLib, BASS_MIDI_StreamSetFonts);
			LOADLIBFUNCTION(BMLib, BASS_MIDI_StreamCreate);
			LOADLIBFUNCTION(BMLib, BASS_MIDI_StreamCreate);
			LOADLIBFUNCTION(BMLib, BASS_MIDI_StreamCreate);

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

		return LibLoader.FreeLib(&BLib);

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
			BERROR(L"Shakra failed to load the required BASS libraries.", false);
			this->_Fail = true;
		}
	}

	void Free() {
		if (!BLib && !BMLib) {
			BLOG(L"BASS isn't loaded, the app probably called ::Free() by accident, ignore.");
			return;
		}

		if (!BLib) {
			if (BASS_Free()) {
				BERROR(L"BASS encountered an error while trying to free its resources.", false);
				this->_Fail = true;
				return;
			}

			BLOG(L"BASS has been shutdown and is ready to be freed.");
		}

		FreeLib();
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
			BERROR(L"There's already a dir stored in memory, it will be overwritten...", false);
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