// MemoryReader.cpp
#include "MemoryReader.h"
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

MemoryReader::MemoryReader() : hProcess(nullptr), pid(0), baseAddr(0) {}
MemoryReader::~MemoryReader() {
    if (hProcess) CloseHandle(hProcess);
}

bool MemoryReader::Attach() {
    HWND hwnd = FindWindowW(L"Guild Wars 2", nullptr);
    if (!hwnd) return false;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return false;
    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) return false;
    
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        baseAddr = (uintptr_t)hMods[0];
    }
    return true;
}
