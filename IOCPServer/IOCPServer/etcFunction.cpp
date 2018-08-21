#include"stdafx.h"
#include "etcFunction.h"


char* WC2C(const wchar_t* str)
{
	char cStr[MAX_STRING];
	//get length
	int len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	//Convert 

	WideCharToMultiByte(CP_ACP, 0, str, -1, cStr, len, 0, 0);

	return cStr;
}
wchar_t* C2WC(const char* str)
{
	wchar_t wStr[MAX_STRING];
	// get length
	int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, NULL);
	// Convert
	MultiByteToWideChar(CP_ACP, 0, str, strnlen(str,MAX_STRING) + 1, wStr, len);

	return wStr;
}