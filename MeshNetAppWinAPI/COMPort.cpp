#include "COMPort.h"
#include <windows.h>
#include <strsafe.h>

using namespace _ComPort_;

BOOL COMPortClass::StartReadCOMPort(int (*clb_GetMessageFromCOMPort)(HWND, TCHAR*), HWND hwndDlg) {

    phWNDData = (PHWNDDATA)CoTaskMemAlloc(sizeof(PHWNDDATA));
    SecureZeroMemory(phWNDData, sizeof(PHWNDDATA));

    phWNDData->hWND_CHAT = hwndDlg;
    phWNDData->ppp = clb_GetMessageFromCOMPort;

    return StartReadCOMPortPrivate(hwndDlg);
}

BOOL COMPortClass::StartReadCOMPortPrivate(HWND hwndDlg) {

    dcb.DCBlength = sizeof(DCB); //в первое поле структуры DCB необходимо занести её длину, она будет использоваться функциями настройки порта для контроля корректности структуры
    //считать структуру DCB из порта
    if (!GetCommState(COMPort, &dcb)) //если не удалось - закрыть порт и вывести сообщение об ошибке в строке  состояния
    {
    }


    //инициализация структуры DCB 
    dcb.BaudRate = CBR_115200; //задаём скорость передачи 115200 бод

    dcb.fBinary = TRUE; //включаем двоичный режим обмена		
    dcb.fOutxCtsFlow = FALSE; //выключаем режим слежения за сигналом CTS
    dcb.fOutxDsrFlow = FALSE; //выключаем режим слежения за сигналом DSR
    dcb.fDtrControl = DTR_CONTROL_DISABLE; //отключаем использование линии DTR
    dcb.fDsrSensitivity = FALSE; //отключаем восприимчивость драйвера к состоянию линии DSR
    dcb.fNull = FALSE; //разрешить приём нулевых байтов
    dcb.fRtsControl = RTS_CONTROL_DISABLE; //отключаем использование линии RTS
    dcb.fAbortOnError = FALSE; //отключаем остановку всех операций чтения/записи при ошибке
    dcb.ByteSize = 8; //задаём 8 бит в байте
    dcb.Parity = 0; //отключаем проверку чётности
    dcb.StopBits = 0; //задаём один стоп-бит

    //загрузить структуру DCB в порт
    if (!SetCommState(COMPort, &dcb)) //если не удалось - закрыть порт и вывести сообщение об ошибке в строке состояния
    {
    }



    //установить таймауты
    timeouts.ReadIntervalTimeout = 0; //таймаут между двумя символами
    timeouts.ReadTotalTimeoutMultiplier = 0; //общий таймаут операции чтения
    timeouts.ReadTotalTimeoutConstant = 0; //константа для общего таймаута операции чтения
    timeouts.WriteTotalTimeoutMultiplier = 0; //общий таймаут операции записи
    timeouts.WriteTotalTimeoutConstant = 0; //константа для общего таймаута операции записи И записываем структуру таймаутов в порт :
    //записать структуру таймаутов в порт
    if (!SetCommTimeouts(COMPort, &timeouts)) //если не удалось - закрыть порт и вывести сообщение обошибке в строке состояния
    {

    }

    SetupComm(COMPort, 2000, 2000);

    reader = CreateThread(NULL, 0, StaticReadThread, this, 0, NULL);

    return TRUE;
}

DWORD WINAPI COMPortClass::StaticReadThread(LPVOID lpParam) {
    // 2. Приводим указатель обратно к типу класса
    COMPortClass* pThis = reinterpret_cast<COMPortClass*>(lpParam);
    // 3. Вызываем обычный метод класса
    return pThis->MemberThreadProc();
}

DWORD COMPortClass::MemberThreadProc() {
   
    COMSTAT comstat; //структура текущего состояния порта, в данной программе используется для определения количества принятых в порт байтов
    DWORD btr, temp, mask, signal; //переменная temp используется в качестве заглушки

    int (*ppp1)(HWND, TCHAR*) = phWNDData->ppp;

    //PHWNDDATA phWNDData;

    //phWNDData = (PHWNDDATA)hwndDlg;

    overlapped.hEvent = CreateEvent(NULL, true, true, NULL); //создать сигнальный объект-событие для асинхронных операций

    SetCommMask(COMPort, EV_RXCHAR); //установить маску на срабатывание по событию приёма байта в порт
    while (1) //пока поток не будет прерван, выполняем цикл
    {
        WaitCommEvent(COMPort, &mask, &overlapped); //ожидать события приёма байта (это и есть	перекрываемая операция)
        signal = WaitForSingleObject(overlapped.hEvent, INFINITE); //приостановить поток до прихода байта
        if (signal == WAIT_OBJECT_0) //если событие прихода байта произошло
        {
            if (GetOverlappedResult(COMPort, &overlapped, &temp, true)) //проверяем, успешно ли завершилась перекрываемая операция WaitCommEvent
                if ((mask & EV_RXCHAR) != 0) //если произошло именно событие прихода байта
                {
                    ClearCommError(COMPort, &temp, &comstat); //нужно заполнить структуру COMSTAT
                    btr = comstat.cbInQue; //и получить из неё количество принятых байтов
                    if (btr) //если действительно есть байты для чтения
                    {
                        ReadFile(COMPort, bufrd, btr, &temp, &overlapped); //прочитать байты из порта в буфер программы
                        counter += btr; //увеличиваем счётчик байтов
                        //ReadPrinting(); //вызываем функцию для вывода данных на экран и в файл
                        //clsCOMPort.GetMSG(AddCHAT, phWNDData->hWND_CHAT, bufrd);


                        LPCTSTR pref = TEXT("DDDD");

                        TCHAR* pMessage = (TCHAR*)CoTaskMemAlloc((5) * sizeof(TCHAR));
                        SecureZeroMemory(pMessage, (5) * sizeof(TCHAR));

                        StringCbCopy(pMessage, 256, pref);

                        ppp1(phWNDData->hWND_CHAT, pMessage);

                        CoTaskMemFree(pMessage);

                    }
                }
        }
    }

    CoTaskMemFree(phWNDData);

    phWNDData = nullptr;

    return 0;
}

//главная функция потока, реализует приём байтов из COM-порта
DWORD WINAPI COMPortClass::ReadThread(LPVOID hwndDlg)
{
    COMSTAT comstat; //структура текущего состояния порта, в данной программе используется для определения количества принятых в порт байтов
    DWORD btr, temp, mask, signal; //переменная temp используется в качестве заглушки

    PHWNDDATA phWNDData;

    phWNDData = (PHWNDDATA)hwndDlg;

    overlapped.hEvent = CreateEvent(NULL, true, true, NULL); //создать сигнальный объект-событие для асинхронных операций

    SetCommMask(COMPort, EV_RXCHAR); //установить маску на срабатывание по событию приёма байта в порт
    while (1) //пока поток не будет прерван, выполняем цикл
    {
        WaitCommEvent(COMPort, &mask, &overlapped); //ожидать события приёма байта (это и есть	перекрываемая операция)
        signal = WaitForSingleObject(overlapped.hEvent, INFINITE); //приостановить поток до прихода байта
        if (signal == WAIT_OBJECT_0) //если событие прихода байта произошло
        {
            if (GetOverlappedResult(COMPort, &overlapped, &temp, true)) //проверяем, успешно ли завершилась перекрываемая операция WaitCommEvent
                if ((mask & EV_RXCHAR) != 0) //если произошло именно событие прихода байта
                {
                    ClearCommError(COMPort, &temp, &comstat); //нужно заполнить структуру COMSTAT
                    btr = comstat.cbInQue; //и получить из неё количество принятых байтов
                    if (btr) //если действительно есть байты для чтения
                    {
                        ReadFile(COMPort, bufrd, btr, &temp, &overlapped); //прочитать байты из порта в буфер программы
                        counter += btr; //увеличиваем счётчик байтов
                        //ReadPrinting(); //вызываем функцию для вывода данных на экран и в файл
                        //clsCOMPort.GetMSG(AddCHAT, phWNDData->hWND_CHAT, bufrd);
                    }
                }
        }
    }
}

BOOL COMPortClass::StartCOMPort(LPTSTR nameCOMPort) {

    return OpenCOMPortPrivate(nameCOMPort);
}

BOOL COMPortClass::OpenCOMPortPrivate(LPTSTR nameCOMPort) {

    LPCTSTR pref = TEXT("\\\\.\\");

    size_t LenNameCOMPort = 0, Len_pref = 0, TotalLen = 0;

    StringCbLength(nameCOMPort, 256, &LenNameCOMPort);
    StringCbLength(pref, 256, &Len_pref);

    Len_pref += sizeof(TCHAR);
    LenNameCOMPort += sizeof(TCHAR);

    TotalLen = Len_pref + LenNameCOMPort;

    LPTSTR FileName = (LPTSTR)CoTaskMemAlloc(TotalLen);

    StringCbCopy(FileName, Len_pref, pref);

    StringCbCat(FileName, TotalLen, (LPCTSTR)nameCOMPort);

    //counter = 0;

    COMPort = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED, NULL);

    if (COMPort == INVALID_HANDLE_VALUE) //если ошибка открытия порта
    {
        COMPort = nullptr;
        CoTaskMemFree(FileName);

        return FALSE;

    }

    CoTaskMemFree(FileName);

    return TRUE;
}

int COMPortClass::GetMSG(int(*_callBackFunc)(HWND, BYTE*), HWND hDlg, BYTE* _bufrd) {

    _callBackFunc(hDlg, _bufrd);

    return 0;
}

BOOL COMPortClass::OpenCOMPort(LPTSTR nameCOMPort, HANDLE& mCOMPort) {
    
    LPCTSTR pref = TEXT("\\\\.\\");

    size_t LenNameCOMPort = 0, Len_pref = 0, TotalLen = 0;

    StringCbLength(nameCOMPort, 256, &LenNameCOMPort);
    StringCbLength(pref, 256, &Len_pref);

    Len_pref += sizeof(TCHAR);
    LenNameCOMPort += sizeof(TCHAR);

    TotalLen = Len_pref + LenNameCOMPort;

    LPTSTR FileName = (LPTSTR)CoTaskMemAlloc(TotalLen);

    StringCbCopy(FileName, Len_pref, pref);

    StringCbCat(FileName, TotalLen, (LPCTSTR)nameCOMPort);

    //counter = 0;

    mCOMPort = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED, NULL);

    if (mCOMPort == INVALID_HANDLE_VALUE) //если ошибка открытия порта
    {
        mCOMPort = nullptr;
        CoTaskMemFree(FileName);

        return FALSE;

    }

    CoTaskMemFree(FileName);

    return TRUE;
}

int COMPortClass::InitCOMPortList(int(*_callBackFunc)(HWND, BYTE*), HWND hDlg) {

    HKEY hKey = 0; //содержит дескриптор ветки реестра
    LSTATUS lResult;


    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM\\"), 0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        return 0;
    }

    DWORD cCOMPort; //содержит количество COM портов
    DWORD MaxLenCOMPortName = 0; //содержит длину самого длинного названия COM порта в символах ANSI без учета нулевого символа
    DWORD MaxLenCOMPortNameByte = 0; //содержит длину самого длинного названия COM порта в байтах

    lResult = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cCOMPort, &MaxLenCOMPortName, &MaxLenCOMPortNameByte, NULL, NULL);

    if (lResult != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 0;
    }

    MaxLenCOMPortName++;


    TCHAR* RegValueName = (TCHAR*)CoTaskMemAlloc((MaxLenCOMPortName) * sizeof(TCHAR));
    BYTE* COMPortName = (BYTE*)CoTaskMemAlloc((MaxLenCOMPortName) * sizeof(BYTE));

    DWORD sizeofRegValueName = MaxLenCOMPortName, sizeofCOMPortName = MaxLenCOMPortName, valueType;

    for (UINT i = 0; i < cCOMPort; i++) {

        SecureZeroMemory(RegValueName, (MaxLenCOMPortName) * sizeof(TCHAR));
        SecureZeroMemory(COMPortName, (MaxLenCOMPortName) * sizeof(BYTE));

        //wmemset(RegValueName, L'\0', MaxLenCOMPortName);
        //memset(COMPortName, '\0', MaxLenCOMPortName);


        sizeofCOMPortName = MaxLenCOMPortName;
        sizeofRegValueName = MaxLenCOMPortName;

        lResult = RegEnumValue(hKey, i, RegValueName, &sizeofRegValueName, NULL, &valueType, COMPortName, &sizeofCOMPortName);

        if (lResult != ERROR_SUCCESS || valueType != REG_SZ) {

            continue;
        }

        _callBackFunc(hDlg, COMPortName);

    }

    CoTaskMemFree(RegValueName);
    CoTaskMemFree(COMPortName);

    RegValueName = nullptr;
    COMPortName = nullptr;

    RegCloseKey(hKey);

    return 0;
}