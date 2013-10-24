// File: HTCSensorSDK.cpp
// Description: 
// Implementation for HTCSensorSDK emulation library for Sony Ericsson XPERIA X2
// Made by UltraShot
// Based on code made by in4m and radcapricorn, etenclub.ru
// TODO: 
// -LightSensor implementation

#include "stdafx.h"
#include <math.h>
#include "lis331dlf.h"
#include "htclib.h"
#include "regext.h"

// define it only if you want to test it on real X2 (I've tested everything on X1 via registry)
#define REAL_DEVICE

#ifndef REAL_DEVICE
	#define DEBUG
#endif

#ifdef DEBUG
	#define DBGMESSAGE(fmt, ...) \
	{ FILE* f = fopen("\\HTCSensor.log", "at"); \
		fprintf(f, "[%s:%d]\t\t: "fmt"\n", __FILE__, __LINE__, __VA_ARGS__); \
		fclose(f); }

#else
	#define DBGMESSAGE(fmt, ...)
#endif

int AccelEnable(HANDLE device)
{
#ifdef REAL_DEVICE
	DWORD bytesCopied = 0;
	RegistrySetDWORD(HKEY_LOCAL_MACHINE,L"Services\\MultiService\\mods\\Rotation", L"OverrideCounter", 30);
	return DeviceIoControl(device, IOCTL_ACCEL_ENABLE_HW, 0, 0, 0, 0, &bytesCopied, 0);
#else
	return ACCELRESULT_DEBUG;
#endif
}

int AccelDisable(HANDLE device)
{
#ifdef REAL_DEVICE
	DWORD bytesCopied = 0;
	RegistrySetDWORD(HKEY_LOCAL_MACHINE,L"Services\\MultiService\\mods\\Rotation", L"OverrideCounter", 0);
	return DeviceIoControl(device, IOCTL_ACCEL_DISABLE_HW, 0, 0, 0, 0, &bytesCopied, 0);
#else
	return ACCELRESULT_DEBUG;
#endif
}


HTC_EXPORT HTCHANDLE WINAPI HTCSensorOpen(DWORD nSensorID)
{
	HTCHandle_t* pHandle = 0;
	switch (nSensorID) {
		case HTC_TILT_SENSOR: {
			pHandle = (HTCHandle_t*)malloc(sizeof(HTCHandle_t));
			pHandle->Signature = HTC_DEVICE_SIGNATURE;
			pHandle->SensorType = nSensorID;
			HANDLE lis331dlf = CreateFileW(L"ACC1:",0xC0000000,3,0,3,0,0);
			pHandle->hDevice = lis331dlf;
			int result = AccelEnable(lis331dlf);
			DBGMESSAGE("HTCSensorOpen: TILT. Result = %d (%ls)", result, result == ACCELRESULT_DEBUG ? L"Debug" : L"Real");
		} break;
		default:
			DBGMESSAGE("Attempt to open sensor other than tilt");
			break;
	}
	DBGMESSAGE("HTCSensorOpen result: %x", pHandle);
	return (HANDLE)pHandle;
}

HTCRESULT WINAPI HTCSensorClose(HTCHANDLE h) {
	HTCHandle_t* pHandle = (HTCHandle_t*)h;
	DBGMESSAGE("HTCSensorClose(%x)", pHandle);

	if (!HTC_DEVICE_VALID(pHandle)) {
		return HTC_ERROR;
	} else {
		switch (pHandle->SensorType) {
			case HTC_TILT_SENSOR: {
				int result = AccelDisable(pHandle->hDevice);
				DBGMESSAGE("HTCSensorClose: TILT. Result = %d (%ls)", result, result == ACCELRESULT_DEBUG ? L"Debug" : L"Real");
				CloseHandle(pHandle->hDevice);
				pHandle->hDevice = 0;

			} break;
			default:
				break;
		}
		pHandle->Signature = 0;
		free(pHandle);
	}
	return HTC_OK;
}

static HTCRESULT _HTCGetGSensorData(HTCHandle_t* pHandle, LPHTC_TILTSENSOR_DATA pTiltData) {

	//Getting x/y/z from device (safer) or from registry (for debugging)
	signed int x = 0,y = 0,z = 0;

	LIS331DLF_XYZ xyz;
	memset(&xyz, 0, sizeof(LIS331DLF_XYZ));

#ifdef REAL_DEVICE
	DWORD bytesCopied=0;
	DeviceIoControl(pHandle->hDevice, IOCTL_ACCEL_READ_VALUE, NULL, NULL, &xyz, sizeof(LIS331DLF_XYZ), &bytesCopied, NULL);
	x=xyz.x;
	y=xyz.y;
	z=xyz.z;
#else
	RegistryGetDWORD(HKEY_LOCAL_MACHINE,L"Drivers\\BuiltIn\\Accelerometer",L"current_accel_x",(DWORD*)&x);
	RegistryGetDWORD(HKEY_LOCAL_MACHINE,L"Drivers\\BuiltIn\\Accelerometer",L"current_accel_y",(DWORD*)&y);
	RegistryGetDWORD(HKEY_LOCAL_MACHINE,L"Drivers\\BuiltIn\\Accelerometer",L"current_accel_z",(DWORD*)&z);
#endif

	//Getting orientation from registry
	DWORD orientation;
	RegistryGetDWORD(HKEY_LOCAL_MACHINE,L"Drivers\\BuiltIn\\Accelerometer",L"current_rotation",(DWORD*)&orientation);
	
	if (z >= 14 && z <= 18)
	{
		orientation = ORIENTATION_FACE_DOWN;
	}
	else if (z <= -14 && z >= -18)
	{
		orientation = ORIENTATION_FACE_UP;
	}
	else
	{
		// Why should we count it, if X2 detects it itself?
		if (orientation == 0)
		{
			orientation = ORIENTATION_PORTRAIT;
		}
		else if (orientation == 90)
		{
			orientation = ORIENTATION_REVERSE_LANDSCAPE;
		}
		else if (orientation == 180)
		{
			orientation = ORIENTATION_UPSIDE_DOWN;
		}
		else
		{
			orientation = ORIENTATION_LANDSCAPE;
		}
	}
	pTiltData->Orientation = orientation;

	DBGMESSAGE("HTCGetGSensorData: GOT %d, %d, %d", x, y, z);
	double fx=(double)x, 
        fy = (double)y, 
        fz = (double)z, 
        roll,
        pitch;
	
	fx = fx / 32.0f;
	fy = fy / 32.0f;
	fz = fz / 32.0f;

	//fixing incorrect directions
	pTiltData->GVectorX = (short)(fy * (-1.00f) * HTC_GSENSOR_RANGE);
	pTiltData->GVectorY = (short)(fx * HTC_GSENSOR_RANGE);
	pTiltData->GVectorZ = (short)(fz * (-1.00f) * HTC_GSENSOR_RANGE);
	
	DBGMESSAGE("HTCGetGSensorData: finally %d, %d, %d", pTiltData->GVectorX, pTiltData->GVectorY, pTiltData->GVectorZ);

	roll = (double)atan(fx / fz);
	if (fz < 0.0f) {
		roll += PI * ((fx > (double)0.0) ? (double)1.0 : (double)-1.0);
	}
	roll *= -RAD2DEG;

	pitch = (double)atan(fy / fz);
	if (fz < 0.0f)	{
		pitch += PI * ((fy > (double)0.0) ? (float)1.0 : (double)-1.0);
	}
	pitch *= -RAD2DEG;

	roll += PI_DEG;
	pitch += PI_DEG;

	pTiltData->TiltX = (int)pitch;
	pTiltData->TiltY = (int)roll;

	return HTC_OK;
};

static HTCRESULT _HTCGetLSensorData(HTCHandle_t* pHandle, LPHTC_LIGHTSENSOR_DATA pLightData) {
	return HTC_ERROR;
}

HTC_EXPORT HTCRESULT WINAPI HTCSensorGetDataOutput(HTCHANDLE h, LPDWORD pData) {
	
	HTCHandle_t* pHandle = (HTCHandle_t*)h;
	if (!HTC_DEVICE_VALID(pHandle) || !pData) {
		return HTC_ERROR;
	} 
	switch (pHandle->SensorType) {
		case HTC_TILT_SENSOR: {
			LPHTC_TILTSENSOR_DATA pTiltData = (LPHTC_TILTSENSOR_DATA)pData;
			return _HTCGetGSensorData(pHandle, pTiltData);
		} break;
		default: {
			return HTC_ERROR;
		} break;
	}
	return S_OK;
}


HTC_EXPORT HTCRESULT WINAPI HTCSensorGetPollingInterval(HTCHANDLE h, LPDWORD lpPollingInterval) {
	DBGMESSAGE("HTCSensorGetPollingInterval called");
	return HTC_DUMMY_RESULT;
}

HTC_EXPORT HTCRESULT WINAPI HTCSensorGetPostureAngle(HTCHANDLE lpHandle, LPDWORD lpD0, LPDWORD lpD1, LPDWORD lpD2, LPDWORD lpD3, LPDWORD lpD4) {
	DBGMESSAGE("HTCSensorGetPostureAngle called");
	return HTC_DUMMY_RESULT;
}

HTC_EXPORT HTCRESULT WINAPI HTCSensorGetPromptAngle(HTCHANDLE lpHandle, LPDWORD lpAngleX, LPDWORD lpAngleY) {
	DBGMESSAGE("HTCSensorGetPromptAngle called");
	return HTC_DUMMY_RESULT;
}

HTC_EXPORT HTCRESULT WINAPI HTCSensorGetSensitivity(HTCHANDLE lpHandle, LPDWORD lpSensitivity) {
	DBGMESSAGE("HTCSensorGetSensitivity called");
	return HTC_DUMMY_RESULT;
}

HTC_EXPORT HTCRESULT WINAPI HTCSensorQueryCapability(DWORD sensorType, LPDWORD lpOutBuffer) {
	HTCRESULT result = HTC_ERROR;
	
	DBGMESSAGE("HTCSensorQueryCapability called");

	switch (sensorType) {
		case HTC_TILT_SENSOR: {
			result = HTC_OK;
		} break;
		default:
			break;
	}
	return result;
}

HTC_EXPORT HTCRESULT WINAPI HTCSensorSetPollingInterval(HTCHANDLE lpHandle, DWORD pollingInterval) {
	DBGMESSAGE("HTCSensorSetPollingInterval called");
	return HTC_DUMMY_RESULT;
}

HTC_EXPORT HTCRESULT WINAPI HTCSensorSetPostureAngle(HTCHANDLE lpHandle, DWORD D0, DWORD D1, DWORD D2, DWORD D3, DWORD D4) {
	DBGMESSAGE("HTCSensorSetPostureAngle called");
	return HTC_DUMMY_RESULT;
}

HTC_EXPORT HTCRESULT WINAPI HTCSensorListener(HTCHANDLE lpHandle) {
	DBGMESSAGE("HTCSensorListener called");
	return HTC_DUMMY_RESULT;
}


HTC_EXPORT HTCRESULT WINAPI HTCSensorSetSensitivity(HTCHANDLE lpHandle, DWORD sensitivity) {
	DBGMESSAGE("HTCSensorSetSensitivity called");
	return HTC_DUMMY_RESULT;
}

HTC_EXPORT HTCRESULT HTCLightSensorGetPollingInterval(HTCHANDLE lpHandle, LPDWORD lpPollingInterval) {
	DBGMESSAGE("HTCLightSensorGetPollingInterval called");
	return HTC_DUMMY_RESULT;
}

HTC_EXPORT HTCRESULT HTCLightSensorSetPollingInterval(HTCHANDLE lpHandle, DWORD pollingInterval) {
	DBGMESSAGE("HTCLightSensorSetPollingInterval called");
	return HTC_DUMMY_RESULT;
}

BOOL WINAPI DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
