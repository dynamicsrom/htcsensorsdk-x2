// File: htclib.h
#pragma once

#define HTC_EXPORT __declspec(dllexport)

typedef struct _HTC_LIGHTSENSOR_DATA {
	int Reserved0;						// reserved, always 3
	unsigned char Luminance;	// 0..30
} HTC_LIGHTSENSOR_DATA, *PHTC_LIGHTSENSOR_DATA, *LPHTC_LIGHTSENSOR_DATA;

typedef struct _HTC_TILTSENSOR_DATA {
  short GVectorX;						// From -1000 to 1000 (about), 0 is flat
  short GVectorY;						// From -1000 to 1000 (about), 0 is flat
  short GVectorZ;						// From -1000 to 1000 (about)

  short Unknown1;						// Always zero (padding)

  int		TiltX;							// From 0 to 359 (pitch *this field needs checking*) 
  int		TiltY;							// From 0 to 359 (roll  *this field needs checking*)
  int		Orientation;				// From 0 to 5, see ORIENTATION_* macros above
} HTC_TILTSENSOR_DATA, *PHTC_TILTSENSOR_DATA, *LPHTC_TILTSENSOR_DATA;

typedef struct _HTCHandle_t {
	int Signature;					// simple validity checking
	int SensorType;
	union {
		HANDLE hDevice;	// G-Sensor handle
		HANDLE hLightSensor;
	};
} HTCHandle_t;

typedef HANDLE HTCHANDLE;
enum {
	HTC_ERROR = 0,
	HTC_OK = 1
};

// HTC Sensor IDs
enum {
	HTC_TILT_SENSOR		= 1,
	HTC_LIGHT_SENSOR	= 2,
	HTC_STYLUS_SENSOR = 3
};

typedef int HTCRESULT;

#define HTC_DEVICE_SIGNATURE 0xfeedface
#define HTC_DEVICE_VALID(d) \
	(BOOL)(d && d->Signature == HTC_DEVICE_SIGNATURE)

#ifdef HTC_GSENSOR_NO_CORRECTION
#define HTC_GSENSOR_RANGE 1000.0f
#else
#define HTC_GSENSOR_RANGE 980.0f
#endif

#define PI ((float)3.1415926535)
#define PI_DEG ((float)180.0)
static const float RAD2DEG = PI_DEG/PI;

enum {
	ORIENTATION_LANDSCAPE					=	0,
	ORIENTATION_REVERSE_LANDSCAPE	=	1,
	ORIENTATION_PORTRAIT					=	2,
	ORIENTATION_UPSIDE_DOWN				=	3,
	ORIENTATION_FACE_DOWN					=	4,
	ORIENTATION_FACE_UP						=	5
};

// Return value for dummy functions
#define HTC_DUMMY_RESULT	HTC_ERROR
