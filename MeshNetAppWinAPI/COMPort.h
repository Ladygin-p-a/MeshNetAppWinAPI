#pragma once
#include <windows.h>

namespace _ComPort_ {

	#define SERIAL_ERROR_OPEN_PORT 0
	#define SERIAL_INCOMING_MSG 1

	class COMPortClass
	{
	public:

		#define BUFSIZE 255 //ёмкость буфера

		unsigned char bufrd[BUFSIZE], bufwr[BUFSIZE]; //приёмный и передающий буферы

		HANDLE COMPort;
		DCB dcb;
		COMMTIMEOUTS timeouts;

		OVERLAPPED overlapped;
		int handle; //дескриптор для работы с файлом с помощью библиотеки <io.h>
		bool fl = 0; //флаг, указывающий на успешность операций записи (1 - успешно, 0 - не успешно
		unsigned long counter; //счётчик принятых байтов, обнуляется при каждом открытии порт
		int countStr = 0;

		HANDLE reader; //дескриптор потока чтения из порта
		int InitCOMPortList(int (*)(HWND, BYTE*), HWND);
		int GetMSG(int (*)(HWND, BYTE*), HWND, BYTE*);
		int OpenCOMPort(LPTSTR, HANDLE&);
		BOOL StartCOMPort(LPTSTR);
		BOOL StartReadCOMPort(int (*)(INT, TCHAR*));
		DWORD WINAPI ReadThread(LPVOID);
		static DWORD WINAPI StaticReadThread(LPVOID);
		DWORD MemberThreadProc();

		/*
		typedef struct MainDLGData {
			int (*SendMessageMainDlg)(INT, TCHAR*);
		} MAINDLGDATA, *PMAINDLGDATA;

		PMAINDLGDATA pMainDLGData = nullptr;
		*/

		int (*SendMessageMainDlg)(INT, TCHAR*);

	private:
		BOOL OpenCOMPortPrivate(LPTSTR);
		BOOL StartReadCOMPortPrivate();
		void ConvByteToTStr(BYTE*);

	};
};