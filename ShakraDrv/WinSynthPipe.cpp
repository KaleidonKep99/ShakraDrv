/*
Shakra Driver component for Windows
This .h file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#ifdef _WIN32

#include "WinSynthPipe.hpp"

bool WinDriver::SynthPipe::OpenSynthHost(const wchar_t* Target) {
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
#ifndef _WIN64
		if (!GetSystemWow64Directory(Dummy, _countof(Dummy)))
		{
			PFGUID = FOLDERID_ProgramFiles;
			LOG(SynthErr, L"32-bit process is running on 32-bit Windows.");
		}
#endif

		if (SUCCEEDED(SHGetKnownFolderPath(PFGUID, 0, NULL, &PF))) {
			swprintf_s(HostApp, MAX_PATH, L"%s\\Shakra Driver\\%s %s", PF, AppName, Target);

			LOG(SynthErr, HostApp);

			CoTaskMemFree(PF);

			memset(&AppPI, 0, sizeof(AppPI));
			memset(&AppSI, 0, sizeof(AppSI));
			AppSI.cb = sizeof(AppSI);

			MessageBox(NULL, Target, L"A", MB_OK);

			CreateProcessW(
				NULL,
				HostApp,
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

			WaitForSingleObject(AppPI.hProcess, 5000);

			CloseHandle(AppPI.hProcess);
			CloseHandle(AppPI.hThread);

			IsOpen = true;
		}
		else {
			CoTaskMemFree(PF);
			return false;
		}
	}

	return true;
}

std::wstring WinDriver::SynthPipe::GenerateID() {
	auto randchar = []() -> char
	{
		const char charset[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
		const size_t max_index = (sizeof(charset) - 1);
		return charset[rand() % max_index];
	};
	std::wstring str(32, 0);
	std::generate_n(str.begin(), 32, randchar);
	return str;
}

bool WinDriver::SynthPipe::PrepareArrays() {
	// If the pipes haven't been initialized, do it now
	if (!DrvShortEvBuf)
	{
		DrvShortEvBuf = new (std::nothrow) ShortEvBuf;

		if (!DrvShortEvBuf) {
			NERROR(SynthErr, L"Failed to allocate pipe handles for the short events buffer.", false);
			return false;
		}
	}

	if (!DrvLongEvBuf)
	{
		DrvLongEvBuf = new (std::nothrow) LongEvBuf;

		if (!DrvLongEvBuf) {
			NERROR(SynthErr, L"Failed to allocate pipe handles for the long events buffer.", false);
			return false;
		}
	}

	return true;
}

bool WinDriver::SynthPipe::PrepareFileMappings(const wchar_t* Pipe, bool Create, int Size) {
	std::wstring TempID = GenerateID();
	wchar_t FMName[MAX_PATH] = { 0 };

	// Prepare arrays if they're not allocated yet
	if (PrepareArrays()) {
		// Check if size is valid
		if (Create) {
			if (Size < 0 || Size > 2147483647) {
				EvBufSize = 4096;
			}
			else EvBufSize = 32768;
		}

		// Initialize short buffer
		swprintf_s(FMName, MAX_PATH, FileMappingTemplate, SEvLabel, (!Pipe ? TempID.c_str() : Pipe));
		MessageBox(NULL, FMName, (!Pipe ? TempID.c_str() : Pipe), MB_OK);
		PDrvShortEvBuf =
			Create ?
			// If "Create" is true, create the file mapping
			CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, 0, sizeof(ShortEvBuf) + (sizeof(DWORD) * 32768), FMName) :
			// Else, open the already existing (if it exists ofc) file mapping
			OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, FMName);

		if (PDrvShortEvBuf) {
			if (Create)
				SetSecurityInfo(PDrvShortEvBuf, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, 0, 0, 0, 0);

			PVOID MOVF = MapViewOfFile(PDrvShortEvBuf, FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (MOVF != nullptr) DrvShortEvBuf = (PShortEvBuf)MOVF;
			else
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}

			DrvShortEvBuf->Buf = (PSE)calloc(MAX_SE_BUF, sizeof(SE));

			if (!DrvShortEvBuf->Buf) {
				NERROR(SynthErr, nullptr, false);
				return false;
			}
		}

		// Initialize long buffer
		swprintf_s(FMName, MAX_PATH, FileMappingTemplate, LEvLabel, (!Pipe ? TempID.c_str() : Pipe));
		MessageBox(NULL, FMName, (!Pipe ? TempID.c_str() : Pipe), MB_OK);
		PDrvLongEvBuf =
			Create ?
			// If "Create" is true, create the file mapping
			CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, 0, sizeof(LongEvBuf), FMName) :
			// Else, open the already existing (if it exists ofc) file mapping
			OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, FMName);

		if (PDrvLongEvBuf) {
			if (Create)
				SetSecurityInfo(PDrvLongEvBuf, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, 0, 0, 0, 0);

			PVOID MOVF = MapViewOfFile(PDrvLongEvBuf, FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (MOVF != nullptr) DrvLongEvBuf = (PLongEvBuf)MOVF;
			else
			{
				NERROR(SynthErr, nullptr, false);
				return false;
			}

			DrvLongEvBuf->Buf = (PLE)calloc(MAX_LE_BUF, sizeof(LE));

			if (!DrvLongEvBuf->Buf) {
				NERROR(SynthErr, nullptr, false);
				return false;
			}
		}

		if (!Create) 
		{
			if (!OpenSynthHost(!Pipe ? TempID.c_str() : Pipe))
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

bool WinDriver::SynthPipe::ClosePipe() {
	if (!PDrvShortEvBuf && !PDrvLongEvBuf) {
		LOG(SynthErr, L"ClosePipe() called with no pipes allocated.");
		return true;
	}

	if (PDrvShortEvBuf) {
		if (DrvShortEvBuf->Buf)
			free(DrvShortEvBuf->Buf);

		UnmapViewOfFile(DrvShortEvBuf);
		CloseHandle(PDrvShortEvBuf);
		PDrvShortEvBuf = nullptr;
	}

	if (PDrvLongEvBuf) {
		UnmapViewOfFile(DrvLongEvBuf);
		CloseHandle(PDrvLongEvBuf);
		PDrvShortEvBuf = nullptr;
	}

	return true;
}

bool WinDriver::SynthPipe::PerformBufferCheck() {
	return ((DrvShortEvBuf->ReadHead != DrvShortEvBuf->WriteHead));
}

void WinDriver::SynthPipe::ResetReadHeadsIfNeeded() {
	if (++DrvShortEvBuf->ReadHead >= EvBufSize)
		DrvShortEvBuf->ReadHead = 0;

	if (++DrvLongEvBuf->ReadHead >= MAX_MIDIHDR_BUF)
		DrvLongEvBuf->ReadHead = 0;
}

int WinDriver::SynthPipe::GetReadHeadPos() {
	return DrvShortEvBuf->ReadHead;
}

int WinDriver::SynthPipe::GetWriteHeadPos() {
	return DrvShortEvBuf->WriteHead;
}

unsigned int WinDriver::SynthPipe::ParseShortEvent() {
	return DrvShortEvBuf->Buf[DrvShortEvBuf->ReadHead].Event;
}

unsigned int WinDriver::SynthPipe::ParseLongEvent(BYTE* PEvent) {
	unsigned int Len = 0;

	if (DrvLongEvBuf->Buf[DrvLongEvBuf->ReadHead].EventLength < 1)
		return 0;

	// Copy new buffer
	Len = DrvLongEvBuf->Buf[DrvLongEvBuf->ReadHead].EventLength;
	memcpy(PEvent, DrvLongEvBuf->Buf[DrvLongEvBuf->ReadHead].Event, Len);

	// Clear buffer
	memset(DrvLongEvBuf->Buf[DrvLongEvBuf->ReadHead].Event, 0, sizeof(BYTE) * 65536);
	DrvLongEvBuf->Buf[DrvLongEvBuf->ReadHead].EventLength = 0;

	return Len;
}

void WinDriver::SynthPipe::SaveShortEvent(unsigned int Event) {
	if (!DrvShortEvBuf)
		return;

	auto NextWriteHead = DrvShortEvBuf->ReadHead + 1;
	if (NextWriteHead >= EvBufSize) NextWriteHead = 0;

	DrvShortEvBuf->Buf[DrvShortEvBuf->ReadHead].Event = Event;

	if (NextWriteHead != DrvShortEvBuf->ReadHead)
		DrvShortEvBuf->WriteHead = NextWriteHead;
}

unsigned int WinDriver::SynthPipe::SaveLongEvent(LPMIDIHDR Event) {
	if (!(Event->dwFlags & MHDR_PREPARED)) {
		NERROR(SynthErr, L"The MIDIHDR is not prepared.", false);
		return MIDIERR_UNPREPARED;
	}

	while (DrvLongEvBuf->Buf[DrvLongEvBuf->ReadHead].EventLength != 0)
		Sleep(1);

	Event->dwFlags &= ~MHDR_DONE;
	Event->dwFlags |= MHDR_INQUEUE;

	// Copy new buffer
	DrvLongEvBuf->Buf[DrvLongEvBuf->ReadHead].EventLength = Event->dwBufferLength;
	memcpy(DrvLongEvBuf->Buf[DrvLongEvBuf->ReadHead].Event, Event->lpData, Event->dwBufferLength);

	Event->dwFlags &= ~MHDR_INQUEUE;
	Event->dwFlags |= MHDR_DONE;

	return MMSYSERR_NOERROR;
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