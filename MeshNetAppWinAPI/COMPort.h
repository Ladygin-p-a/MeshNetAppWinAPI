#pragma once
#include <windows.h>

namespace _ComPort_ {

	class COMPortClass
	{
	public:		
		int InitCOMPortList(int (*)(HWND, BYTE*), HWND);
		int OpenCOMPort(LPTSTR, HANDLE&);
	};
}
