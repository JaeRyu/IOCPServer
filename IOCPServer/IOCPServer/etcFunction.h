#pragma once
#include "stdafx.h"
#include "protocol.h"


char* WC2C(const wchar_t* str);
wchar_t* C2WC(const char* str);


#ifdef UNICODE
#define ConvertChar C2WC
#else
#define ConvertChar WC2C
#endif