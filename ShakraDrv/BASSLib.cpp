/*
Shakra Driver component
This .cpp file contains the required code to run the driver.

This file is required for both Windows and Linux.
*/

#include "BASSLib.hpp"

// Quick log

BASS::BASS() {
	// Nothing.
}

BASS::BASS(int OutputIdentifier, int Flags) {
	BASS(OutputIdentifier, 0, Flags);
}

BASS::BASS(int OutputIdentifier, int AudioFrequency, int Flags) {
	if (LoadLib()) {
		if (this->OutputID == 0 && this->AudioFreq == 0)
			BLOG(L"No audio frequency specified with NULL output. Defaulting to 44.1kHz.");

		this->OutputID = OutputIdentifier;
		this->AudioFreq = AudioFrequency ? AudioFrequency : 44100;
		this->Flgs = Flags;

		if (!BASS_Init(OutputID, AudioFreq, Flgs, nullptr, nullptr)) {
			BERROR(L"An error has occured during the initialization of BASS.", false);

			this->_Fail = true;
			return;
		}

		this->BASSErr.Log(L"BASS is ready for incoming streams.", FU, FI, LI);
		return;
	}

	BERROR(L"Shakra failed to load BASS.DLL.", false);
	this->_Fail = true;
	return;
}

BASS::~BASS() {
	if (!BASS_Free()) {
		BERROR(L"BASS encountered an error while trying to free the stream.", false);

		this->_Fail = true;
		return;
	}

	FreeLib();

	BLOG(L"BASS has been shutdown and is ready to be freed.");
}

bool BASS::SetCustomLoadDir(wchar_t* Directory, size_t DirectoryBuf) {
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
		wcscpy_s(this->CustomDir, sizeof(wchar_t) * MAX_PATH, Directory);

		BLOG(L"Custom dir stored successfully into memory.");
		return true;
	}

	BERROR(L"The calloc() function failed to allocate memory for the custom dir.", false);
	return false;
}

bool BASS::LoadLib() {
#ifdef _WIN32
	
	wchar_t LibName[10] = L"\\bass.dll";
	return LibLoader.LoadLib(&BASSLib, LibName, CustomDir);

#elif __linux__

// stub

#elif __APPLE__

// stub

#endif
}

bool BASS::FreeLib() {
#ifdef _WIN32

	if (BASS_Start() != BASS_ERROR_INIT) {
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