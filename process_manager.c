#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdio.h>

#define ID_LISTBOX 1
#define ID_REFRESH_BUTTON 2
#define COLUMN_WIDTH 80

// Function to calculate system time in 100-nanosecond intervals
ULONGLONG GetSystemTimeInTicks()
{
    FILETIME systemTime;
    GetSystemTimeAsFileTime(&systemTime);
    return (((ULONGLONG)systemTime.dwHighDateTime << 32) | systemTime.dwLowDateTime);
}

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

    SendMessage(hwndListBox, LB_RESETCONTENT, 0, 0);

    char header[512];
    sprintf(header, "%-*s%-*s%-*s%-*s%-*s%-*s", COLUMN_WIDTH, "PID", COLUMN_WIDTH, "Name", COLUMN_WIDTH, "CPU (%)",
            COLUMN_WIDTH, "I/O (%)", COLUMN_WIDTH, "CPU Bound", COLUMN_WIDTH, "Memory (MB)");
    SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)header);

    ULONGLONG totalIoActivity = 0; // To calculate I/O utilization relative to other processes

    // Pass 1: Compute total I/O for all processes
    do
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess)
        {
            IO_COUNTERS ioCounters;
            if (GetProcessIoCounters(hProcess, &ioCounters))
            {
                totalIoActivity += ioCounters.ReadTransferCount + ioCounters.WriteTransferCount;
            }
            CloseHandle(hProcess);
        }
    } while (Process32Next(hProcessSnap, &pe32));

    // Reset snapshot and loop through again for detailed information
    Process32First(hProcessSnap, &pe32);
    ULONGLONG systemTimeStart = GetSystemTimeInTicks();

    do
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess)
        {
            FILETIME creationTime, exitTime, kernelTime, userTime;
            IO_COUNTERS ioCounters;
            PROCESS_MEMORY_COUNTERS pmc;

            if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime))
            {
                ULONGLONG cpuTime = (((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime) +
                                    (((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime);

                ULONGLONG systemTimeElapsed = GetSystemTimeInTicks() - systemTimeStart;
                double cpuUsage = 100.0 * cpuTime / systemTimeElapsed; // CPU usage percentage

                double ioUsage = 0.0;
                if (GetProcessIoCounters(hProcess, &ioCounters))
                {
                    ULONGLONG ioTotal = ioCounters.ReadTransferCount + ioCounters.WriteTransferCount;
                    ioUsage = totalIoActivity > 0 ? (100.0 * ioTotal / totalIoActivity) : 0.0; // I/O usage percentage
                }

                // Determine if the process is CPU-bound
                const char *type = (cpuUsage > ioUsage) ? "CPU-Bound" : "I/O-Bound";

                // Get memory usage
                double memoryUsage = 0.0;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
                {
                    memoryUsage = pmc.WorkingSetSize / (1024.0 * 1024.0); // Convert bytes to MB
                }

                // Add process information to the list box with columns aligned
                char processInfo[512];
                sprintf(processInfo, "%-*d%-*s%-*.1f%-*.1f%-*s%-*.2f", COLUMN_WIDTH, pe32.th32ProcessID, COLUMN_WIDTH,
                        pe32.szExeFile, COLUMN_WIDTH, cpuUsage, COLUMN_WIDTH, ioUsage, COLUMN_WIDTH, type, COLUMN_WIDTH,
                        memoryUsage);
                SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)processInfo);
            }

            CloseHandle(hProcess);
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
}

// Function to dynamically resize controls
void ResizeControls(HWND hwnd, HWND hwndListBox, HWND hwndButton)
{
    RECT rect;
    GetClientRect(hwnd, &rect);

    // Resize the ListBox to fit the window width and height, leaving space for the button
    MoveWindow(hwndListBox, 10, 10, rect.right - 20, rect.bottom - 70, TRUE);

    // Center the Refresh Button at the bottom
    int buttonWidth = 120, buttonHeight = 30;
    int buttonX = (rect.right - buttonWidth) / 2;
    int buttonY = rect.bottom - 50;
    MoveWindow(hwndButton, buttonX, buttonY, buttonWidth, buttonHeight, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndListBox;
    static HWND hwndButton;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Create a ListBox to display process information
        hwndListBox = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS | WS_HSCROLL,
                                   10, 10, 600, 300, hwnd, (HMENU)ID_LISTBOX, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // Create a Refresh Button
        hwndButton = CreateWindow("BUTTON", "Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  300, 350, 120, 30, hwnd, (HMENU)ID_REFRESH_BUTTON, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // Get process info and populate the list box
        GetProcessInfo(hwnd, hwndListBox);
        return 0;
    }
    case WM_SIZE:
    {
        // Resize the ListBox and button dynamically when the window is resized
        ResizeControls(hwnd, hwndListBox, hwndButton);
        return 0;
    }
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_REFRESH_BUTTON)
        {
            // Refresh button pressed, reload the process list
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

int main()
{
    // Window Class Setup
    const char CLASS_NAME[] = "ProcessManagerClass";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create Window with fullscreen style
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Process Manager", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, wc.hInstance, NULL);

    if (hwnd == NULL)
    {
        MessageBox(NULL, "Failed to create window.", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    // Show window maximized
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
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
