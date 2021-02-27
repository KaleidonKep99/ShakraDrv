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
#include <vector>

#include "WinDriver.hpp"
#include "WinError.hpp"

// BASS
#include "inc/win32/bass.h"
#include "inc/win32/bassmidi.h"

#define BERROR(x, y)		BASSErr.ThrowError(x, FU, FI, LI, y)
#define BFERROR(x)			BASSErr.ThrowFatalError(x)
#define BLOG(x)				BASSErr.Log(x, FU, FI, LI)

#define LOADLIBFUNCTION(l, f) *((void**)&f)=GetProcAddress(l,#f)

#elif __linux__

// stub

#elif __APPLE__

// stub

#endif

namespace MIDISynth {
	class BASS {
	private:
		wchar_t* CustomDir = nullptr;
		int OutputID = 0;
		int AudioFreq = 0;
		int StreamAddress = 0;
		int InitFlags = NULL;
		int StrmFlags = BASS_SAMPLE_FLOAT;
		bool _Fail = false;
		vector<HSOUNDFONT> SoundFontHandles;
		vector<BASS_MIDI_FONTEX> SoundFontPresets;

#ifdef _WIN32
		WinDriver::LibLoader LibLoader;
		WinDriver::WinLib BLib;
		WinDriver::WinLib BMLib;
#elif __linux__
		// stub
#elif __APPLE__
		// stub
#endif

		ErrorSystem::WinErr BASSErr;

		bool LoadLib() {
#ifdef _WIN32

			// Load BASS
			if (LibLoader.LoadLib(&BLib, (wchar_t*)L"bass.dll", CustomDir))
			{
				// Load funcs from BASS
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelFlags);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelGetAttribute);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelGetData);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelGetLevelEx);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelIsActive);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelPlay);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelRemoveFX);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelSeconds2Bytes);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelSeconds2Bytes);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelSetAttribute);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelSetDevice);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelSetFX);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelSetSync);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelStop);
				LOADLIBFUNCTION(BLib.Lib, BASS_ChannelUpdate);
				LOADLIBFUNCTION(BLib.Lib, BASS_ErrorGetCode);
				LOADLIBFUNCTION(BLib.Lib, BASS_Free);
				LOADLIBFUNCTION(BLib.Lib, BASS_FXSetParameters);
				LOADLIBFUNCTION(BLib.Lib, BASS_GetDevice);
				LOADLIBFUNCTION(BLib.Lib, BASS_GetDeviceInfo);
				LOADLIBFUNCTION(BLib.Lib, BASS_GetInfo);
				LOADLIBFUNCTION(BLib.Lib, BASS_Init);
				LOADLIBFUNCTION(BLib.Lib, BASS_IsStarted);
				LOADLIBFUNCTION(BLib.Lib, BASS_PluginFree);
				LOADLIBFUNCTION(BLib.Lib, BASS_PluginLoad);
				LOADLIBFUNCTION(BLib.Lib, BASS_SetConfig);
				LOADLIBFUNCTION(BLib.Lib, BASS_SetDevice);
				LOADLIBFUNCTION(BLib.Lib, BASS_SetVolume);
				LOADLIBFUNCTION(BLib.Lib, BASS_Stop);
				LOADLIBFUNCTION(BLib.Lib, BASS_StreamFree);
				LOADLIBFUNCTION(BLib.Lib, BASS_Update);

				if (!BASS_Init)
				{
					BLOG(L"Something's wrong! BASS_Init has no address.");
					return false;
				}

				BLOG(L"BASS is ready.");
			}

			// Load BASSMIDI
			if (LibLoader.LoadLib(&BMLib, (wchar_t*)L"bassmidi.dll", CustomDir))
			{
				// Load funcs from BASSMIDI
				LOADLIBFUNCTION(BMLib.Lib, BASS_MIDI_StreamCreate);
				LOADLIBFUNCTION(BMLib.Lib, BASS_MIDI_StreamEvent);
				LOADLIBFUNCTION(BMLib.Lib, BASS_MIDI_StreamEvents);
				LOADLIBFUNCTION(BMLib.Lib, BASS_MIDI_StreamLoadSamples);
				LOADLIBFUNCTION(BMLib.Lib, BASS_MIDI_StreamSetFonts);
				LOADLIBFUNCTION(BMLib.Lib, BASS_MIDI_FontInit);
				LOADLIBFUNCTION(BMLib.Lib, BASS_MIDI_FontLoad);
				LOADLIBFUNCTION(BMLib.Lib, BASS_MIDI_FontUnload);

				if (!BASS_MIDI_StreamCreate)
				{
					BLOG(L"Something's wrong! BASS_MIDI_StreamCreate has no address.");
					return false;
				}

				BLOG(L"BASSMIDI is ready.");
			}

			return true;

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

			return (LibLoader.FreeLib(&BLib) && LibLoader.FreeLib(&BMLib));

#elif __linux__

			// stub

#elif __APPLE__

			// stub

#endif
		}

	public:
		// Init
		BASS(int OutputIdentifier, int AudioFrequency, int InitFlags, int StrmFlags) {
			Init(OutputIdentifier, L"Default", AudioFrequency, InitFlags, StrmFlags);
		}

		BASS(int AudioFrequency, int InitFlags, int StrmFlags) {
			Init(-1, L"Default", AudioFrequency, InitFlags, StrmFlags);
		}

		BASS() {
			// Nothingness
		}

		~BASS() {
			// Nothingness
		}

		void Init(int OutputIdentifier, const wchar_t* AudioEngine, int AudioFrequency, int InitFlags, int StrmFlags) {
			if (LoadLib()) {
				if (this->OutputID == 0 && this->AudioFreq == 0)
					BLOG(L"No audio frequency specified with NULL output. Defaulting to 44.1kHz.");

				this->OutputID = OutputIdentifier;
				this->AudioFreq = AudioFrequency ? AudioFrequency : 44100;
				this->InitFlags = InitFlags;
				this->StrmFlags = StrmFlags;

				if (_wcsicmp(AudioEngine, L"DirectSound")) 
				{
					this->InitFlags |= BASS_DEVICE_DSOUND;
				}
				else if (_wcsicmp(AudioEngine, L"Steinberg ASIO")) 
				{

				}
				else 
				{
					
				}

				if (!BASS_Init(OutputID, AudioFreq, InitFlags, 0, 0)) {
					BERROR(L"An error has occured during the initialization of BASS.", false);
					this->_Fail = true;
					return;
				}

				if (!(StreamAddress = BASS_MIDI_StreamCreate(16, StrmFlags, AudioFreq)))
				{
					BERROR(L"An error has occured during the initialization of the MIDI stream.", false);
					BASS_Free();
					this->_Fail = true;
					return;
				}

				BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 2);
				BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 5);
				BASS_ChannelSetAttribute(StreamAddress, BASS_ATTRIB_MIDI_VOICES, 1024);

				HSOUNDFONT SF = BASS_MIDI_FontInit(L"test.sf2", BASS_UNICODE | 0x400000);
				BASS_MIDI_FONTEX SFConf = { SF, -1, -1, -1, 0, 0 };

				SoundFontHandles.push_back(SF);
				SoundFontPresets.push_back(SFConf);

				BASS_MIDI_StreamSetFonts(StreamAddress, &SoundFontPresets[0], (int)SoundFontPresets.size() | BASS_MIDI_FONT_EX);

				BASS_ChannelPlay(StreamAddress, false);

				BLOG(L"BASS is ready for incoming streams.");
			}
			else {
				BERROR(L"Shakra failed to load the required BASS libraries.", false);
				this->_Fail = true;
			}
		}

		void Free() {
			if (!BLib.Lib && !BMLib.Lib) {
				BLOG(L"BASS isn't loaded, the app probably called ::Free() by accident, ignore.");
				return;
			}

			if (BLib.Lib) {
				BASS_StreamFree(StreamAddress);

				if (SoundFontHandles.size()) {
					for (auto it = SoundFontHandles.begin(); it != SoundFontHandles.end(); ++it) {
						BASS_MIDI_FontFree(*it);
						BLOG(L"Freed SoundFont...");
					}
					SoundFontHandles.resize(0);
					SoundFontPresets.resize(0);
					BLOG(L"SoundFont vectors resized to zero.");
				}

				if (BASS_Free()) {
					BERROR(L"BASS encountered an error while trying to free its resources.", false);
					this->_Fail = true;
					return;
				}

				BLOG(L"BASS has been shutdown and is ready to be freed.");
			}

			FreeLib();
		}

		void PlayEvent(int Event) {
			int Length = 3;

			switch (Event & 0xF0) {
			case 0x90:
				BASS_MIDI_StreamEvent(StreamAddress, Event & 0xF, MIDI_EVENT_NOTE, Event >> 8);
				return;
			case 0x80:
				BASS_MIDI_StreamEvent(StreamAddress, Event & 0xF, MIDI_EVENT_NOTE, (BYTE)(Event >> 8));
				return;
			default:
				if (!(Event - 0x80 & 0xC0))
				{
					BASS_MIDI_StreamEvents(StreamAddress, BASS_MIDI_EVENTS_RAW, &Event, Length);
					return;
				}

				if (!((Event - 0xC0) & 0xE0)) 
					Length = 2;

				else if ((Event & 0xF0) == 0xF0)
				{
					switch (Event & 0xF)
					{
					case 3:
						Length = 2;
						break;
					default:
						return;
					}
				}

				BASS_MIDI_StreamEvents(StreamAddress, BASS_MIDI_EVENTS_RAW, &Event, Length);
				return;
			}
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

			if (!this->CustomDir) {
				this->CustomDir = new (std::nothrow) wchar_t[DirectoryBuf];

				if (this->CustomDir)
				{
					wcscpy_s(this->CustomDir, DirectoryBuf, Directory);

					BLOG(L"Custom dir stored successfully into memory.");
					return true;
				}
			}

			BERROR(L"The calloc() function failed to allocate memory for the custom dir.", false);
			return false;
		}

		// Only used by apps that are bundled with the driver
		bool Fail() const { return _Fail; }
	};
}

#endif