#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

#define MAX_PROCESSES 100
#define ID_LISTBOX 1
#define ID_REFRESH_BUTTON 2
#define COLUMN_WIDTH 45

// Function to retrieve process info and display it in separate columns
void GetProcessInfo(HWND hwnd, HWND hwndListBox)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    // Take a snapshot of all processes in the system
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, "Failed to take process snapshot.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32))
    {
        MessageBox(NULL, "Failed to retrieve process information.", "Error", MB_OK | MB_ICONERROR);
        CloseHandle(hProcessSnap);
        return;
    }

    // Clear the ListBox before adding new data
    SendMessage(hwndListBox, LB_RESETCONTENT, 0, 0);

    // Add header to the list
    char header[256];
    sprintf(header, "%-*s%-*s%-*s", COLUMN_WIDTH, "PID", COLUMN_WIDTH, "Name", COLUMN_WIDTH, "CPU/I/O Bound");
    SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)header);

    // Loop through all processes
    do
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess)
        {
            FILETIME creationTime, exitTime, kernelTime, userTime;
            IO_COUNTERS ioCounters;

            if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime))
            {
                ULONGLONG cpuTime = (((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime) +
                                    (((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime);

                if (GetProcessIoCounters(hProcess, &ioCounters))
                {
                    ULONGLONG ioTotal = ioCounters.ReadTransferCount + ioCounters.WriteTransferCount;

                    // Determine if the process is CPU-bound or I/O-bound
                    const char *type = (cpuTime > ioTotal) ? "CPU-Bound" : "I/O-Bound";

                    // Add process information to the list box with columns aligned
                    char processInfo[256];
                    sprintf(processInfo, "%-*d%-*s%-*s", COLUMN_WIDTH, pe32.th32ProcessID, COLUMN_WIDTH, pe32.szExeFile, COLUMN_WIDTH, type);
                    SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)processInfo);
                }
            }

            CloseHandle(hProcess);
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
}

// Window Procedure function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Create a ListBox to display process information, with horizontal scrolling enabled
        HWND hwndListBox = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS | LBS_MULTIPLESEL | WS_HSCROLL,
                                        10, 40, 600, 300, hwnd, (HMENU)ID_LISTBOX, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // Create a Refresh Button
        CreateWindow("BUTTON", "Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     620, 10, 100, 30, hwnd, (HMENU)ID_REFRESH_BUTTON, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // Get process info and populate the list box
        GetProcessInfo(hwnd, hwndListBox);

        return 0;
    }
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_REFRESH_BUTTON)
        {
            // Refresh button pressed, reload the process list
            HWND hwndListBox = GetDlgItem(hwnd, ID_LISTBOX);
            GetProcessInfo(hwnd, hwndListBox);
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Entry point
int main()
{
    // Window Class Setup
    const char CLASS_NAME[] = "ProcessManagerClass";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create Window with increased width
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Process Manager", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, 800, 400, NULL, NULL, wc.hInstance, NULL);

    if (hwnd == NULL)
    {
        MessageBox(NULL, "Failed to create window.", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    // Show window
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
