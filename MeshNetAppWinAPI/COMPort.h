#pragma once
#include <windows.h>

namespace _ComPort_ {

	#define SERIAL_ERROR_OPEN_PORT 0
	#define SERIAL_INCOMING_MSG 1
	#define SERIAL_OK_OPEN_PORT 2
	#define SERIAL_CHECK_EMPTY_PORT_NAME 3
	#define COMPORTLIST_ENABLED 4
	#define COMPORTLIST_DISABLED 5

	#define COMPortNameLen 10
	#define EventCount 2

	class COMPortClass
	{
	public:

		#define BUFSIZE 255 //ёмкость буфера
		#define BUFSIZE_MSG 60 //ёмкость буфера для сообщения

		BYTE bufrd[BUFSIZE], bufwr[BUFSIZE]; //приёмный и передающий буферы

		TCHAR SERIAL_CHECK_EMPTY_PORT_NAME_MSG[BUFSIZE_MSG] = TEXT("ВЫ НЕ ВЫБРАЛИ ПОРТ ДЛЯ СОЕДИНЕНИЯ");
		TCHAR SERIAL_ERROR_OPEN_PORT_MSG[BUFSIZE_MSG]       = TEXT("НЕ УДАЛОСЬ ОТКРЫТЬ ВЫБРАННЫЙ COM ПОРТ.");
		TCHAR SERIAL_OK_OPEN_PORT_MSG[BUFSIZE_MSG]          = TEXT("ВЫБРАННЫЙ COM ПОРТ УСПЕШНО ОТКРЫТ");
		TCHAR CONNECT_MSG[BUFSIZE_MSG]                      = TEXT("Соединиться");
		TCHAR DISCONNECT_MSG[BUFSIZE_MSG]                   = TEXT("Прервать");
		TCHAR EMPTY_MSG[1]                                  = TEXT("");

		HANDLE COMPort;
		DCB dcb;
		COMMTIMEOUTS timeouts;

		OVERLAPPED overlapped;
		HANDLE EventTerminateCOMPortThread;

		int handle; //дескриптор для работы с файлом с помощью библиотеки <io.h>
		bool fl = 0; //флаг, указывающий на успешность операций записи (1 - успешно, 0 - не успешно
		unsigned long counter; //счётчик принятых байтов, обнуляется при каждом открытии порт
		int countStr = 0;
		BOOL PortIsOpen = FALSE;

		HANDLE reader; //дескриптор потока чтения из порта
		INT InitCOMPortList(int (*)(BYTE*));
		BOOL BeginSerial(LPTSTR, int (*)(INT, TCHAR*));
		BOOL StopSerial();
		BOOL StartReadCOMPort();
		DWORD WINAPI ReadThread(LPVOID);
		static DWORD WINAPI StaticReadThread(LPVOID);
		DWORD ReadThread();


		INT(*SendMessageMainDlg)(INT, TCHAR*);

	private:
		BOOL OpenCOMPortPrivate(LPTSTR);
		BOOL StartReadCOMPortPrivate();
		void ConvByteToTStr(BYTE*);

	};
};