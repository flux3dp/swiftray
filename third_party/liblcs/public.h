#ifndef LCSAPI_PUBLIC
#define LCSAPI_PUBLIC

#include <stdint.h>

#if defined(LCSDLL_EXPORTS)
#	if defined(__unix__) || defined(__APPLE__)
#		define LCS_IMPORT __attribute__ ((visibility("default")))
#		define LCS_API  __attribute__ ((stdcall))
#	elif defined(_WIN32)
#		define LCS_IMPORT __declspec(dllexport)
#		define LCS_API
#	elif defined(__ANDROID__)
#		define LCS_IMPORT
#		define LCS_API  __attribute__ ((stdcall))
#	endif
#else
#	if defined(__unix__) || defined(__APPLE__)
#		define LCS_IMPORT
#		define LCS_API   __attribute__ ((stdcall))
#	elif defined(_WIN32)
#		define LCS_IMPORT __declspec(dllimport)
#		define LCS_API
#	elif defined(__ANDROID__)
#		define LCS_IMPORT
#		define LCS_API   __attribute__ ((stdcall))
#	endif
#endif

//input IO default state
#define INPUT_NOT_INIT 0xFFFF
//output IO default state
#define OUTPUT_NOT_INIT 0x0

///sdk error code
enum LCS2Error
{
	LCS_RES_NO_ERROR = 0x00000000,				///< operate success
	LCS_GENERAL_ACTION_FAILED = 0x00000001,
	LCS_GENERAL_ACTION_TIMEOUT = 0x00200001,
	LCS_GENERAL_INVALID_PARAM = 0x00210001,
	LCS_GENERAL_OUT_OF_RANGE = 0x00220000,
	LCS_GENERAL_MEMORY_NOT_ENOUGH = 0x00220001,
	LCS_GENERAL_BUFFER_SIZE_TOO_SMALL = 0x00220002,
	LCS_GENERAL_WRITE_ERROR = 0x00220003,
	LCS_GENERAL_READ_ERROR = 0x00220004,
	LCS_GENERAL_UUID_EXIST = 0x00220005,
	LCS_GENERAL_UUID_NOT_EXIST = 0x00220006,
	LCS_GENERAL_CURRENTLY_BUSY = 0x00220007,
	LCS_GENERAL_PERMISSION_FAILED = 0x00220008,
	LCS_GENERAL_NOT_INITIALIZED = 0x00230000,
	LCS_GENERAL_NOT_OPENED = 0x00230004,
	LCS_GENERAL_AREADY_OPENED = 0x00240000,

	LCS_BOARD_NOT_CONNECT = 0x00300002,
};

/// laser type
enum LCS2LaserType
{
	LCS_CO2 = 0,
	LCS_YAG = 1,
	LCS_FIBER = 2,
	LCS_MOPA = 3,
	LCS_UV = 4,
	LCS_SPI = 5,
	LCS_QCW = 6,
	LCS_WELDYAG = 7            ///< YAG for weld
};

// Wobble type
enum WobbleType
{
	WT_DISABLE = -1,   ///< disable
	WT_WHEEL = 0,	   ///< wheel
	WT_SINE,		   ///< sin
	WT_ELLIPSE,		   ///< ellipse
	WT_STAND8,		   ///< stand eight
	WT_SLEEP8,		   ///< lie eight
};

struct ListStatus {
	uint32_t bMainOpen : 1;    ///< main list open
	uint32_t bSubOepn : 1;     ///< sub list open
	uint32_t bCharOpen : 1;    ///< char list open
	uint32_t bLoop : 1;	       ///< main list loop
	uint32_t bPaused : 1;      ///< main list pause
	uint32_t useless1 : 3;     ///< useless

	uint32_t bBusy1 : 1;       ///< list 1 is busy
	uint32_t bBusy2 : 1;       ///< list 2 is busy
	uint32_t useless2 : 6;     ///< useless

	uint32_t useless : 16;     ///< useless
};

struct BoardRunStatus {

	uint32_t bCacheReady : 1; ///< board is ready
	uint32_t bConnected : 1;  ///< board is connect
	uint32_t useless1 : 6;    ///< useless
	uint32_t useless2 : 16;   ///< useless
	uint32_t boardSpace : 8;  ///< board ram free space, unit:4k byte
};

struct AxisStatus
{
	uint32_t axis1Stop : 1;   ///< axis1 is stop
	uint32_t axis1Zero : 1;   ///< axis1 is at zero
	uint32_t axis1NLimit : 1; ///< axis1 is at Negtive Limit
	uint32_t axis1PLimit : 1; ///< axis1 is at Positive Limit
	uint32_t useless1 : 4;    ///< axis1 useless
	uint32_t axis2Stop : 1;   ///< axis2 is stop
	uint32_t axis2Zero : 1;   ///< axis2 is at zero
	uint32_t axis2NLimit : 1; ///< axis2 is at Negtive Limit
	uint32_t axis2PLimit : 1; ///< axis2 is at Positive Limit
	uint32_t useless2 : 4;    ///< axis2 useless
	uint32_t useless3 : 16;   ///< useless
};

#endif // LCSAPI_PUBLIC
