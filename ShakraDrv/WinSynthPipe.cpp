/*
Shakra Driver component for Windows
This .h file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#ifdef _WIN32

#include "WinSynthPipe.hpp"

bool WinDriver::SynthPipe::PrepareArrays() {
	// If the pipes haven't been initialized, do it now
	if (!PDrvEvBuf)
	{
		PDrvEvBuf = new (std::nothrow) HANDLE[MAX_DRIVERS];

		if (!PDrvEvBuf) {
			NERROR(SynthErr, L"Failed to allocate pipe handles.", false);
			return false;
		}
	}

	if (!PDrvEvBufHeads)
	{
		PDrvEvBufHeads = new (std::nothrow) HANDLE[MAX_DRIVERS];

		if (!PDrvEvBufHeads) {
			NERROR(SynthErr, L"Failed to allocate buffer handles.", false);
			return false;
		}
	}

	// If the buffer hasn't been initialized, do it now
	if (!DrvEvBuf) {
		DrvEvBuf = new (std::nothrow) DWORD * [EvBufSize];

		if (!DrvEvBuf) {
			NERROR(SynthErr, L"Failed to allocate events buffer.", false);
			return false;
		}
	}

	if (!DrvEvBufHeads)
	{
		DrvEvBufHeads = new (std::nothrow) PEvBufHeads[MAX_DRIVERS];

		if (!DrvEvBufHeads) {
			NERROR(SynthErr, L"Failed to allocate heads for the buffer.", false);
			return false;
		}
	}

	return true;
}

bool WinDriver::SynthPipe::CreatePipe(unsigned short PipeID, int Size) {
	wchar_t PipeNameH[32] = { 0 };
	wchar_t PipeNameD[32] = { 0 };

	// Check if the pipe ID is valid
	if (PipeID > MAX_DRIVERS) {
		return false;
	}

	// Prepare arrays if they're not allocated yet
	if (PrepareArrays()) {
		// Check if size is valid
		if (Size < 0 || Size > 2147483647) {
			EvBufSize = 4096;
		}
		else EvBufSize = Size;

		// Prepare the path to the pipe
		swprintf_s(PipeNameH, 32, PipeNameTemplate, L"Heads", PipeID);
		swprintf_s(PipeNameD, 32, PipeNameTemplate, L"Data", PipeID);

		// Initialize buffer
		PDrvEvBuf[PipeID] = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(DWORD) * EvBufSize, PipeNameD);
		if (PDrvEvBuf[PipeID]) {
			DrvEvBuf[PipeID] = new DWORD[EvBufSize];

			SetNamedSecurityInfo(PipeNameD, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);
			DrvEvBuf[PipeID] = (DWORD*)MapViewOfFile(PDrvEvBuf[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (DrvEvBuf[PipeID] == nullptr)
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}

			LOG(SynthErr, L"MapViewOfFile has successfully allocated DrvEvBuf.");
		}

		// Initialize buffer heads
		PDrvEvBufHeads[PipeID] = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(EvBufHeads), PipeNameH);
		if (PDrvEvBufHeads[PipeID]) {
			SetNamedSecurityInfo(PipeNameH, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);
			DrvEvBufHeads[PipeID] = (PEvBufHeads)MapViewOfFile(PDrvEvBufHeads[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (DrvEvBufHeads[PipeID] == nullptr)
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}

			LOG(SynthErr, L"MapViewOfFile has successfully allocated PDrvEvBufHeads.");
		}

		return true;
	}

	NERROR(SynthErr, nullptr, false);
	return false;
}

bool WinDriver::SynthPipe::OpenPipe(unsigned short PipeID) {
	wchar_t PipeNameH[32] = { 0 };
	wchar_t PipeNameD[32] = { 0 };

	// Check if the pipe ID is valid
	if (PipeID < 0 || PipeID > MAX_DRIVERS) {
		return false;
	}

	// Prepare arrays if they're not allocated yet
	if (PrepareArrays()) {
		// Prepare the path to the pipe
		swprintf_s(PipeNameH, 32, PipeNameTemplate, L"Heads", PipeID);
		swprintf_s(PipeNameD, 32, PipeNameTemplate, L"Data", PipeID);

		// Initialize buffer
		PDrvEvBuf[PipeID] = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, PipeNameD);
		if (PDrvEvBuf[PipeID]) {
			DrvEvBuf[PipeID] = new DWORD[EvBufSize];

			SetNamedSecurityInfo(PipeNameD, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);
			DrvEvBuf[PipeID] = (DWORD*)MapViewOfFile(PDrvEvBuf[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (DrvEvBuf[PipeID] == nullptr)
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}

			LOG(SynthErr, L"MapViewOfFile has successfully allocated DrvEvBuf.");
		}

		// Initialize buffer heads
		PDrvEvBufHeads[PipeID] = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, PipeNameH);
		if (PDrvEvBufHeads[PipeID]) {
			SetNamedSecurityInfo(PipeNameH, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);
			DrvEvBufHeads[PipeID] = (PEvBufHeads)MapViewOfFile(PDrvEvBufHeads[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (DrvEvBufHeads[PipeID] == nullptr)
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}

			LOG(SynthErr, L"MapViewOfFile has successfully allocated PDrvEvBufHeads.");
		}

		return true;
	}

	NERROR(SynthErr, nullptr, false);
	return false;
}

bool WinDriver::SynthPipe::ClosePipe(unsigned short PipeID) {
	if (!PDrvEvBufHeads && !PDrvEvBuf) {
		LOG(SynthErr, L"ClosePipe() called with no pipes allocated.");
		return true;
	}

	if (PDrvEvBuf[PipeID]) {
		delete[] DrvEvBuf[PipeID];
		UnmapViewOfFile(DrvEvBuf[PipeID]);
	}

	if (PDrvEvBufHeads[PipeID]) {
		delete[] DrvEvBufHeads[PipeID];
		UnmapViewOfFile(DrvEvBuf[PipeID]);
	}

	if (CloseHandle(PDrvEvBuf[PipeID]) &&
		CloseHandle(PDrvEvBufHeads[PipeID])) {
		PDrvEvBuf[PipeID] = nullptr;
		return true;
	}

	LOG(SynthErr, L"CloseHandle() was unable to close the driver's handle.");
	return false;
}

bool WinDriver::SynthPipe::PerformBufferCheck(unsigned short PipeID) {
	return (DrvEvBufHeads[PipeID]->ReadHead != DrvEvBufHeads[PipeID]->WriteHead);
}

void WinDriver::SynthPipe::ResetReadHeadIfNeeded(unsigned short PipeID) {
	if (++DrvEvBufHeads[PipeID]->ReadHead >= EvBufSize)
		DrvEvBufHeads[PipeID]->ReadHead = 0;
}

int WinDriver::SynthPipe::GetReadHeadPos(unsigned short PipeID) {
	return DrvEvBufHeads[PipeID]->ReadHead;
}

int WinDriver::SynthPipe::GetWriteHeadPos(unsigned short PipeID) {
	return DrvEvBufHeads[PipeID]->WriteHead;
}

unsigned int WinDriver::SynthPipe::ParseShortEvent(unsigned short PipeID) {
	return DrvEvBuf[PipeID][DrvEvBufHeads[PipeID]->ReadHead];
}

void WinDriver::SynthPipe::SaveShortEvent(unsigned short PipeID, unsigned int Event) {
	if (!DrvEvBuf[PipeID])
		return;

	auto NextWriteHead = DrvEvBufHeads[PipeID]->WriteHead + 1;
	if (NextWriteHead >= EvBufSize) NextWriteHead = 0;

	DrvEvBuf[PipeID][DrvEvBufHeads[PipeID]->WriteHead] = Event;

	if (NextWriteHead != DrvEvBufHeads[PipeID]->ReadHead)
		DrvEvBufHeads[PipeID]->WriteHead = NextWriteHead;
}

void WinDriver::SynthPipe::SaveLongEvent(unsigned short PipeID, MIDIHDR* Event) {
	// STUB
	return;

	DWORD Dummy = 0;

	if (DrvEvBuf[PipeID])
	{
		Event->dwFlags &= ~MHDR_DONE;
		Event->dwFlags |= MHDR_INQUEUE;

		WriteFile(DrvEvBuf[PipeID], Event->lpData, Event->dwBytesRecorded, &Dummy, NULL);

		Event->dwFlags &= ~MHDR_INQUEUE;
		Event->dwFlags |= MHDR_DONE;
	}
}


unsigned int WinDriver::SynthPipe::PrepareLongEvent(MIDIHDR* Event) {
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

unsigned int WinDriver::SynthPipe::UnprepareLongEvent(MIDIHDR* Event) {
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