/*
Shakra Driver component
This .cpp file contains the required code to run the driver.

This file is required for both Windows and Linux.
*/

#include "BASS.hpp"

BASS::BASS(int OutputIdentifier, int Flags) {
	BASS(OutputIdentifier, 0, Flags);
}

BASS::BASS(int OutputIdentifier, int AudioFrequency, int Flags) {
	if (OutputID == 0 && AudioFreq == 0)
		BASSErr.Log(F, L"No audio frequency specified with NULL output. Defaulting to 44.1kHz.");

	OutputID = OutputIdentifier;
	AudioFreq = AudioFrequency ? AudioFrequency : 44100;
	Flgs = Flags;

	if (!BASS_Init(OutputID, AudioFreq, Flgs, nullptr, nullptr)) {
		BASSErr.ThrowError(F, L"An error has occured during the initialization of BASS.", false);

		_Fail = true;
		return;
	}

	BASSErr.Log(F, L"BASS is ready for incoming streams.");
}

BASS::~BASS() {
	if (!BASS_Free()) {
		BASSErr.ThrowError(F, L"BASS encountered an error while trying to free the stream.", false);

		_Fail = true;
		return;
	}

	BASSErr.Log(F, L"BASS has been shutdown and is ready to be freed.");
}

bool BASS::SetCustomLoadDir(wchar_t* Directory, size_t DirectoryBuf) {
	if (Directory == nullptr) {
		BASSErr.ThrowError(F, L"No pointer has been passed to the function", false);
		return false;
	}

	if (DirectoryBuf == 0) {
		BASSErr.ThrowError(F, L"No size has been passed to the function", false);
		return false;
	}

	if (wcslen(CustomDir) != 0) {
		BASSErr.Log(F, L"There's already a dir stored in memory, it will be overwrited...");
		free(CustomDir);
	}

	if (CustomDir != nullptr) {
		memset(CustomDir, 0, sizeof(wchar_t) * MAX_PATH);
		wcscpy_s(CustomDir, sizeof(wchar_t) * MAX_PATH, Directory);

		BASSErr.Log(F, L"Custom dir stored successfully into memory.");
		return true;
	}

	BASSErr.ThrowError(F, L"The calloc() function failed to allocate memory for the custom dir.", false);
	return false;
}

bool BASS::LoadLib() {
#ifdef _WIN32
	
	wchar_t FPath[MAX_PATH] = { 0 };

	if (BASSLib != nullptr)
	{
		BASSErr.Log(F, L"BASS has already been loaded through another function. All good.");
		return true;
	}

	if (!(BASSLib = LoadLibrary(L"bass.dll"))) {
		BASSErr.Log(F, L"No BASS lib found in memory. We'll load our own.");

		if (CustomDir != nullptr) {

			BASSErr.Log(F, L"A custom dir has been specified, we'll use it to load BASS.");
			wcscat_s(FPath, MAX_PATH, CustomDir);
		}
		else {
			wchar_t* SPath;
			HRESULT SHR = SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_NO_ALIAS, NULL, &SPath);

			if (!SUCCEEDED(SHR)) {
				BASSErr.ThrowError(F, L"Failed to load BASS into memory.", false);
				return false;
			}

			wcscat_s(FPath, MAX_PATH, SPath);
			wcscat_s(FPath, MAX_PATH, L"\\Shakra");
		}

		wcscat_s(FPath, MAX_PATH, L"\\bass.dll");
		if (!(BASSLib = LoadLibraryEx(FPath, NULL, 0))) {
			BASSErr.ThrowError(F, L"Failed to load BASS into memory.", false);
			return false;
		}
	}

	return true;

#elif __linux__

// stub

#elif __APPLE__

// stub

#endif
}

bool BASS::FreeLib() {
#ifdef _WIN32

	if (BASS_Start() != BASS_ERROR_INIT) {
		BASSErr.ThrowError(F, L"You can't unload BASS while it's active!", false);
		return false;
	}

	if (BASSLib != nullptr) {
		if (!FreeLibrary(BASSLib)) {
			BASSErr.ThrowFatalError(L"The driver failed to unload BASS.DLL.\n\nThis is not supposed to happen!");
			return false;
		}
	}
	else BASSErr.Log(F, L"BASS isn't loaded.");

	return true;

#elif __linux__

	// stub

#elif __APPLE__

	// stub

#endif
}