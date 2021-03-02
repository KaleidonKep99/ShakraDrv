/*
Shakra Driver component for Windows
This .cpp file contains the required code to run the driver under Windows 8.1 and later.

This file is useful only if you want to compile the driver under Windows, it's not needed for Linux/macOS porting.
*/

#pragma once

// This file contains all the global vars that are used by Shakra

#define MAX_DRIVERS		4

#define MAX_MIDIHDR_BUF	256
#define MIDIHDR_WRITTEN	29