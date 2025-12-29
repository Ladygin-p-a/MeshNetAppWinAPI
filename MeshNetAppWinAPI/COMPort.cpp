#include "COMPort.h"
#include <windows.h>
#include <strsafe.h>

using namespace _ComPort_;

BOOL COMPortClass::StartReadCOMPort() {

    return StartReadCOMPortPrivate();
}

BOOL COMPortClass::StartReadCOMPortPrivate() {

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

                        ConvByteToTStr(bufrd);

                    }
                }
        }
    }

    return 0;
}

//выводим принятые байты на экран и в файл (если включено) 
void COMPortClass::ConvByteToTStr(BYTE* bufrd)
{
    size_t len = 0;
    len = strlen((const char*)bufrd);

   
    len++; //добавим счетчик под нулевой символ

    TCHAR *pMessage = (TCHAR*)CoTaskMemAlloc((len) * sizeof(TCHAR));

    SecureZeroMemory(pMessage, (len) * sizeof(TCHAR));

    
    for (unsigned int ii = 0; ii < len; ii++) { //len
        pMessage[ii] = bufrd[ii];
    }

       
    SendMessageMainDlg(SERIAL_INCOMING_MSG, pMessage);
    

    memset(bufrd, 0, BUFSIZE); //очистить буфер (чтобы данные не накладывались друг на друга)

    CoTaskMemFree(pMessage);
    pMessage = nullptr;
}

BOOL COMPortClass::BeginSerial(LPTSTR nameCOMPort, int (*CallBackFuncMainDlg)(INT, TCHAR*)) {

    SendMessageMainDlg = CallBackFuncMainDlg;

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

        TCHAR msg[] = TEXT("НЕ УДАЛОСЬ ОТКРЫТЬ ВЫБРАННЫЙ COM ПОРТ.");

        SendMessageMainDlg(SERIAL_ERROR_OPEN_PORT, msg);

        return FALSE;

    }

    TCHAR msg[] = TEXT("ВЫБРАННЫЙ COM ПОРТ УСПЕШНО ОТКРЫТ");

    SendMessageMainDlg(SERIAL_OK_OPEN_PORT, msg);

    CoTaskMemFree(FileName);

    BOOL res = StartReadCOMPort();

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


        sizeofCOMPortName  = MaxLenCOMPortName;
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