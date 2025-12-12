#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "resource.h"
#include <strsafe.h>
#include "COMPort.h"

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

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitCPList(HWND);
//int GetCOMPortStrList(HWND);
int InitCOMPortList(HWND);

#define BUFSIZE 255 //ёмкость буфера
unsigned char bufrd[BUFSIZE], bufwr[BUFSIZE]; //приёмный и передающий буферы

HANDLE COMPort;
DCB dcb;
COMMTIMEOUTS timeouts;

OVERLAPPED overlapped;
int handle; //дескриптор для работы с файлом с помощью библиотеки <io.h>
bool fl = 0; //флаг, указывающий на успешность операций записи (1 - успешно, 0 - не успешно
unsigned long counter; //счётчик принятых байтов, обнуляется при каждом открытии порт

HANDLE reader; //дескриптор потока чтения из порта
DWORD WINAPI ReadThread(LPVOID);

int CALLBACK ShowMSG(int, HWND);

void ReadPrinting(void);
void CloseCOMPort(void);

_ComPort::COMPort clsCOMPort;

TCHAR Planets[9][10] =
{
    TEXT("Mercury"), TEXT("Venus"), TEXT("Terra"), TEXT("Mars"),
    TEXT("Jupiter"), TEXT("Saturn"), TEXT("Uranus"), TEXT("Neptune"),
    TEXT("Pluto??")
};

TCHAR A[16];
int  k = 0;


int CALLBACK ShowMSG(int a, HWND hwndDlg) {

    MessageBox(hwndDlg, TEXT("!!!!!!!!!!!!"), TEXT("this message from ShowMSG"), MB_OK);
    return 0;

}

//главная функция потока, реализует приём байтов из COM-порта
DWORD WINAPI ReadThread(LPVOID)
{
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
                        ReadPrinting(); //вызываем функцию для вывода данных на экран и в файл
                    }
                }
        }
    }
}

//выводим принятые байты на экран и в файл (если включено) 
void ReadPrinting()
{
    /*CString strname = (CString)bufrd;

    pList1->AddString(strname.GetString());*/

    size_t len = 0;

    while ((TCHAR) * (bufrd + len) != '\0') {
        len++;
    }

    TCHAR* pCOMPortStrList1 = (TCHAR*)malloc((len + 1) * sizeof(TCHAR));
    memset(pCOMPortStrList1, '\0', (len + 1) * sizeof(TCHAR));


    for (unsigned int ii = 0; ii < len; ii++) { //len
        pCOMPortStrList1[ii] = bufrd[ii];
    }

    //pList1->AddString(pCOMPortStrList1);
    memset(bufrd, 0, BUFSIZE); //очистить буфер (чтобы данные не накладывались друг на друга)

    free(pCOMPortStrList1);
    pCOMPortStrList1 = nullptr;
}

int InitCOMPortList(HWND hDlg) {

    HKEY hKey = 0; //содержит дескриптор ветки реестра
    LSTATUS lResult;

    DWORD dwIndex;

    HWND hWndComboBox = GetDlgItem(hDlg, IDC_CPLIST);

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


        sizeofCOMPortName  = MaxLenCOMPortName;
        sizeofRegValueName = MaxLenCOMPortName;

        lResult = RegEnumValue(hKey, i, RegValueName, &sizeofRegValueName, NULL, &valueType, COMPortName, &sizeofCOMPortName);

        if (lResult != ERROR_SUCCESS || valueType != REG_SZ) {
            
            continue;
        }     
      
        dwIndex = SendMessage(hWndComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)COMPortName);
        //или так (через макрос) :ComboBox_AddString(hWndComboBox, A);

        // Send the CB_SETCURSEL message to display an initial item 
        //  in the selection field  
        SendMessage(hWndComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);



    }

    CoTaskMemFree(RegValueName);
    CoTaskMemFree(COMPortName);

    RegValueName = nullptr;
    COMPortName  = nullptr;

    RegCloseKey(hKey);
}


INT_PTR MainDlgproc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {

    

    switch (message)
    {
    case WM_INITDIALOG:

        //InitCPList(hwndDlg);
        InitCOMPortList(hwndDlg);
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
            int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL,(WPARAM)0, (LPARAM)0);
            TCHAR  ListItem[10];

            (TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT,(WPARAM)ItemIndex, (LPARAM)ListItem);


            //MessageBox(hwndDlg, (LPCWSTR)ListItem, TEXT("Item Selected"), MB_OK);

            /*
            CString str1, str2 = L"\\\\.\\";
            //MessageBox(str1, MB_OK);

            str2 += str1;
            */

            clsCOMPort.func(ShowMSG, 666, hwndDlg);

            LPCTSTR a1 = L"НЕ ОТКРЫТ", a2 = L"ОТКРЫТ";
            LPCTSTR str1 = TEXT("\\\\.\\");

            size_t stLen_ListItem = 0, stLen_str1 = 0, stTotalLen = 0;

            StringCbLength(ListItem, 256, &stLen_ListItem);
            StringCbLength(str1, 256, &stLen_str1);

            stLen_str1 += sizeof(TCHAR);
            stLen_ListItem += sizeof(TCHAR);

            stTotalLen = stLen_str1 + stLen_ListItem;

            LPTSTR str2 = (LPTSTR)CoTaskMemAlloc(stTotalLen);

            StringCbCopy(str2, stLen_str1, str1);

            StringCbCat(str2, stTotalLen, (LPCTSTR)ListItem);

            counter = 0;

            COMPort = CreateFile(str2, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED, NULL);

            if (COMPort == INVALID_HANDLE_VALUE) //если ошибка открытия порта
            {

                MessageBox(hwndDlg, a1, TEXT("INFO"), MB_OK);

            }
            else {


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

                MessageBox(hwndDlg, a2, TEXT("INFO"), MB_OK);
                reader = CreateThread(NULL, 0, ReadThread, NULL, 0, NULL);
            }

            CoTaskMemFree(str2);

            return TRUE;
        }

        switch (LOWORD(wParam))
        {
        case IDOK:
            return TRUE;

        case IDCANCEL:
            DestroyWindow(hwndDlg);
            return TRUE;
        }
    }
    return FALSE;
}

void InitCPList(HWND hDlg) {

    //GetCOMPortStrList(hDlg);
    
    
    /*
    TCHAR achTemp[256];
    DWORD dwIndex;

    HWND hWndComboBox = GetDlgItem(hDlg, IDC_CPLIST);

    memset(&A, 0, sizeof(A));
    for (k = 0; k <= 8; k += 1)
    {
        wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)Planets[k]);


        // Добавление строки в ComboBox
        //так
        dwIndex = SendMessage(hWndComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
        //или так (через макрос) :ComboBox_AddString(hWndComboBox, A);
        
    }

    // Send the CB_SETCURSEL message to display an initial item 
    //  in the selection field  
    SendMessage(hWndComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    //SendDlgItemMessage(hwndDlg, IDC_CPLIST, CB_SETCURSEL, 0, 0);
    */
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