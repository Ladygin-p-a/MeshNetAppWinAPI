#pragma once
#include <windows.h>

namespace _ComPort {

	class COMPort
	{
	public:
		int	func(int (*)(int, HWND), int, HWND);
	};
}
