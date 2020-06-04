/*
Shakra Driver component for Windows
This .cpp file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#include "WinDriver.hpp"
#include "WinError.hpp"

static WinDriver::DriverComponent DriverComponent;
static ErrorSystem DrvErr;

LONG WINAPI DriverProc(DWORD DriverIdentifier, HDRVR HDRVR, UINT Message, LONG Param1, LONG Param2) {
	switch (Message) {
	case DRV_OPEN:
		return DriverComponent.SetDriverHandle(HDRVR) ? DRVCNF_OK : DRVCNF_CANCEL;
	case DRV_CLOSE:
		return DriverComponent.UnsetDriverHandle() ? DRVCNF_OK : DRVCNF_CANCEL;
	case DRV_CONFIGURE:
	case DRV_QUERYCONFIGURE:
	case DRV_LOAD:
	case DRV_FREE:
		return DRV_OK;
	default:
		return DefDriverProc(DriverIdentifier, HDRVR, Message, Param1, Param2);
	}
}

MMRESULT modMessage(UINT DeviceIdentifier, UINT Message, DWORD_PTR DriverAddress, DWORD_PTR Param1, DWORD_PTR Param2) {
	switch (Message) {
	case MODM_OPEN:
		// Open the driver, and if everything goes fine, inform the app through a callback
		if (DriverComponent.OpenDriver((LPMIDIOPENDESC)Param1, (DWORD)Param2, DriverAddress)) {
			DriverComponent.CallbackFunction(MOM_OPEN, 0, 0);
			return MMSYSERR_NOERROR;
		}

		// Something went wrong, the driver failed to open
		DrvErr.ThrowError(F, L"Failed to open driver.", false);
		return MMSYSERR_NOTENABLED;

	default:
		return MMSYSERR_NOERROR;
	}
}