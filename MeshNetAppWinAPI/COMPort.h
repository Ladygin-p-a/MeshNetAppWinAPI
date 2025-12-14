#pragma once
#include <windows.h>

namespace _ComPort_ {

	class COMPortClass
	{
	public:		
		int InitCOMPortList(int (*)(HWND, BYTE*), HWND);
		int GetMSG(int (*)(HWND, BYTE*), HWND, BYTE*);
		int OpenCOMPort(LPTSTR, HANDLE&);
	};
}
