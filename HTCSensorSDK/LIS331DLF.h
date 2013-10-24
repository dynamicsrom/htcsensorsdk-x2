// File: LIS331DLF.h
// Description: 
// Control codes for LIS331DLF accelerometer found in Sony Ericsson XPERIA X2
#pragma once

#define IOCTL_ACCEL_ENABLE_HW 0x80000004
#define IOCTL_ACCEL_DISABLE_HW 0x80000008
#define IOCTL_ACCEL_READ_VALUE 0x8000000C

typedef struct
{
	int x;
	int y;
	int z;
}LIS331DLF_XYZ;

#define ACCELRESULT_DEBUG 2

