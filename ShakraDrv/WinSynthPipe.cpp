/*
Shakra Driver component for Windows
This .h file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#ifdef _WIN32

#include "WinSynthPipe.hpp"

bool WinDriver::SynthPipe::OpenPipe(unsigned short PipeID) {
	wchar_t PipeName[32] = { 0 };
	DWORD PipeDW = PIPE_READMODE_MESSAGE;

	// Check if the pipe ID is valid
	if (PipeID < 0 || PipeID > MAX_DRIVERS) {
		return false;
	}

	// If the pipes haven't been initialized, do it now
	if (!DrvPipes)
	{
		DrvPipes = new (std::nothrow) HANDLE[MAX_DRIVERS];

		if (!DrvPipes) {
			NERROR(SynthErr, L"Failed to allocate pipe handles.", false);
			return false;
		}
	}

	// Prepare the path to the pipe
	swprintf_s(PipeName, 32, PipeNameTemplate, PipeID);

	for (;;) {
		// Open the pipe
		DrvPipes[PipeID] = CreateFile(PipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (DrvPipes[PipeID] != INVALID_HANDLE_VALUE)
			break;

		// If the pipe is busy, quit
		if (GetLastError() != ERROR_PIPE_BUSY) {
			NERROR(SynthErr, L"Failed to open pipe as client.", false);
			return false;
		}

		// All pipe instances are busy, so wait for 20 seconds. 

		if (!WaitNamedPipe(PipeName, 15000)) {
			NERROR(SynthErr, L"Could not open pipe as a client after 15 seconds of wait time.", false);
			return false;
		}
	}

	if (!SetNamedPipeHandleState(DrvPipes[PipeID], &PipeDW, NULL, NULL)) {
		NERROR(SynthErr, L"SetNamedPipeHandleState failed.", false);
		return false;
	}

	return true;
}

bool WinDriver::SynthPipe::ClosePipe(unsigned short PipeID) {
	if (!DrvPipes) {
		LOG(SynthErr, L"ClosePipe() called with no pipes allocated.");
		return true;
	}

	if (CloseHandle(DrvPipes[PipeID])) {
		DrvPipes[PipeID] = nullptr;
		return true;
	}

	LOG(SynthErr, L"CloseHandle() was unable to close the driver's handle.");
	return false;
}

void WinDriver::SynthPipe::ParseShortEvent(unsigned short PipeID, unsigned int Event) {
	DWORD Dummy = 0;

	if (DrvPipes[PipeID])
		WriteFile(DrvPipes[PipeID], &Event, sizeof(unsigned int), &Dummy, NULL);
}

void WinDriver::SynthPipe::ParseLongEvent(unsigned short PipeID, LPMIDIHDR Event) {
	DWORD Dummy = 0;

	if (DrvPipes[PipeID])
		WriteFile(DrvPipes[PipeID], Event, sizeof(LPMIDIHDR), &Dummy, NULL);
}


unsigned int WinDriver::SynthPipe::PrepareLongEvent(LPMIDIHDR Event) {
	if (!Event) {
		NERROR(SynthErr, L"The MIDIHDR buffer doesn't exist, or hasn't been allocated by the host application.", false);
		return MMSYSERR_INVALPARAM;
	}

	if (Event->dwBufferLength > 65535) {
		NERROR(SynthErr, L"The given MIDIHDR buffer is greater than 64K.", false);
		return MMSYSERR_INVALPARAM;
	}

	if (Event->dwFlags & MHDR_PREPARED)
		return MMSYSERR_NOERROR;

	Event->dwFlags |= MHDR_PREPARED;
	return MMSYSERR_NOERROR;
}

unsigned int WinDriver::SynthPipe::UnprepareLongEvent(LPMIDIHDR Event) {
	if (!Event) {
		NERROR(SynthErr, L"The MIDIHDR buffer doesn't exist, or hasn't been allocated by the host application.", false);
		return MMSYSERR_INVALPARAM;
	}

	if (!(Event->dwFlags & MHDR_PREPARED))
		return MMSYSERR_NOERROR;

	if (Event->dwFlags & MHDR_INQUEUE) {
		NERROR(SynthErr, L"The MIDIHDR buffer is still in queue.", false);
		return MIDIERR_STILLPLAYING;
	}

	Event->dwFlags &= ~MHDR_PREPARED;
	return MMSYSERR_NOERROR;
}

#endif