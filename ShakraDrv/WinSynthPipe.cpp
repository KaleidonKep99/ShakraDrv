/*
Shakra Driver component for Windows
This .h file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#ifdef _WIN32

#include "WinSynthPipe.hpp"

bool WinDriver::SynthPipe::OpenSynthHost() {
	const wchar_t* AppName = L"ShakraHost.exe";
	wchar_t Dummy[MAX_PATH] = { 0 };
	wchar_t HostApp[MAX_PATH] = { 0 };

	PWSTR PF = NULL;
	GUID PFGUID = FOLDERID_ProgramFilesX86;

	HANDLE AppSnapshot;
	PROCESSENTRY32 AppEntry;
	PROCESS_INFORMATION AppPI;
	STARTUPINFO AppSI;

	bool IsOpen = false;

	AppSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	AppEntry.dwSize = sizeof(PROCESSENTRY32);

	while (!IsOpen) {
		if (Process32First(AppSnapshot, &AppEntry)) {
			if (!_wcsicmp(AppEntry.szExeFile, AppName))
				IsOpen = true;

			while (Process32Next(AppSnapshot, &AppEntry) && !IsOpen) {
				if (!_wcsicmp(AppEntry.szExeFile, AppName))
				{
					IsOpen = true;
					LOG(SynthErr, L"Found a match for the synth.");
					break;
				}
			}
		}

		if (IsOpen)
			break;

#ifndef _WIN64
		if (!GetSystemWow64Directory(Dummy, _countof(Dummy)))
		{
			PFGUID = FOLDERID_ProgramFiles;
			LOG(SynthErr, L"32-bit process is running on 32-bit Windows.");
		}
#endif

		if (SUCCEEDED(SHGetKnownFolderPath(PFGUID, 0, NULL, &PF))) {
			swprintf_s(HostApp, MAX_PATH, L"%s\\Shakra Driver\\%s", PF, AppName);

			LOG(SynthErr, HostApp);

			CoTaskMemFree(PF);

			memset(&AppPI, 0, sizeof(AppPI));
			memset(&AppSI, 0, sizeof(AppSI));
			AppSI.cb = sizeof(AppSI);

			CreateProcess(
				HostApp,
				NULL,
				NULL,
				NULL,
				TRUE,
				CREATE_NEW_CONSOLE,
				NULL,
				NULL,
				&AppSI,
				&AppPI
			);
			LOG(SynthErr, L"Created synthesizer process.");

			WaitForSingleObject(AppPI.hProcess, 1500);

			CloseHandle(AppPI.hProcess);
			CloseHandle(AppPI.hThread);
		}
		else {
			CoTaskMemFree(PF);
			return false;
		}
	}

	return true;
}

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

	if (!PDrvLongEvBuf)
	{
		PDrvLongEvBuf = new (std::nothrow) HANDLE[MAX_DRIVERS];

		if (!PDrvLongEvBuf) {
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

	if (!DrvLongEvBuf) {
		DrvLongEvBuf = new (std::nothrow) MIDIHDR * [MAX_MIDIHDR_BUF];

		if (!DrvLongEvBuf) {
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
	wchar_t PipeNameL[32] = { 0 };
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
		swprintf_s(PipeNameL, 32, PipeNameTemplate, L"LongData", PipeID);
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
		}

		PDrvLongEvBuf[PipeID] = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(MIDIHDR) * MAX_MIDIHDR_BUF, PipeNameL);
		if (PDrvLongEvBuf[PipeID]) {
			DrvLongEvBuf[PipeID] = new MIDIHDR[1];

			SetNamedSecurityInfo(PipeNameD, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);
			DrvLongEvBuf[PipeID] = (MIDIHDR*)MapViewOfFile(PDrvLongEvBuf[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (DrvLongEvBuf[PipeID] == nullptr)
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}
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
		}

		return true;
	}

	NERROR(SynthErr, nullptr, false);
	return false;
}

bool WinDriver::SynthPipe::OpenPipe(unsigned short PipeID) {
	wchar_t PipeNameH[32] = { 0 };
	wchar_t PipeNameL[32] = { 0 };
	wchar_t PipeNameD[32] = { 0 };

	// Check if the pipe ID is valid
	if (PipeID < 0 || PipeID > MAX_DRIVERS) {
		return false;
	}

	if (!OpenSynthHost()) {
		NERROR(SynthErr, L"Unable to open Shakra host synthesizer.", false);
		return false;
	}

	// Prepare arrays if they're not allocated yet
	if (PrepareArrays()) {
		// Prepare the path to the pipe
		swprintf_s(PipeNameH, 32, PipeNameTemplate, L"Heads", PipeID);
		swprintf_s(PipeNameL, 32, PipeNameTemplate, L"LongData", PipeID);
		swprintf_s(PipeNameD, 32, PipeNameTemplate, L"Data", PipeID);

		// Initialize buffer
		PDrvEvBuf[PipeID] = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, PipeNameD);
		if (PDrvEvBuf[PipeID]) {
			DrvEvBuf[PipeID] = new DWORD[EvBufSize];
			DrvEvBuf[PipeID] = (DWORD*)MapViewOfFile(PDrvEvBuf[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (DrvEvBuf[PipeID] == nullptr)
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}
		}

		PDrvLongEvBuf[PipeID] = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, PipeNameL);
		if (PDrvLongEvBuf[PipeID]) {
			DrvLongEvBuf[PipeID] = new MIDIHDR[1];
			DrvLongEvBuf[PipeID] = (MIDIHDR*)MapViewOfFile(PDrvLongEvBuf[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (DrvLongEvBuf[PipeID] == nullptr)
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}
		}

		// Initialize buffer heads
		PDrvEvBufHeads[PipeID] = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, PipeNameH);
		if (PDrvEvBufHeads[PipeID]) {
			DrvEvBufHeads[PipeID] = (PEvBufHeads)MapViewOfFile(PDrvEvBufHeads[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (DrvEvBufHeads[PipeID] == nullptr)
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}
		}

		return true;
	}

	NERROR(SynthErr, nullptr, false);
	return false;
}

bool WinDriver::SynthPipe::ClosePipe(unsigned short PipeID) {
	if (!PDrvEvBuf && !PDrvLongEvBuf && !PDrvEvBufHeads) {
		LOG(SynthErr, L"ClosePipe() called with no pipes allocated.");
		return true;
	}

	if (PDrvEvBuf[PipeID]) {
		UnmapViewOfFile(DrvEvBuf[PipeID]);
		CloseHandle(PDrvEvBuf[PipeID]);
		PDrvEvBuf[PipeID] = nullptr;
	}

	if (PDrvLongEvBuf[PipeID]) {
		UnmapViewOfFile(DrvLongEvBuf[PipeID]);
		CloseHandle(PDrvLongEvBuf[PipeID]);
		PDrvLongEvBuf[PipeID] = nullptr;
	}

	if (PDrvEvBufHeads[PipeID]) {
		UnmapViewOfFile(DrvEvBufHeads[PipeID]);
		CloseHandle(PDrvEvBufHeads[PipeID]);
		PDrvEvBufHeads[PipeID] = nullptr;
	}

	LOG(SynthErr, L"ClosePipe is done.");
	return true;
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

int WinDriver::SynthPipe::ParseLongEvent(unsigned short PipeID, LPSTR* IIMidiHdr) {
	if (DrvLongEvBuf[PipeID][0].lpData == nullptr) {
		memcpy(IIMidiHdr, &DrvLongEvBuf[PipeID][0].lpData, DrvLongEvBuf[PipeID][0].dwBufferLength);
		free(DrvLongEvBuf[PipeID][0].lpData);
		return DrvLongEvBuf[PipeID][0].dwBufferLength;
	}

	return -1;
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
	Event->dwFlags &= ~MHDR_DONE;
	Event->dwFlags |= MHDR_INQUEUE;

	if (DrvLongEvBuf[PipeID][0].lpData == nullptr) {
		DrvLongEvBuf[PipeID][0].lpData = (LPSTR)malloc(Event->dwBufferLength);
		memcpy(&DrvLongEvBuf[PipeID][0].lpData, Event->lpData, Event->dwBufferLength);
		DrvLongEvBuf[PipeID][0].dwBufferLength = Event->dwBufferLength;
		DrvLongEvBuf[PipeID][0].dwBytesRecorded = Event->dwBytesRecorded;

		while (DrvLongEvBuf[PipeID][0].lpData)
			Sleep(1);
	}

	Event->dwFlags &= ~MHDR_INQUEUE;
	Event->dwFlags |= MHDR_DONE;
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