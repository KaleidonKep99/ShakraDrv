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
			NERROR(SynthErr, L"Failed to allocate pipe handles for the short events buffer.", false);
			return false;
		}
	}

	if (!PDrvLongEvBuf)
	{
		PDrvLongEvBuf = new (std::nothrow) HANDLE[MAX_DRIVERS];

		if (!PDrvLongEvBuf) {
			NERROR(SynthErr, L"Failed to allocate pipe handles for the long events buffer.", false);
			return false;
		}
	}

	if (!PDrvLongEvBufLen)
	{
		PDrvLongEvBufLen = new (std::nothrow) HANDLE[MAX_DRIVERS];

		if (!PDrvLongEvBufLen) {
			NERROR(SynthErr, L"Failed to allocate pipe handles for the long events buffer.", false);
			return false;
		}
	}

	if (!PDrvEvBufHeads)
	{
		PDrvEvBufHeads = new (std::nothrow) HANDLE[MAX_DRIVERS];

		if (!PDrvEvBufHeads) {
			NERROR(SynthErr, L"Failed to allocate buffer R/W heads handles.", false);
			return false;
		}
	}

	// If the buffer hasn't been initialized, do it now
	if (!DrvEvBuf) {
		DrvEvBuf = new (std::nothrow) LPDWORD[MAX_DRIVERS];

		if (!DrvEvBuf) {
			NERROR(SynthErr, L"Failed to allocate short events buffer.", false);
			return false;
		}
	}

	if (!DrvLongEvBuf) {
		DrvLongEvBuf = new (std::nothrow) LPBYTE[MAX_DRIVERS];

		if (!DrvLongEvBuf) {
			NERROR(SynthErr, L"Failed to allocate long events buffer.", false);
			return false;
		}
	}

	if (!DrvLongEvBufLen) {
		DrvLongEvBufLen = new (std::nothrow) LPDWORD[MAX_DRIVERS];

		if (!DrvLongEvBufLen) {
			NERROR(SynthErr, L"Failed to allocate long events buffer.", false);
			return false;
		}
	}

	if (!DrvEvBufHeads) {
		DrvEvBufHeads = new (std::nothrow) PEvBufHeads[MAX_DRIVERS];

		if (!DrvEvBufHeads) {
			NERROR(SynthErr, L"Failed to allocate heads for the buffer.", false);
			return false;
		}
	}

	return true;
}

bool WinDriver::SynthPipe::PrepareFileMappings(unsigned short PipeID, bool Create, int Size) {
	wchar_t FMName[32] = { 0 };

	// Check if the pipe ID is valid
	if (PipeID > MAX_DRIVERS) {
		return false;
	}

	// Prepare arrays if they're not allocated yet
	if (PrepareArrays()) {
		// Check if size is valid
		if (Create) {
			if (Size < 0 || Size > 2147483647) {
				EvBufSize = 4096;
			}
			else EvBufSize = Size;
		}

		// Initialize short buffer
		swprintf_s(FMName, 32, FileMappingTemplate, SEvLabel, PipeID);
		PDrvEvBuf[PipeID] = 
			Create ?
			// If "Create" is true, create the file mapping
			CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(DWORD) * EvBufSize, FMName) :
			// Else, open the already existing (if it exists ofc) file mapping
			OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, FMName);

		if (PDrvEvBuf[PipeID]) {
			SetNamedSecurityInfo(FMName, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);
			PVOID MOVF = MapViewOfFile(PDrvEvBuf[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, sizeof(DWORD) * EvBufSize);

			if (MOVF != nullptr) {
				DrvEvBuf[PipeID] = (DWORD*)MOVF;
			}
			else
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}
		}


		// Initialize long buffer
		swprintf_s(FMName, 32, FileMappingTemplate, LEvLabel, PipeID);
		PDrvLongEvBuf[PipeID] =
			Create ?
			// If "Create" is true, create the file mapping
			CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(BYTE) * 65536, FMName) :
			// Else, open the already existing (if it exists ofc) file mapping
			OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, FMName);

		if (PDrvLongEvBuf[PipeID]) {
			SetNamedSecurityInfo(FMName, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);
			PVOID MOVF = MapViewOfFile(PDrvLongEvBuf[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BYTE) * 65536);

			if (MOVF != nullptr) {
				DrvLongEvBuf[PipeID] = (BYTE*)MOVF;
			}
			else
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}
		}

		swprintf_s(FMName, 32, FileMappingTemplate, LEvLenLabel, PipeID);
		PDrvLongEvBufLen[PipeID] =
			Create ?
			// If "Create" is true, create the file mapping
			CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(DWORD), FMName) :
			// Else, open the already existing (if it exists ofc) file mapping
			OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, FMName);

		if (PDrvLongEvBufLen[PipeID]) {
			SetNamedSecurityInfo(FMName, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);
			PVOID MOVF = MapViewOfFile(PDrvLongEvBufLen[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, sizeof(DWORD));

			if (MOVF != nullptr) {
				DrvLongEvBufLen[PipeID] = (DWORD*)MOVF;
			}
			else
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}
		}

		// Initialize buffer heads
		swprintf_s(FMName, 32, FileMappingTemplate, EvHeadsLabel, PipeID);
		PDrvEvBufHeads[PipeID] =
			Create ?
			// If "Create" is true, create the file mapping
			CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(EvBufHeads), FMName) :
			// Else, open the already existing (if it exists ofc) file mapping
			OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, FMName);

		if (PDrvEvBufHeads[PipeID]) {
			SetNamedSecurityInfo(FMName, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, (PACL)NULL, NULL);
			PVOID MOVF = MapViewOfFile(PDrvEvBufHeads[PipeID], FILE_MAP_ALL_ACCESS, 0, 0, sizeof(EvBufHeads));

			if (MOVF != nullptr) {
				DrvEvBufHeads[PipeID] = (PEvBufHeads)MOVF;
			}
			else
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

	if (PDrvLongEvBufLen[PipeID]) {
		UnmapViewOfFile(DrvLongEvBufLen[PipeID]);
		CloseHandle(PDrvLongEvBufLen[PipeID]);
		PDrvLongEvBufLen[PipeID] = nullptr;
	}

	if (PDrvEvBufHeads[PipeID]) {
		UnmapViewOfFile(DrvEvBufHeads[PipeID]);
		CloseHandle(PDrvEvBufHeads[PipeID]);
		PDrvEvBufHeads[PipeID] = nullptr;
	}

	return true;
}

bool WinDriver::SynthPipe::PerformBufferCheck(unsigned short PipeID) {
	return (DrvEvBufHeads[PipeID]->ShortReadHead != DrvEvBufHeads[PipeID]->ShortWriteHead);
}

void WinDriver::SynthPipe::ResetReadHeadsIfNeeded(unsigned short PipeID) {
	if (++DrvEvBufHeads[PipeID]->ShortReadHead >= EvBufSize)
		DrvEvBufHeads[PipeID]->ShortReadHead = 0;

	if (++DrvEvBufHeads[PipeID]->LongReadHead >= MAX_MIDIHDR_BUF)
		DrvEvBufHeads[PipeID]->LongReadHead = 0;
}

void WinDriver::SynthPipe::GetReadHeadPos(unsigned short PipeID, int *SRH, int *LRH) {
	*SRH = DrvEvBufHeads[PipeID]->ShortReadHead;
	*LRH = DrvEvBufHeads[PipeID]->LongReadHead;
}

void WinDriver::SynthPipe::GetWriteHeadPos(unsigned short PipeID, int* SWH, int* LWH) {
	*SWH = DrvEvBufHeads[PipeID]->ShortWriteHead;
	*LWH = DrvEvBufHeads[PipeID]->LongWriteHead;
}

unsigned int WinDriver::SynthPipe::ParseShortEvent(unsigned short PipeID) {
	return DrvEvBuf[PipeID][DrvEvBufHeads[PipeID]->ShortReadHead];
}

unsigned int WinDriver::SynthPipe::ParseLongEvent(unsigned short PipeID, BYTE* PEvent) {
	unsigned int Len = 0;

	if (*(DrvLongEvBufLen[PipeID]) < 1)
		return 0;

	// Copy new buffer
	Len = *(DrvLongEvBufLen[PipeID]);
	memcpy(PEvent, DrvLongEvBuf[PipeID], Len);

	// Clear buffer
	memset(DrvLongEvBuf[PipeID], 0, sizeof(BYTE) * 65536);
	*(DrvLongEvBufLen[PipeID]) = 0;

	return Len;
}

void WinDriver::SynthPipe::SaveShortEvent(unsigned short PipeID, unsigned int Event) {
	if (!DrvEvBuf[PipeID])
		return;

	auto NextWriteHead = DrvEvBufHeads[PipeID]->ShortWriteHead + 1;
	if (NextWriteHead >= EvBufSize) NextWriteHead = 0;

	DrvEvBuf[PipeID][DrvEvBufHeads[PipeID]->ShortWriteHead] = Event;

	if (NextWriteHead != DrvEvBufHeads[PipeID]->ShortReadHead)
		DrvEvBufHeads[PipeID]->ShortWriteHead = NextWriteHead;
}

unsigned int WinDriver::SynthPipe::SaveLongEvent(unsigned short PipeID, LPMIDIHDR Event) {
	if (!(Event->dwFlags & MHDR_PREPARED)) {
		NERROR(SynthErr, L"The MIDIHDR is not prepared.", false);
		return MIDIERR_UNPREPARED;
	}

	while (*(DrvLongEvBufLen[PipeID]) != 0)
		Sleep(1);

	Event->dwFlags &= ~MHDR_DONE;
	Event->dwFlags |= MHDR_INQUEUE;

	// Copy new buffer
	*(DrvLongEvBufLen[PipeID]) = Event->dwBufferLength;
	memcpy(DrvLongEvBuf[PipeID], Event->lpData, Event->dwBufferLength);

	Event->dwFlags &= ~MHDR_INQUEUE;
	Event->dwFlags |= MHDR_DONE;

	return MMSYSERR_NOERROR;
}

unsigned int WinDriver::SynthPipe::PrepareLongEvent(unsigned short PipeID, LPMIDIHDR Event) {
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

unsigned int WinDriver::SynthPipe::UnprepareLongEvent(unsigned short PipeID, LPMIDIHDR Event) {
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