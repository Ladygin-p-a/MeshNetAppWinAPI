#include "COMPort.h"
#include <windows.h>

using namespace _ComPort;

int COMPort::func(int(*op)(int, HWND), int a, HWND hWnd) {

	return op(a, hWnd);

}