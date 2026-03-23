// GUIInjector.cpp - Final Working Version
#include <windows.h>
#include <string>
#include <vector>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

using namespace std;

HWND hCombo, hBtnInject, hBtnRefresh, hLblStatus;
vector<DWORD> g_pids;
char g_dllPath[MAX_PATH];

void DoInject() {
    int idx = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (idx == CB_ERR || idx >= (int)g_pids.size()) return;
    DWORD pid = g_pids[idx];

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) { SetWindowText(hLblStatus, "Error: Cannot open process"); return; }

    SIZE_T sz = strlen(g_dllPath) + 1;
    LPVOID mem = VirtualAllocEx(hProc, NULL, sz, MEM_COMMIT, PAGE_READWRITE);
    if (!mem) { CloseHandle(hProc); SetWindowText(hLblStatus, "Error: Alloc failed"); return; }

    WriteProcessMemory(hProc, mem, g_dllPath, sz, NULL);
    HMODULE hK = GetModuleHandleA("kernel32.dll");
    FARPROC addr = GetProcAddress(hK, "LoadLibraryA");
    HANDLE hT = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)addr, mem, 0, NULL);
    if (hT) { WaitForSingleObject(hT, 5000); CloseHandle(hT); }
    
    VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
    CloseHandle(hProc);
    SetWindowText(hLblStatus, "Status: Injection Successful! Check game (F12).");
}

void RefreshList(HWND hwndDlg) {
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
    g_pids.clear();
    HWND hw = NULL;
    while ((hw = FindWindowEx(NULL, hw, NULL, NULL)) != NULL) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hw, &pid);
        if (pid == 0) continue;
        HANDLE hP = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hP) {
            char name[MAX_PATH];
            if (GetModuleBaseNameA(hP, NULL, name, MAX_PATH)) {
                string s = string(name);
                if (s.find("Gw2") != string::npos || s.find("Guild") != string::npos) {
                    bool ex = false; for(DWORD p : g_pids) if(p==pid) ex=true;
                    if(!ex) {
                        g_pids.push_back(pid);
                        char buf[128]; sprintf_s(buf, "PID: %lu (%s)", pid, name);
                        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)buf);
                    }
                }
            }
            CloseHandle(hP);
        }
    }
    if (g_pids.empty()) {
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)"No GW2 Found!");
        EnableWindow(hBtnInject, FALSE);
        SetWindowText(hLblStatus, "Status: Start GW2 first.");
    } else {
        SendMessage(hCombo, CB_SETCURSEL, 0, 0);
        EnableWindow(hBtnInject, TRUE);
        SetWindowText(hLblStatus, "Status: Ready!");
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static HWND hDllEdit;
    switch (msg) {
        case WM_CREATE: {
            CreateWindow("STATIC", "1. Select Process:", WS_CHILD | WS_VISIBLE, 10, 10, 200, 20, hwnd, NULL, NULL, NULL);
            hCombo = CreateWindow("COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 10, 30, 250, 200, hwnd, NULL, NULL, NULL);
            CreateWindow("STATIC", "2. DLL Path:", WS_CHILD | WS_VISIBLE, 10, 60, 200, 20, hwnd, NULL, NULL, NULL);
            hDllEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 10, 80, 250, 20, hwnd, NULL, NULL, NULL);
            char dir[MAX_PATH]; GetCurrentDirectoryA(MAX_PATH, dir);
            strcpy_s(g_dllPath, dir); strcat_s(g_dllPath, "\\GW2Framework.dll");
            SetWindowTextA(hDllEdit, g_dllPath);
            hBtnRefresh = CreateWindow("BUTTON", "Refresh", WS_CHILD | WS_VISIBLE, 10, 110, 100, 30, hwnd, NULL, NULL, NULL);
            hBtnInject = CreateWindow("BUTTON", "INJECT", WS_CHILD | WS_VISIBLE, 120, 110, 100, 30, hwnd, NULL, NULL, NULL);
            hLblStatus = CreateWindow("STATIC", "Status: Init...", WS_CHILD | WS_VISIBLE, 10, 150, 250, 20, hwnd, NULL, NULL, NULL);
            RefreshList(hwnd);
            break;
        }
        case WM_COMMAND:
            if (LOWORD(wp) == 1000) { /* Refresh ID is auto, need to match handles */ } 
            // Simplified: We rely on handle comparison in a real app, here we assume standard IDs or use a simpler loop
            // For CMake/VS auto-gen IDs, we check via GetWindowLong or just assign explicit IDs in CreateWindow
            // To keep it single-file simple:
            if ((HWND)lp == hBtnRefresh) RefreshList(hwnd);
            if ((HWND)lp == hBtnInject) DoInject();
            break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nCmd) {
    WNDCLASS wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = hI; wc.lpszClassName = "GW2InjGUI"; 
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, "GW2InjGUI", "GW2 Injector (Yaya GUI)", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 320, 230, NULL, NULL, hI, NULL);
    ShowWindow(hwnd, nCmd); UpdateWindow(hwnd);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
