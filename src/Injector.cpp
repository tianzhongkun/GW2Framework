// Injector.cpp - Standalone Injector (EXE)
// Usage: Start the game first, then run this program to inject the DLL.
#include <windows.h>
#include <iostream>
#include <string>

using namespace std;

int main() {
    cout << "=== GW2 Framework Injector (Yaya Edition) ===" << endl;
    
    // 1. Find game window
    HWND hWnd = FindWindowW(L"Guild Wars 2", nullptr);
    if (!hWnd) {
        cout << "[!] Error: Guild Wars 2 is not running." << endl;
        cout << "    Please start the game first." << endl;
        system("pause");
        return 1;
    }
    cout << "[+] Game window found!" << endl;

    // 2. Get Process ID
    DWORD pid;
    GetWindowThreadProcessId(hWnd, &pid);
    if (!pid) {
        cout << "[!] Error: Cannot get PID." << endl;
        return 1;
    }
    cout << "[+] PID: " << pid << endl;

    // 3. Open process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        cout << "[!] Error: Cannot open process." << endl;
        return 1;
    }
    cout << "[+] Process handle obtained." << endl;

    // 4. Get DLL path
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    string dllPath = string(currentDir) + "\\GW2Framework.dll";
    cout << "[+] DLL Path: " << dllPath << endl;

    // 5. Allocate memory
    SIZE_T dllSize = dllPath.size() + 1;
    LPVOID pRemoteMem = VirtualAllocEx(hProcess, nullptr, dllSize, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteMem) {
        cout << "[!] Error: Cannot allocate memory." << endl;
        CloseHandle(hProcess);
        return 1;
    }

    // 6. Write DLL path
    if (!WriteProcessMemory(hProcess, pRemoteMem, dllPath.c_str(), dllSize, nullptr)) {
        cout << "[!] Error: Cannot write memory." << endl;
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // 7. Get LoadLibraryA address
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryA");
    if (!loadLibAddr) {
        cout << "[!] Error: Cannot find LoadLibraryA." << endl;
        return 1;
    }

    // 8. Create remote thread
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, pRemoteMem, 0, nullptr);
    if (!hThread) {
        cout << "[!] Error: Cannot create remote thread." << endl;
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    cout << "[+] Injection successful! Waiting..." << endl;
    WaitForSingleObject(hThread, 5000);
    
    // 9. Cleanup
    VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    cout << "[+] Done! Check the game (Press F12)." << endl;
    return 0;
}
