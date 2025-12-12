#include "COMPort.h"
#include <windows.h>
#include <strsafe.h>

using namespace _ComPort_;

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