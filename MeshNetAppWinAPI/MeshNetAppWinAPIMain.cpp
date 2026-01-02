#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "resource.h"
#include <strsafe.h>
#include "COMPort.h"
#include <mbstring.h>
#include <windowsx.h>


#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


// The main window class name.
static TCHAR szWindowClass[] = _T("DesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Windows Desktop Guided Tour Application");


TCHAR  COMPortName[COMPortNameLen] { 0 };

// Stored instance handle for use in Win32 API calls such as FindResource
HINSTANCE hInst;
HWND hWndCHATDlg; //Handle окна-списка сообщений в чате
HWND hWndCONNECTBtn; //Handle кнопки Соединиться
HWND _hWNDMainDlg; //Handle главного окна
HWND hWndCOMPortList; //Handle списка ком портов

// Forward declarations of functions included in this code module:
//LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT CALLBACK InitCOMPortList(BYTE*);//получает список компортов
INT CALLBACK GetMessageFromSerial(INT, TCHAR*); //главная функция приема сообщений от порта

int countStr = 0;


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
        MessageBox(_hWNDMainDlg, pMessage, TEXT("ОШИБКА ОТКРЫТИЯ ПОРТА"), MB_OK);
        return 0;
    }
    case SERIAL_OK_OPEN_PORT:
    {
        //MessageBox(_hWNDMainDlg, pMessage, TEXT("ИНФОРМАЦИЯ"), MB_OK);
        return 0;
    }
    case SERIAL_CHECK_EMPTY_PORT_NAME:
    {
        TCHAR lpch[COMPortNameLen]{ 0 };
        int cch = ComboBox_GetText(hWndCOMPortList, lpch, COMPortNameLen);

        if (!cch) {

            MessageBox(_hWNDMainDlg, pMessage, TEXT("ОШИБКА ВЫБОРА ПОРТА"), MB_OK);
        }

        return cch;
    }
    case COMPORTLIST_ENABLED:
    {
        ComboBox_Enable(hWndCOMPortList, TRUE);
        Button_SetText(hWndCONNECTBtn, clsCOMPort.CONNECT_MSG);

        return 0;
    }
    case COMPORTLIST_DISABLED:
    {
        ComboBox_Enable(hWndCOMPortList, FALSE);
        Button_SetText(hWndCONNECTBtn, clsCOMPort.DISCONNECT_MSG);

        return 0;
    }
    }
    
    return 0;

}


int CALLBACK InitCOMPortList(BYTE* COMPortName) {

    DWORD dwIndex;

    dwIndex = SendMessage(hWndCOMPortList, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)COMPortName);
    //или так (через макрос) :ComboBox_AddString(hWndComboBox, A);
    
    ////устанавливаем добавленный порт как выбранный вариант  
    //SendMessage(hWndCOMPortList, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    
    return 0;

}


INT_PTR MainDlgproc(HWND hWNDMainDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message)
    {
    case WM_INITDIALOG:
        
        _hWNDMainDlg = hWNDMainDlg;

        hWndCHATDlg     = GetDlgItem(hWNDMainDlg, IDC_CHAT);
        hWndCONNECTBtn  = GetDlgItem(hWNDMainDlg, IDC_CONNECT);
        hWndCOMPortList = GetDlgItem(hWNDMainDlg, IDC_COMPortLIST);

        clsCOMPort.InitCOMPortList(InitCOMPortList);

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
        if ((!clsCOMPort.PortIsOpen) && (LOWORD(wParam) == IDC_CONNECT) && ((HIWORD(wParam) == BN_CLICKED))) //Обработаем нажатие кнопки Соединиться
        {
            
            BOOL res = clsCOMPort.BeginSerial(COMPortName, GetMessageFromSerial);

            return TRUE;
        }

        if ((clsCOMPort.PortIsOpen) && (LOWORD(wParam) == IDC_CONNECT) && ((HIWORD(wParam) == BN_CLICKED))) //Обработаем нажатие кнопки Прервать
        {

            clsCOMPort.StopSerial();

            return TRUE;
        }

        if (HIWORD(wParam) == CBN_SELCHANGE)
            // If the user makes a selection from the list:
            //   Send CB_GETCURSEL message to get the index of the selected list item.
            //   Send CB_GETLBTEXT message to get the item.
            //   Display the item in a messagebox.
        {
            int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
           

            (TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT,(WPARAM)ItemIndex, (LPARAM)COMPortName);

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

