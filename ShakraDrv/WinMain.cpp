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

BOOL WINAPI DllMain(HINSTANCE HinstanceDLL, DWORD fdwReason, LPVOID lpvR) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		if (!DisableThreadLibraryCalls(HinstanceDLL))
			return FALSE;

		break;

	case DLL_PROCESS_DETACH:
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}

LONG WINAPI DriverProc(DWORD DriverIdentifier, HDRVR DriverHandle, UINT Message, LONG Param1, LONG Param2) {
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

MMRESULT modMessage(UINT DeviceIdentifier, UINT Message, DWORD_PTR DriverAddress, DWORD_PTR Param1, DWORD_PTR Param2) {
	switch (Message) {
	case MODM_DATA:
		return MMSYSERR_NOERROR;

	case MODM_LONGDATA:
		DriverComponent.CallbackFunction(MM_MOM_DONE, Param1, 0);
		return MMSYSERR_NOERROR;

	case MODM_PREPARE:
		DriverComponent.CallbackFunction(MM_MOM_DONE, Param1, 0);
		return MMSYSERR_NOERROR;

	case MODM_UNPREPARE:
		DriverComponent.CallbackFunction(MM_MOM_DONE, Param1, 0);
		return MMSYSERR_NOERROR;

	case MODM_RESET:
	case MODM_GETVOLUME:
	case MODM_SETVOLUME:
		return MMSYSERR_NOERROR;

	case MODM_OPEN:
		// Open the driver, and if everything goes fine, inform the app through a callback
		if (DriverComponent.OpenDriver((LPMIDIOPENDESC)Param1, (DWORD)Param2, DriverAddress)) {
			// Initialize BASS
			BASSSys = BASS(-1, 0, BASS_DEVICE_DEFAULT);

			DriverComponent.CallbackFunction(MM_MOM_OPEN, 0, 0);

			DLOG(DrvErr, L"The driver has been opened.");
			return MMSYSERR_NOERROR;
		}

		// Something went wrong, the driver failed to open
		DERROR(DrvErr, L"Failed to open driver.", false);

		return MIDIERR_NOTREADY;

	case MODM_CLOSE:
		if (DriverComponent.CloseDriver()) {
			DriverComponent.CallbackFunction(MM_MOM_CLOSE, 0, 0);

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