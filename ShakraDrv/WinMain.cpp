/*
Shakra Driver component for Windows
This .cpp file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#include "WinMain.hpp"

static WinDriver::DriverComponent DriverComponent;
static WinDriver::DriverMask DriverMask;
static ErrorSystem DrvErr;
static BASS BASSSys;

typedef WINBASEAPI BOOL(WINAPI* fIW64P)(_In_ HANDLE, _Out_ PBOOL);
static bool isWow64Process() {
	BOOL IsUnderWOW64 = FALSE;
	const fIW64P IW64P = (fIW64P)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");

	if (IW64P)
		if (!IW64P(GetCurrentProcess(), &IsUnderWOW64))
			return false;

	return IsUnderWOW64;
}

bool __stdcall DllMain(HINSTANCE HinstanceDLL, DWORD fdwReason, LPVOID lpvR) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		if (!DisableThreadLibraryCalls(HinstanceDLL))
			return false;

		break;

	case DLL_PROCESS_DETACH:
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return true;
}

void __stdcall DriverRegistration(HWND HWND, HINSTANCE HinstanceDLL, LPSTR CommandLine, DWORD CmdShow) {
	// Check the argument sent by RunDLL32, and see what the heck it wants from us
	bool Registration = (_stricmp(CommandLine, "RegisterDrv") == 0), UnregisterPass = false;;

	// Used for registration
	HKEY DeviceRegKey, DriverSubKey, DriversSubKey;
	HDEVINFO DeviceInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	DEVPROPTYPE DevicePropertyType;
	WCHAR szBuffer[4096];
	DWORD dwSize, dwPropertyRegDataType;
	DWORD configClass = CONFIGFLAG_MANUAL_INSTALL | CONFIGFLAG_NEEDS_FORCED_CONFIG;

	// We need to register, woohoo
	if (_stricmp(CommandLine, "RegisterDrv") == 0 || _stricmp(CommandLine, "UnregisterDrv") == 0) {
		DeviceInfo = SetupDiGetClassDevs(&DevGUID, NULL, NULL, 0);

		if (DeviceInfo == INVALID_HANDLE_VALUE)
		{
			DERROR(DrvErr, L"SetupDiGetClassDevs returned a NULL struct, unable to register the driver.", true);
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
						DERROR(DrvErr, L"The driver is already registered!", false);
						return;
					}
					else 
					{
						DLOG(DrvErr, L"Got the item to delete, time to prepare for deletion.");
						UnregisterPass = true;
						break;
					}
				}
			}
		}

		if (!Registration && !UnregisterPass) 
		{
			DERROR(DrvErr, L"The driver is not registered!", false);
			return;
		}

		if (Registration) 
		{
			if (!SetupDiCreateDeviceInfo(DeviceInfo, DEVICE_NAME_MEDIA, &DevGUID, DEVICE_DESCRIPTION, NULL, DICD_GENERATE_ID, &DeviceInfoData)) {
				SetupDiDestroyDeviceInfoList(DeviceInfo);
				DERROR(DrvErr, L"SetupDiCreateDeviceInfo failed, unable to register the driver.", true);
				return;
			}

			if (!SetupDiRegisterDeviceInfo(DeviceInfo, &DeviceInfoData, 0, NULL, NULL, NULL)) {
				SetupDiDestroyDeviceInfoList(DeviceInfo);
				DERROR(DrvErr, L"SetupDiRegisterDeviceInfo failed, unable to register the driver.", true);
				return;
			}

			DeviceRegKey = SetupDiCreateDevRegKey(DeviceInfo, &DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
			SetupDiDestroyDeviceInfoList(DeviceInfo);

			if (DeviceRegKey == INVALID_HANDLE_VALUE)
			{
				DERROR(DrvErr, L"SetupDiCreateDevRegKey returned a NULL registry key, unable to register the driver.", true);
				return;
			}

			if (RegSetValueEx(DeviceRegKey, DRIVER_CLASS_PROP_DRIVER_DESC, NULL, REG_SZ, (LPBYTE)DEVICE_DESCRIPTION, sizeof(DEVICE_DESCRIPTION)) != ERROR_SUCCESS)
			{
				DERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_CLASS_PROP_DRIVER_DESC, unable to register the driver.", true);
				return;
			}
			
			if (RegSetValueEx(DeviceRegKey, DRIVER_CLASS_PROP_PROVIDER_NAME, NULL, REG_SZ, (LPBYTE)DRIVER_PROVIDER_NAME, sizeof(DRIVER_PROVIDER_NAME)) != ERROR_SUCCESS)
			{
				DERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_CLASS_PROP_PROVIDER_NAME, unable to register the driver.", true);
				return;
			}

			if (RegCreateKeyEx(DeviceRegKey, DRIVER_CLASS_SUBKEY_DRIVERS, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &DriversSubKey, NULL) != ERROR_SUCCESS)
			{
				DERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_CLASS_SUBKEY_DRIVERS, unable to register the driver.", true);
				return;
			}

			if (RegSetValueEx(DriversSubKey, DRIVER_CLASS_PROP_SUBCLASSES, NULL, REG_SZ, (LPBYTE)DRIVER_CLASS_SUBCLASSES, sizeof(DRIVER_CLASS_SUBCLASSES)) != ERROR_SUCCESS) 
			{
				DERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_CLASS_PROP_SUBCLASSES, unable to register the driver.", true);
				return;
			}

			if (RegCreateKeyEx(DriversSubKey, DRIVER_SUBCLASS_SUBKEYS, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &DriverSubKey, NULL) != ERROR_SUCCESS)
			{
				DERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_SUBCLASS_SUBKEYS, unable to register the driver.", true);
				return;
			}

			RegCloseKey(DriversSubKey);

			if (RegSetValueEx(DriverSubKey, DRIVER_SUBCLASS_PROP_DRIVER, NULL, REG_SZ, (LPBYTE)SHAKRA_DRIVER_NAME, sizeof(SHAKRA_DRIVER_NAME)) != ERROR_SUCCESS) 
			{
				DERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_SUBCLASS_PROP_DRIVER, unable to register the driver.", true);
				return;
			}

			if (RegSetValueEx(DriverSubKey, DRIVER_SUBCLASS_PROP_DESCRIPTION, NULL, REG_SZ, (LPBYTE)DEVICE_DESCRIPTION, sizeof(DEVICE_DESCRIPTION)) != ERROR_SUCCESS)
			{
				DERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_SUBCLASS_PROP_DESCRIPTION, unable to register the driver.", true);
				return;
			}

			MIDIRegEntryName EntryName;
			int MIDIEntry = 0;
			const wchar_t* ShakraEntryName = EntryName.withIndex(MIDIEntry)->ToString();
			if (RegSetValueEx(DriverSubKey, DRIVER_SUBCLASS_PROP_ALIAS, NULL, REG_SZ, (LPBYTE)ShakraEntryName, sizeof(MIDI_REGISTRY_ENTRY_TEMPLATE)) != ERROR_SUCCESS)
			{
				DERROR(DrvErr, L"RegSetValueEx failed to write DRIVER_SUBCLASS_PROP_ALIAS, unable to register the driver.", true);
				return;
			}

			RegCloseKey(DeviceRegKey);

			MessageBox(HWND, L"Driver successfully registered!", L"Shakra - Success", MB_OK | MB_ICONINFORMATION);
		}
		else {
			if (!SetupDiRemoveDevice(DeviceInfo, &DeviceInfoData))
			{
				SetupDiDestroyDeviceInfoList(DeviceInfo);
				DERROR(DrvErr, L"SetupDiRemoveDevice failed, unable to unregister the driver.", true);
				return;
			}

			SetupDiDestroyDeviceInfoList(DeviceInfo);
			MessageBox(HWND, L"Driver successfully unregistered!", L"Shakra - Success", MB_OK | MB_ICONINFORMATION);
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

long __stdcall DriverProc(DWORD DriverIdentifier, HDRVR DriverHandle, UINT Message, LONG Param1, LONG Param2) {
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
	switch (Message) {
	case MODM_DATA:
		return MMSYSERR_NOERROR;

	case MODM_LONGDATA:
		DriverComponent.CallbackFunction(MOM_DONE, Param1, 0);
		return MMSYSERR_NOERROR;

	case MODM_PREPARE:
		DriverComponent.CallbackFunction(MOM_DONE, Param1, 0);
		return MMSYSERR_NOERROR;

	case MODM_UNPREPARE:
		DriverComponent.CallbackFunction(MOM_DONE, Param1, 0);
		return MMSYSERR_NOERROR;

	case MODM_RESET:
	case MODM_GETVOLUME:
	case MODM_SETVOLUME:
		return MMSYSERR_NOERROR;

	case MODM_OPEN:
		// Open the driver, and if everything goes fine, inform the app through a callback
		if (DriverComponent.OpenDriver((LPMIDIOPENDESC)Param1, (DWORD)Param2, DriverAddress)) {
			// Driver is busy in MODM_OPEN, reject any other MODM_OPEN/MODM_CLOSE call for the time being
			DriverBusy = true;

			// Initialize BASS
			BASSSys = BASS(-1, 0, BASS_DEVICE_DEFAULT);

			if (BASSSys.Fail()) {
				// Something went wrong, the driver failed to open
				DERROR(DrvErr, L"The driver did open, but it wasn't able to complete the BASS initialization process.", false);
				DriverComponent.CloseDriver();
				return MMSYSERR_ERROR;
			}

			DriverComponent.CallbackFunction(MOM_OPEN, 0, 0);
			DLOG(DrvErr, L"The driver has been opened.");

			DriverBusy = false;
			return MMSYSERR_NOERROR;
		}

		// Something went wrong, the driver failed to open
		DERROR(DrvErr, L"Failed to open driver.", false);
		return MIDIERR_NOTREADY;

	case MODM_CLOSE:
		// The driver isn't done opening the stream
		if (DriverBusy) {
			DLOG(DrvErr, L"Can't accept MODM_CLOSE while the driver is busy in MODM_OPEN!");
			return MIDIERR_NOTREADY;
		}

		if (DriverComponent.CloseDriver()) {
			BASSSys.~BASS();
			DriverComponent.CallbackFunction(MOM_CLOSE, 0, 0);

			DLOG(DrvErr, L"The driver has been closed.");
			return MMSYSERR_NOERROR;
		}

		return MMSYSERR_ERROR;

	case MODM_GETNUMDEVS:
		return 0x1;

	case MODM_GETDEVCAPS:
		return DriverMask.GiveCaps((PVOID)Param1, (DWORD)Param2);

	case MODM_CACHEPATCHES:
	case MODM_CACHEDRUMPATCHES:
	case DRV_QUERYDEVICEINTERFACESIZE:
	case DRV_QUERYDEVICEINTERFACE:
		return MMSYSERR_NOERROR;

	default:
		return MMSYSERR_ERROR;
	}
}