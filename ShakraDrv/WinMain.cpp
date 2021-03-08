/*
Shakra Driver component for Windows
This .cpp file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

// Debug

#include "WinMain.hpp"

// Win32 components
static WinDriver::DriverComponent DriverComponent;
static WinDriver::DriverCallback* DriverAppCallback;
static WinDriver::DriverMask* DriverMask;

// Synth components
static WinDriver::SynthPipe SynthSys;

// Error handler
static ErrorSystem::WinErr DrvErr;

bool __stdcall DllMain(HINSTANCE HinstanceDLL, DWORD fdwReason, LPVOID lpvR) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		if (!DisableThreadLibraryCalls(HinstanceDLL))
			return false;

		if (DriverAppCallback == nullptr) {
			DriverAppCallback = new (std::nothrow) WinDriver::DriverCallback[MAX_DRIVERS];

			if (DriverAppCallback == nullptr) {
				FNERROR(DrvErr, L"DriverCallback returned nullptr. Can not continue!");
				return NULL;
			}
		}

		if (DriverMask == nullptr) {
			DriverMask = new (std::nothrow) WinDriver::DriverMask[MAX_DRIVERS];

			if (DriverMask == nullptr) {
				FNERROR(DrvErr, L"DriverMask returned nullptr. Can not continue!");
				return NULL;
			}
		}

		break;

	case DLL_PROCESS_DETACH:
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return true;
}

static bool isWow64Process() {
	BOOL IsUnderWOW64 = false;
	const fIW64P IW64P = (fIW64P)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");

	if (IW64P)
		if (!IW64P(GetCurrentProcess(), &IsUnderWOW64))
			return false;

	return IsUnderWOW64;
}

void DriverRegistration(HWND HWND, LPSTR CommandLine, bool CmdShow) {
	// Check the argument sent by RunDLL32, and see what the heck it wants from us
	bool Registration = (_stricmp(CommandLine, "RegisterDrv") == 0), UnregisterPass = false;

	// Used for registration
	HKEY	DeviceRegKey,
			DriverSubKey,
			DriversSubKey,
#ifdef _WIN64
			Drivers32Key,
			DriversWOW64Key;
#else
			Drivers32Key;
#endif

	HDEVINFO DeviceInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	wchar_t szBuffer[4096];
	DWORD dwSize, dwPropertyRegDataType, dummy;
	DWORD configClass = CONFIGFLAG_MANUAL_INSTALL | CONFIGFLAG_NEEDS_FORCED_CONFIG;

	// Drivers32 Shakra
	wchar_t Buf[255];
	wchar_t ShakraKey[255];
	DWORD bufSize = sizeof(Buf);
	DWORD shakraSize = sizeof(ShakraKey);
	bool OnlyDrivers32 = true;

	// If user is not an admin, abort.
	if (!IsUserAnAdmin()) 
	{
		NERROR(DrvErr, L"You can not manage the driver without administration rights.", true);
		return;
	}

	// If it's running under WoW64 emulation, tell the user to use the x64 DLL under RunDLL32 instead
	if (isWow64Process()) 
	{
		NERROR(DrvErr, L"You can not register the driver using the 32-bit library under 64-bit Windows.\n\nPlease use the 64-bit library.", true);
		return;
	}

	// Open Drivers32 key
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &Drivers32Key, &dummy) != ERROR_SUCCESS) {
		NERROR(DrvErr, L"RegCreateKeyEx failed to open the Drivers32 key, unable to (un)register the driver.", true);
		return;
	}

#ifdef _WIN64
	// Open Drivers32 for WOW64 key
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_32KEY, NULL, &DriversWOW64Key, &dummy) != ERROR_SUCCESS) {
		NERROR(DrvErr, L"RegCreateKeyEx failed to open the Drivers32 key from the WOW64 hive, unable to (un)register the driver.", true);
		return;
	}
#endif

	// We need to register, woohoo
	if (_stricmp(CommandLine, "RegisterDrv") == 0 || _stricmp(CommandLine, "UnregisterDrv") == 0) {
		DeviceInfo = SetupDiGetClassDevs(&DevGUID, NULL, NULL, 0);

		if (DeviceInfo == INVALID_HANDLE_VALUE)
		{
			NERROR(DrvErr, L"SetupDiGetClassDevs returned a NULL struct, unable to register the driver.", true);
			return;
		}
		
		// Die Windows 10
		for (int DeviceIndex = 0; ; DeviceIndex++) {
			ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
			ZeroMemory(&szBuffer, sizeof(szBuffer));
			DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

			if (!SetupDiEnumDeviceInfo(DeviceInfo, DeviceIndex, &DeviceInfoData))
				break;

			if (SetupDiGetDeviceRegistryProperty(
				DeviceInfo, 
				&DeviceInfoData,
				SPDRP_DEVICEDESC,
				&dwPropertyRegDataType, 
				(BYTE*)szBuffer, 
				sizeof(szBuffer),
				&dwSize)) 
			{
				if (_wcsicmp(szBuffer, DEVICE_DESCRIPTION) == 0)
				{
					if (Registration)
					{
						NERROR(DrvErr, L"The driver is already registered!", false);
						return;
					}
					else 
					{
						LOG(DrvErr, L"Got the item to delete, time to prepare for deletion.");
						UnregisterPass = true;
						OnlyDrivers32 = false;
						break;
					}
				}
			}
		}
		
		// Normal check for 8.1 and lower
		for (int MIDIEntry = 1; MIDIEntry < 32; MIDIEntry++)
		{
#ifdef _WIN64
			int Step = 0;

			if (Step > 1) break;
#endif

			ZeroMemory(Buf, sizeof(Buf));
			ZeroMemory(ShakraKey, sizeof(ShakraKey));

			swprintf_s(ShakraKey, MIDI_REGISTRY_ENTRY_TEMPLATE, MIDIEntry);
			LOG(DrvErr, L"Copied MIDI_REGISTRY_ENTRY_TEMPLATE with correct value to ShakraKey.");

#ifdef _WIN64
			if (Step == 0)
			{
#endif
				if (RegQueryValueExW(Drivers32Key, ShakraKey, 0, 0, reinterpret_cast<LPBYTE>(&Buf), &bufSize) == ERROR_SUCCESS)
				{
					LOG(DrvErr, L"Driver has been found in the Drivers32 subkey.");

					if (_wcsicmp(Buf, DRIVER_SUBCLASS_SUBKEYS) == 0)
					{
						UnregisterPass = true;
#ifdef _WIN64
						Step++;
						MIDIEntry = 1;
#else
						break;
#endif
					}
				}
				else continue;
#ifdef _WIN64
			}
			else 
			{
				if (RegQueryValueExW(DriversWOW64Key, ShakraKey, 0, 0, reinterpret_cast<LPBYTE>(&Buf), &bufSize) == ERROR_SUCCESS)
				{
					LOG(DrvErr, L"Driver has been found in the Drivers32 subkey (WOW64 hive).");

					if (_wcsicmp(Buf, DRIVER_SUBCLASS_SUBKEYS) == 0)
					{
						UnregisterPass = true;
						Step++;
					}
				}
				else continue;
			}
#endif
		}

		if (!Registration && !UnregisterPass) 
		{
			NERROR(DrvErr, L"The driver is not registered!", false);
			return;
		}

		if (Registration) 
		{
			if (!SetupDiCreateDeviceInfo(DeviceInfo, DEVICE_NAME_MEDIA, &DevGUID, DEVICE_DESCRIPTION, NULL, DICD_GENERATE_ID, &DeviceInfoData)) 
			{
				SetupDiDestroyDeviceInfoList(DeviceInfo);
				NERROR(DrvErr, L"SetupDiCreateDeviceInfo failed, unable to register the driver.", true);
				return;
			}

			if (!SetupDiRegisterDeviceInfo(DeviceInfo, &DeviceInfoData, NULL, NULL, NULL, NULL)) {
				SetupDiDestroyDeviceInfoList(DeviceInfo);
				NERROR(DrvErr, L"SetupDiRegisterDeviceInfo failed, unable to register the driver.", true);
				return;
			}

			if (!SetupDiSetDeviceRegistryProperty(DeviceInfo, &DeviceInfoData, SPDRP_CONFIGFLAGS, (BYTE*)&configClass, sizeof(configClass))) {
				SetupDiDestroyDeviceInfoList(DeviceInfo);
				NERROR(DrvErr, L"SetupDiSetDeviceRegistryProperty failed, unable to register the driver.", true);
				return;
			}

			if (!SetupDiSetDeviceRegistryProperty(DeviceInfo, &DeviceInfoData, SPDRP_MFG, (BYTE*)&DRIVER_PROVIDER_NAME, sizeof(DRIVER_PROVIDER_NAME))) {
				SetupDiDestroyDeviceInfoList(DeviceInfo);
				NERROR(DrvErr, L"SetupDiSetDeviceRegistryProperty failed, unable to register the driver.", true);
				return;
			}

			DeviceRegKey = SetupDiCreateDevRegKey(DeviceInfo, &DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
			SetupDiDestroyDeviceInfoList(DeviceInfo);

			if (DeviceRegKey == INVALID_HANDLE_VALUE)
			{
				NERROR(DrvErr, L"SetupDiCreateDevRegKey returned a NULL registry key, unable to register the driver.", true);
				return;
			}

			if (RegSetValueEx(DeviceRegKey, DRIVER_CLASS_PROP_DRIVER_DESC, NULL, REG_SZ, (LPBYTE)DEVICE_DESCRIPTION, sizeof(DEVICE_DESCRIPTION)) != ERROR_SUCCESS)
			{
				NERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_CLASS_PROP_DRIVER_DESC, unable to register the driver.", true);
				return;
			}
			
			if (RegSetValueEx(DeviceRegKey, DRIVER_CLASS_PROP_PROVIDER_NAME, NULL, REG_SZ, (LPBYTE)DRIVER_PROVIDER_NAME, sizeof(DRIVER_PROVIDER_NAME)) != ERROR_SUCCESS)
			{
				NERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_CLASS_PROP_PROVIDER_NAME, unable to register the driver.", true);
				return;
			}

			if (RegCreateKeyEx(DeviceRegKey, DRIVER_CLASS_SUBKEY_DRIVERS, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &DriversSubKey, NULL) != ERROR_SUCCESS)
			{
				NERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_CLASS_SUBKEY_DRIVERS, unable to register the driver.", true);
				return;
			}

			if (RegSetValueEx(DriversSubKey, DRIVER_CLASS_PROP_SUBCLASSES, NULL, REG_SZ, (LPBYTE)DRIVER_CLASS_SUBCLASSES, sizeof(DRIVER_CLASS_SUBCLASSES)) != ERROR_SUCCESS) 
			{
				NERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_CLASS_PROP_SUBCLASSES, unable to register the driver.", true);
				return;
			}

			if (RegCreateKeyEx(DriversSubKey, DRIVER_SUBCLASS_SUBKEYS, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &DriverSubKey, NULL) != ERROR_SUCCESS)
			{
				NERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_SUBCLASS_SUBKEYS, unable to register the driver.", true);
				return;
			}

			if (RegSetValueEx(DriverSubKey, DRIVER_SUBCLASS_PROP_DRIVER, NULL, REG_SZ, (LPBYTE)SHAKRA_DRIVER_NAME, sizeof(SHAKRA_DRIVER_NAME)) != ERROR_SUCCESS)
			{
				NERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_SUBCLASS_PROP_DRIVER, unable to register the driver.", true);
				return;
			}

			if (RegSetValueEx(DriverSubKey, DRIVER_SUBCLASS_PROP_DESCRIPTION, NULL, REG_SZ, (LPBYTE)DEVICE_DESCRIPTION, sizeof(DEVICE_DESCRIPTION)) != ERROR_SUCCESS)
			{
				NERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_SUBCLASS_PROP_DESCRIPTION, unable to register the driver.", true);
				return;
			}

			for (int MIDIEntry = 1; MIDIEntry < 32; MIDIEntry++) 
			{
				LSTATUS Drv32 = ERROR_SUCCESS, DrvWOW64 = ERROR_SUCCESS;

				ZeroMemory(Buf, bufSize);
				ZeroMemory(ShakraKey, shakraSize);

				swprintf_s(ShakraKey, MIDI_REGISTRY_ENTRY_TEMPLATE, MIDIEntry);
				LOG(DrvErr, L"Copied MIDI_REGISTRY_ENTRY_TEMPLATE with correct value to ShakraKey.");
				
				Drv32 = RegQueryValueExW(Drivers32Key, ShakraKey, 0, 0, reinterpret_cast<LPBYTE>(&Buf), &bufSize);
#ifdef _WIN64
				DrvWOW64 = RegQueryValueExW(DriversWOW64Key, ShakraKey, 0, 0, reinterpret_cast<LPBYTE>(&Buf), &bufSize);
#endif

				if (Drv32 != ERROR_SUCCESS || DrvWOW64 != ERROR_SUCCESS)
				{
					LOG(DrvErr, L"Key does not exist, let's create it.");

					if (Drv32 != ERROR_SUCCESS)
					{
						if (RegSetValueEx(Drivers32Key, ShakraKey, NULL, REG_SZ, (LPBYTE)SHAKRA_DRIVER_NAME, sizeof(SHAKRA_DRIVER_NAME)) != ERROR_SUCCESS)
						{
							NERROR(DrvErr, L"RegSetValueEx failed to write the MIDI alias to Drivers32, unable to register the driver.", true);
							return;
						}
					}

#ifdef _WIN64
					if (DrvWOW64 != ERROR_SUCCESS)
					{
						if (RegSetValueEx(DriversWOW64Key, ShakraKey, NULL, REG_SZ, (LPBYTE)SHAKRA_DRIVER_NAME, sizeof(SHAKRA_DRIVER_NAME)) != ERROR_SUCCESS)
						{
							NERROR(DrvErr, L"RegSetValueEx failed to write the MIDI alias to Drivers32 in the WOW64 hive, unable to register the driver.", true);
							return;
						}
					}
#endif

					if (RegSetValueEx(DriverSubKey, DRIVER_SUBCLASS_PROP_ALIAS, NULL, REG_SZ, (LPBYTE)ShakraKey, shakraSize) != ERROR_SUCCESS)
					{
						NERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_SUBCLASS_PROP_ALIAS, unable to register the driver.", true);
						return;
					}

					break;
				}
				else 
				{
					if (_wcsicmp(Buf, DRIVER_SUBCLASS_SUBKEYS) == 0)
					{
						NERROR(DrvErr, L"The driver has been already registered!", false);
						return;
					}

					LOG(DrvErr, Buf);
				}
			}

			RegCloseKey(Drivers32Key);
#ifdef _WIN64
			RegCloseKey(DriversWOW64Key);
#endif
			RegCloseKey(DeviceRegKey);
			RegCloseKey(DriversSubKey);

			if (CmdShow) MessageBox(HWND, L"Driver successfully registered!", L"Shakra - Success", MB_OK | MB_ICONINFORMATION);
		}
		else 
		{
			if (!OnlyDrivers32) 
			{
				if (!SetupDiRemoveDevice(DeviceInfo, &DeviceInfoData))
				{
					SetupDiDestroyDeviceInfoList(DeviceInfo);
					NERROR(DrvErr, L"SetupDiRemoveDevice failed, unable to unregister the driver.", true);
					return;
				}
			}

			for (int MIDIEntry = 1; MIDIEntry < 32; MIDIEntry++)
			{
				ZeroMemory(Buf, sizeof(Buf));
				ZeroMemory(ShakraKey, sizeof(ShakraKey));

				swprintf_s(ShakraKey, MIDI_REGISTRY_ENTRY_TEMPLATE, MIDIEntry);
				LOG(DrvErr, L"Copied MIDI_REGISTRY_ENTRY_TEMPLATE with correct value to ShakraKey.");

				if (RegQueryValueExW(Drivers32Key, ShakraKey, 0, 0, reinterpret_cast<LPBYTE>(&Buf), &bufSize) == ERROR_SUCCESS)
				{
					LOG(DrvErr, L"Driver has been found in the Drivers32 subkey.");

					if (_wcsicmp(Buf, DRIVER_SUBCLASS_SUBKEYS) == 0)
					{
						RegDeleteValue(Drivers32Key, ShakraKey);
						LOG(DrvErr, L"Driver has been removed from the Drivers32 subkey.");
					}
				}

#ifdef _WIN64
				if (RegQueryValueExW(DriversWOW64Key, ShakraKey, 0, 0, reinterpret_cast<LPBYTE>(&Buf), &bufSize) == ERROR_SUCCESS)
				{
					LOG(DrvErr, L"Driver has been found in the Drivers32 subkey (WOW64 hive).");

					if (_wcsicmp(Buf, DRIVER_SUBCLASS_SUBKEYS) == 0)
					{
						RegDeleteValue(DriversWOW64Key, ShakraKey);
						LOG(DrvErr, L"Driver has been removed from the Drivers32 subkey (WOW64 hive).");
					}
				}
#endif
			}

			RegCloseKey(Drivers32Key);
#ifdef _WIN64
			RegCloseKey(DriversWOW64Key);
#endif
			SetupDiDestroyDeviceInfoList(DeviceInfo);

			if (CmdShow) MessageBox(HWND, L"Driver successfully unregistered!", L"Shakra - Success", MB_OK | MB_ICONINFORMATION);
		}

		return;
	}

	// I have no idea?
	else {
		MessageBoxA(
			HWND,
			"RunDLL32 sent an empty or unrecognized command line.\n\nThe driver doesn't know what to do, so press OK to quit.",
			"Shakra", MB_ICONERROR | MB_OK);
		return;
	}
}

void __stdcall DriverReg(HWND HWND, HINSTANCE HinstanceDLL, LPSTR CommandLine, DWORD CmdShow) {
	DriverRegistration(HWND, CommandLine, true);
}

void __stdcall SilentDriverReg(HWND HWND, HINSTANCE HinstanceDLL, LPSTR CommandLine, DWORD CmdShow) {
	DriverRegistration(HWND, CommandLine, false);
}

LRESULT __stdcall DriverProc(DWORD DriverIdentifier, HDRVR DriverHandle, UINT Message, LONG Param1, LONG Param2) {
	switch (Message) {
	case DRV_LOAD:
		return DriverComponent.SetDriverHandle(DriverHandle);
	case DRV_FREE:
		return DriverComponent.UnsetDriverHandle();

	case DRV_OPEN:
	case DRV_CLOSE:
		return DRVCNF_OK;

	case DRV_QUERYCONFIGURE:
		return DRVCNF_OK;

	case DRV_ENABLE:
	case DRV_DISABLE:
		return DRVCNF_OK;

	case DRV_INSTALL:
		return DRVCNF_OK;

	case DRV_REMOVE:
		return DRVCNF_OK;
	}

	return DefDriverProc(DriverIdentifier, DriverHandle, Message, Param1, Param2);
}

unsigned int modMessage(UINT DeviceIdentifier, UINT Message, DWORD_PTR DriverAddress, DWORD_PTR Param1, DWORD_PTR Param2) {
	unsigned int modM = MMSYSERR_NOERROR;

	switch (Message) {
	case MODM_DATA:
		SynthSys.SaveShortEvent(DeviceIdentifier, (DWORD)Param1);
		return MMSYSERR_NOERROR;

	case MODM_LONGDATA:
		modM = SynthSys.SaveLongEvent(DeviceIdentifier, (MIDIHDR*)Param1);
		DriverAppCallback[DeviceIdentifier].CallbackFunction(MOM_DONE, Param1, 0);
		return modM;

	case MODM_PREPARE:
		modM = SynthSys.PrepareLongEvent(DeviceIdentifier, (MIDIHDR*)Param1);
		return modM;

	case MODM_UNPREPARE:
		modM = SynthSys.UnprepareLongEvent(DeviceIdentifier, (MIDIHDR*)Param1);
		return modM;

	case MODM_RESET:
	case MODM_GETVOLUME:
	case MODM_SETVOLUME:
		return MMSYSERR_NOERROR;

	case MODM_OPEN:
		// Open the driver, and if everything goes fine, inform the app through a callback
		if (DriverComponent.OpenDriver((LPMIDIOPENDESC)Param1, (DWORD)Param2, DriverAddress)) {
			// Driver is busy in MODM_OPEN, reject any other MODM_OPEN/MODM_CLOSE call for the time being
			DriverBusy = true;

			if (!SynthSys.PrepareFileMappings(DeviceIdentifier, false, 0)) {
				// Something went wrong, the driver failed to open
				NERROR(DrvErr, L"Failed to open driver.", false);
				DriverComponent.CloseDriver();
				return MMSYSERR_ERROR;
			}

			DriverAppCallback[DeviceIdentifier].PrepareCallbackFunction((LPMIDIOPENDESC)Param1, (DWORD)Param2);
			DriverAppCallback[DeviceIdentifier].CallbackFunction(MOM_OPEN, 0, 0);

			DriverBusy = false;
			return MMSYSERR_NOERROR;
		}

		// Something went wrong, the driver failed to open
		NERROR(DrvErr, L"Failed to open driver.", false);
		return MIDIERR_NOTREADY;

	case MODM_CLOSE:
		// The driver isn't done opening the stream
		if (DriverBusy) {
			LOG(DrvErr, L"Can't accept MODM_CLOSE while the driver is busy in MODM_OPEN!");
			return MIDIERR_NOTREADY;
		}

		SynthSys.ClosePipe(DeviceIdentifier);
		DriverAppCallback[DeviceIdentifier].CallbackFunction(MOM_CLOSE, NULL, NULL);
		DriverAppCallback[DeviceIdentifier].ClearCallbackFunction();

		return MMSYSERR_NOERROR;

	case MODM_GETNUMDEVS:
		return 0x1;

	case MODM_GETDEVCAPS:
		return DriverMask[DeviceIdentifier].GiveCaps((PVOID)Param1, (DWORD)Param2);

	case MODM_CACHEPATCHES:
	case MODM_CACHEDRUMPATCHES:
	case DRV_QUERYDEVICEINTERFACESIZE:
	case DRV_QUERYDEVICEINTERFACE:
		return MMSYSERR_NOERROR;

	default:
		return MMSYSERR_ERROR;
	}
}

//
// USED INTERNALLY BY SHAKRA HOST
//

bool WINAPI SH_CP(unsigned short PipeID, int Size) {
	return SynthSys.PrepareFileMappings(PipeID, true, Size);
}

unsigned int WINAPI SH_PSE(unsigned short PipeID) {
	return SynthSys.ParseShortEvent(PipeID);
}

unsigned int WINAPI SH_PLE(unsigned short PipeID, BYTE* PEvent) {
	return SynthSys.ParseLongEvent(PipeID, PEvent);
}

void WINAPI SH_RRHIN(unsigned short PipeID) {
	SynthSys.ResetReadHeadsIfNeeded(PipeID);
}

void WINAPI SH_GRHP(unsigned short PipeID, int *SRH, int *LRH) {
	SynthSys.GetReadHeadPos(PipeID, SRH, LRH);
}

void WINAPI SH_GWHP(unsigned short PipeID, int* SWH, int* LWH) {
	SynthSys.GetWriteHeadPos(PipeID, SWH, LWH);
}

bool WINAPI SH_BC(unsigned short PipeID) {
	return SynthSys.PerformBufferCheck(PipeID);
}

//
// KDMAPI
//

int WINAPI InitializeKDMAPIStream() {
	if (!SynthSys.PrepareFileMappings(0, false, 0)) {
		// Something went wrong, the driver failed to open
		NERROR(DrvErr, L"Failed to open driver.", false);
		DriverComponent.CloseDriver();
		return FALSE;
	}
	
	return TRUE;
}

int WINAPI TerminateKDMAPIStream() {
	if (SynthSys.ClosePipe(0)) {
		NERROR(DrvErr, L"Failed to close driver.", false);
		return FALSE;
	}

	return TRUE;
}

uint32_t WINAPI SendDirectData(unsigned long Message) {
	SynthSys.SaveShortEvent(0, Message);
	return MMSYSERR_NOERROR;
}

int WINAPI IsKDMAPIAvailable() {
	return TRUE;
}