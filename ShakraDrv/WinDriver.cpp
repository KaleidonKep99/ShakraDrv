/*
Shakra Driver component for Windows
This .cpp file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#ifdef _WIN32

#include "WinDriver.hpp"

namespace WinDriver {
	bool LibLoader::LoadLib(HMODULE* Target, wchar_t* Lib, wchar_t* CustomDir) {
		wchar_t FPath[MAX_PATH] = { 0 };

		if ((*Target) != nullptr)
		{
			DLOG(LibErr, L"BASS has already been loaded through another function. All good.");
			return true;
		}

		if (!((*Target) = LoadLibrary(Lib))) {
			DLOG(LibErr, L"No BASS lib found in memory. We'll load our own.");
	
			if (CustomDir != nullptr) {
				DLOG(LibErr, L"A custom dir has been specified, we'll use it to load BASS.");
				wcscat_s(FPath, MAX_PATH, CustomDir);
			}
			else {
				wchar_t* SPath;
				HRESULT SHR = SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_NO_ALIAS, NULL, &SPath);

				if (!SUCCEEDED(SHR)) {
					DERROR(LibErr, L"Failed to get system directory.", false);
					return false;
				}

				wcscat_s(FPath, MAX_PATH, SPath);
				wcscat_s(FPath, MAX_PATH, L"\\Shakra");
			}

			wcscat_s(FPath, MAX_PATH, Lib);
			if (!(*Target = LoadLibraryEx(FPath, NULL, 0))) {
				DERROR(LibErr, L"Failed to load BASS into memory.", false);
				return false;
			}
		}

		return true;
	}

	bool LibLoader::FreeLib(HMODULE* Target) {
		if ((*Target) != nullptr) {
			if (!FreeLibrary(*Target)) {
				DFERROR(LibErr, L"The driver failed to unload a library.\n\nThis is not supposed to happen!");
				return false;
			}
		}
		else DLOG(LibErr, L"The lib isn't loaded.");

		return true;
	}

	bool DriverMask::ChangeName(const wchar_t* NewName) {
		if (NewName != nullptr) {
			if (wcslen(NewName) < 31)
			{
				this->Name = NewName;
				return true;
			}

			DERROR(MaskErr, L"The name is too long!", false);
		}
		else DERROR(MaskErr, L"Something went wrong while trying to access the new name. Memory corruption?", true);

		return false;
	}

	bool DriverMask::ChangeSettings(short NMID, short NPID, short NT, short NS) {
		// TODO: Check if the values contain valid technologies/support flags

		this->ManufacturerID = NMID;
		this->ProductID = NPID;
		this->Technology = NT;
		this->Support = NS;

		return true;
	}

	unsigned long DriverMask::GiveCaps(PVOID CapsPointer, DWORD CapsSize) {
		MIDIOUTCAPSA CapsA;
		MIDIOUTCAPSW CapsW;
		MIDIOUTCAPS2A Caps2A;
		MIDIOUTCAPS2W Caps2W;
		size_t WCSTSRetVal;

		// Why would this happen? Stupid MIDI app dev smh
		if (CapsPointer == nullptr)
		{
			DERROR(MaskErr, L"A null pointer has been passed to the function. The driver can't share its info with the application.", false);
			return MMSYSERR_INVALPARAM;
		}

		if (CapsSize == 0) {
			DERROR(MaskErr, L"CapsSize has a value of 0, how is the driver supposed to determine the subtype of the struct?", false);
			return MMSYSERR_INVALPARAM;
		}

		// I have to support all this s**t or else it won't work in some apps smh
		switch (CapsSize) {
		case (sizeof(MIDIOUTCAPSA)):
			wcstombs_s(&WCSTSRetVal, CapsA.szPname, sizeof(CapsA.szPname), this->Name.c_str(), MAXPNAMELEN);
			CapsA.dwSupport = this->Support;
			CapsA.wChannelMask = 0xFFFF;
			CapsA.wMid = this->ManufacturerID;
			CapsA.wPid = this->ProductID;
			CapsA.wTechnology = this->Technology;
			CapsA.wNotes = 65535;
			CapsA.wVoices = 65535;
			CapsA.vDriverVersion = MAKEWORD(6, 2);
			memcpy((LPMIDIOUTCAPSA)CapsPointer, &CapsA, min(CapsSize, sizeof(CapsA)));
			break;

		case (sizeof(MIDIOUTCAPSW)):
			wcsncpy_s(CapsW.szPname, this->Name.c_str(), MAXPNAMELEN);
			CapsW.dwSupport = this->Support;
			CapsW.wChannelMask = 0xFFFF;
			CapsW.wMid = this->ManufacturerID;
			CapsW.wPid = this->ProductID;
			CapsW.wTechnology = this->Technology;
			CapsW.wNotes = 65535;
			CapsW.wVoices = 65535;
			CapsW.vDriverVersion = MAKEWORD(6, 2);
			memcpy((LPMIDIOUTCAPSW)CapsPointer, &CapsW, min(CapsSize, sizeof(CapsW)));
			break;

		case (sizeof(MIDIOUTCAPS2A)):
			wcstombs_s(&WCSTSRetVal, Caps2A.szPname, sizeof(Caps2A.szPname), this->Name.c_str(), MAXPNAMELEN);
			Caps2A.dwSupport = this->Support;
			Caps2A.wChannelMask = 0xFFFF;
			Caps2A.wMid = this->ManufacturerID;
			Caps2A.wPid = this->ProductID;
			Caps2A.wTechnology = this->Technology;
			Caps2A.wNotes = 65535;
			Caps2A.wVoices = 65535;
			Caps2A.vDriverVersion = MAKEWORD(6, 2);
			memcpy((LPMIDIOUTCAPS2A)CapsPointer, &Caps2A, min(CapsSize, sizeof(Caps2A)));
			break;

		case (sizeof(MIDIOUTCAPS2W)):
			wcsncpy_s(Caps2W.szPname, this->Name.c_str(), MAXPNAMELEN);
			Caps2W.dwSupport = this->Support;
			Caps2W.wChannelMask = 0xFFFF;
			Caps2W.wMid = this->ManufacturerID;
			Caps2W.wPid = this->ProductID;
			Caps2W.wTechnology = this->Technology;
			Caps2W.wNotes = 65535;
			Caps2W.wVoices = 65535;
			Caps2W.vDriverVersion = MAKEWORD(6, 2);
			memcpy((LPMIDIOUTCAPS2W)CapsPointer, &Caps2W, min(CapsSize, sizeof(Caps2W)));
			break;
		}

		DLOG(MaskErr, L"Caps have been shared with the app.");
		return MMSYSERR_NOERROR;
	}

	void DriverComponent::CallbackFunction(DWORD Message, DWORD Arg1, DWORD Arg2) {
		WMMC Callback = nullptr;
		int ReturnMessage = 0;

		switch (this->CallbackMode & CALLBACK_TYPEMASK) {

		case CALLBACK_FUNCTION:	// Use a custom function to notify the app
			// Prepare the custom callback
			Callback = (*(WMMC)(this->Callback));

			// It doesn't exist! Not good!
			if (Callback == nullptr)
			{
				DFERROR(DrvErr, L"The callback function became not valid. This is dangerous and shouldn't happen.");
				return;
			}

			// It's alive, use the app's custom function to send the callback
			Callback((HMIDIOUT)(this->WMMHandle), Message, this->Instance, Arg1, Arg2);
			break;

		case CALLBACK_EVENT:	// Set an event to notify the app
			ReturnMessage = SetEvent((HANDLE)(this->Callback));
			break;

		case CALLBACK_THREAD:	// Send a message to a thread to notify the app
			ReturnMessage = PostThreadMessage(this->Callback, Message, Arg1, Arg2);
			break;

		case CALLBACK_WINDOW:	// Send a message to the app's main window
			ReturnMessage = PostMessage((HWND)(this->Callback), Message, Arg1, Arg2);
			break;

		default:				// stub
			break;

		}
	}

	bool DriverComponent::SetDriverHandle(HDRVR Handle) {
		// The app tried to initialize the driver with no pointer?
		if (Handle == nullptr) {
			DERROR(DrvErr, L"A null pointer has been passed to the function.", true);
			return false;
		}

		// We already have the same pointer in memory.
		if (this->DrvHandle == Handle) {
			DLOG(DrvErr, L"We already have the handle stored in memory. The app has Alzheimer I guess?");
			return true;
		}

		// A pointer is already stored in the variable, UnSetDriverHandle hasn't been called
		if (this->DrvHandle != nullptr) {
			DERROR(DrvErr, L"DrvHandle has been set in a previous call and not freed.", false);
			return false;
		}

		// All good, save the pointer to a local variable and return true
		this->DrvHandle = Handle;
		return true;
	}

	bool DriverComponent::UnsetDriverHandle() {
		// Warn through stdout if the app is trying to free the driver twice
		if (this->DrvHandle == nullptr)
			DLOG(DrvErr, L"The application called UnsetDriverHandle even though there's no handle set. Bad design?");

		// Free the driver by setting the local variable to nullptr, then return true
		this->DrvHandle = nullptr;
		return true;
	}

	bool DriverComponent::OpenDriver(MIDIOPENDESC* OpInfStruct, DWORD CallbackMode, DWORD_PTR CookedPlayerAddress) {
		if (OpInfStruct->hMidi == nullptr) {
			DERROR(DrvErr, L"No valid HMIDI pointer has been specified.", false);
			return false;
		}

		// Save the pointer's address to memory
		this->WMMHandle = OpInfStruct->hMidi;

		// Check if the app wants the driver to do callbacks
		if (CallbackMode != CALLBACK_NULL) {
			if (this->Callback != 0) {
				this->Callback = OpInfStruct->dwCallback;
				this->CallbackMode = CallbackMode;
			}

			// If callback mode is specified but no callback address is specified, abort the initialization
			else {
				DERROR(DrvErr, L"No memory address has been specified for the callback function.", false);
				return false;
			}
		}

		// Check if the app wants a cooked player
		if (CallbackMode & MIDI_IO_COOKED) {
			if (CookedPlayerAddress == NULL) {
				DERROR(DrvErr, L"No memory address has been specified for the MIDI_IO_COOKED player.", false);
				return false;
			}
			// stub
		}

		// Everything is hunky-dory, proceed
		return true;
	}

	bool DriverComponent::CloseDriver() {
		// stub
		this->CallbackFunction(MOM_CLOSE, NULL, NULL);
		this->WMMHandle = nullptr;

		return true;
	}
}

#endif