#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "resource.h"
#include <strsafe.h>
#include "COMPort.h"
#include <mbstring.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Global variables
constexpr UINT MAX_SIZE_COMPortName = 12;
constexpr UINT MAX_SIZE_RegValueName = 255;

// The main window class name.
static TCHAR szWindowClass[] = _T("DesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Windows Desktop Guided Tour Application");

// Stored instance handle for use in Win32 API calls such as FindResource
HINSTANCE hInst;
HWND hWndCHATDlg; //Handle окна-списка сообщений в чате


// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//int GetCOMPortStrList(HWND);
int CALLBACK InitCOMPortList(HWND, BYTE*);//получает список компортов
int CALLBACK GetMessageFromSerial(INT, TCHAR*);

#define BUFSIZE 255 //ёмкость буфера
unsigned char bufrd[BUFSIZE], bufwr[BUFSIZE]; //приёмный и передающий буферы

HANDLE COMPort;
DCB dcb;
COMMTIMEOUTS timeouts;
HWND _hWNDMainDlg;

OVERLAPPED overlapped;
int handle; //дескриптор для работы с файлом с помощью библиотеки <io.h>
bool fl = 0; //флаг, указывающий на успешность операций записи (1 - успешно, 0 - не успешно
unsigned long counter; //счётчик принятых байтов, обнуляется при каждом открытии порт
int countStr = 0;

HANDLE reader; //дескриптор потока чтения из порта

_ComPort_::COMPortClass clsCOMPort;

int CALLBACK GetMessageFromSerial(INT MSG_ID, TCHAR* pMessage) {

    switch(MSG_ID)
    {
    case SERIAL_INCOMING_MSG:
    {

        int pos = (int)SendMessage(hWndCHATDlg, LB_ADDSTRING, 0, (LPARAM)pMessage);
        // Set the array index of the player as item data.
        // This enables us to retrieve the item from the array
        // even after the items are sorted by the list box.
        SendMessage(hWndCHATDlg, LB_SETITEMDATA, pos, (LPARAM)countStr);

        return 0;
    }
    case SERIAL_ERROR_OPEN_PORT:
    {
        MessageBox(_hWNDMainDlg, pMessage, TEXT("ИНФОРМАЦИЯ"), MB_OK);
        return 0;
    }
    case SERIAL_OK_OPEN_PORT:
    {
        //MessageBox(_hWNDMainDlg, pMessage, TEXT("ИНФОРМАЦИЯ"), MB_OK);
        return 0;
    }
    }
    
    return 0;

}


int CALLBACK InitCOMPortList(HWND hDlg, BYTE* COMPortName) {

    DWORD dwIndex;

    HWND hWndComboBox = GetDlgItem(hDlg, IDC_CPLIST);

    dwIndex = SendMessage(hWndComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)COMPortName);
    //или так (через макрос) :ComboBox_AddString(hWndComboBox, A);

    // Send the CB_SETCURSEL message to display an initial item 
    //  in the selection field  
    SendMessage(hWndComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    
    return 0;

}


INT_PTR MainDlgproc(HWND hWNDMainDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message)
    {
    case WM_INITDIALOG:
        
        _hWNDMainDlg = hWNDMainDlg;

        hWndCHATDlg = GetDlgItem(hWNDMainDlg, IDC_CHAT);

        clsCOMPort.InitCOMPortList(InitCOMPortList, hWNDMainDlg);

        return TRUE;

    case WM_COMMAND:

        /*HWND hwnd;
        hwnd = GetDesktopWindow();
        HDC hdc;
        hdc = GetWindowDC(hwnd);

        RECT r;
        GetClientRect(hwnd, &r);

        DrawText(hdc, TEXT("My Text"), -1, &r, DT_LEFT);
        */

        if (HIWORD(wParam) == CBN_SELCHANGE)
            // If the user makes a selection from the list:
            //   Send CB_GETCURSEL message to get the index of the selected list item.
            //   Send CB_GETLBTEXT message to get the item.
            //   Display the item in a messagebox.
        {
            int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
            TCHAR  COMPortName[10]{0};

            (TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT,(WPARAM)ItemIndex, (LPARAM)COMPortName);

            BOOL res = clsCOMPort.BeginSerial(COMPortName, GetMessageFromSerial);

            return TRUE;
        }

        switch (LOWORD(wParam))
        {
        case IDOK:
            return TRUE;

        case IDCANCEL:
            DestroyWindow(hWNDMainDlg);
            return TRUE;
        }
    }
    return FALSE;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN_WND), NULL, MainDlgproc);

    return 0;
    /*WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    // Store instance handle in our global variable
    hInst = hInstance;

    // The parameters to CreateWindowEx explained:
    // WS_EX_OVERLAPPEDWINDOW : An optional extended window style.
    // szWindowClass: the name of the application
    // szTitle: the text that appears in the title bar
    // WS_OVERLAPPEDWINDOW: the type of window to create
    // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
    // 500, 100: initial size (width, height)
    // NULL: the parent of this window
    // NULL: this application does not have a menu bar
    // hInstance: the first parameter from WinMain
    // NULL: not used in this application
    HWND hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 100,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindowEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindowEx
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;*/
}

/*
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR greeting[] = _T("Hello, Windows desktop!");

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        // Here your application is laid out.
        // For this introduction, we just print out "Hello, Windows desktop!"
        // in the top left corner.
        TextOut(hdc,
            5, 5,
            greeting, _tcslen(greeting));
        // End application specific layout section.

        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}*/

/*int GetCOMPortStrList(HWND hDlg) {

    DWORD dwIndex;

    HWND hWndComboBox = GetDlgItem(hDlg, IDC_CPLIST);

    //pCOMPortNameList->ResetContent();

    int r = 0;
    HKEY hkey = NULL;
    //Открываем раздел реестра, в котором хранится иинформация о COM портах
    r = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM\\"), 0, KEY_READ, &hkey);
    if (r != ERROR_SUCCESS)
        return 0;

    unsigned long CountValues = 0, MaxValueNameLen = 0, MaxValueLen = 0;
    //Получаем информацию об открытом разделе реестра
    RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &CountValues, &MaxValueNameLen, &MaxValueLen, NULL, NULL);
    ++MaxValueNameLen;
    //Выделяем память
    TCHAR* bufferName = NULL, * bufferData = NULL;
    bufferName = (TCHAR*)malloc(MaxValueNameLen * sizeof(TCHAR));
    if (!bufferName)
    {
        RegCloseKey(hkey);
        return 0;
    }
    bufferData = (TCHAR*)malloc((MaxValueLen + 1) * sizeof(TCHAR));
    if (!bufferData)
    {
        free(bufferName);
        RegCloseKey(hkey);
        return 0;
    }


    TCHAR* pCOMPortStrList = (TCHAR*)malloc((CountValues * MaxValueLen + 1) * sizeof(TCHAR));
    memset(pCOMPortStrList, '\0', (CountValues * MaxValueLen + 1) * sizeof(TCHAR));

    //TCHAR pCOMPortStrList[128];
    //SecureZeroMemory(pCOMPortStrList, sizeof(pCOMPortStrList));

    unsigned long NameLen, type, DataLen, count = 0;
    //Цикл перебора параметров раздела реестра
    for (unsigned int i = 0; i < CountValues; i++)
    {
        NameLen = MaxValueNameLen;
        DataLen = MaxValueLen;
        r = RegEnumValue(hkey, i, bufferName, &NameLen, NULL, &type, (LPBYTE)bufferData, &DataLen);
        if ((r != ERROR_SUCCESS) || (type != REG_SZ))
            continue;

        //_tprintf(TEXT("%s\n"), bufferData);

        //for (unsigned int ii = 0; ii < _tcslen(bufferData); ii++) {

        size_t len = 0;

        while ((TCHAR) * (bufferData + len) != '\0') {
            len++;
        }

        for (unsigned int ii = 0; ii < len; ii++) {
            TCHAR aaa = (TCHAR) * (bufferData + ii);
            if (aaa != '\0') {
                pCOMPortStrList[count] = aaa;
                count++;
            }

        }

        pCOMPortStrList[count] = ';';
        count++;

        //pCOMPortNameList->AddString(bufferData);

        dwIndex = SendMessage(hWndComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)bufferData);
        //или так (через макрос) :ComboBox_AddString(hWndComboBox, A);

        // Send the CB_SETCURSEL message to display an initial item 
        //  in the selection field  
        SendMessage(hWndComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    }
    pCOMPortStrList[count] = '\0';

    //Освобождаем память
    free(bufferName);
    bufferName = 0;
    free(bufferData);
    bufferData = 0;
    //Закрываем раздел реестра
    RegCloseKey(hkey);

    //return pCOMPortStrList;
    return 0;

}*/