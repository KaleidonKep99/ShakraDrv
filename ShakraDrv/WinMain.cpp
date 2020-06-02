/*
Shakra Driver component for Windows
This .cpp file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#include "WinDriver.h"
#include "WinError.h"

using namespace std;

static WinDriver::DriverComponent Driver;

LONG WINAPI DriverProc(DWORD dwDriverIdentifier, HDRVR HDRVR, UINT uMessage, LONG lParam1, LONG lParam2) {
	switch (uMessage) {
	case DRV_OPEN:
		return Driver.SetDriverHandle(HDRVR) ? DRV_OK : DRV_CANCEL;
	case DRV_CLOSE:
		return Driver.UnsetDriverHandle() ? DRV_OK : DRV_CANCEL;
	case DRV_CONFIGURE:
	case DRV_QUERYCONFIGURE:
	case DRV_LOAD:
	case DRV_FREE:
		return DRV_OK;
	default:
		return DefDriverProc(dwDriverIdentifier, HDRVR, uMessage, lParam1, lParam2);
	}
}

MMRESULT modMessage(UINT uDeviceID, UINT uMessage, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	switch (uMessage) {
	default:
		return MMSYSERR_NOERROR;
	}
}