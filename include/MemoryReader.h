// MemoryReader.h - 内存读取封装
#pragma once
#include <windows.h>
#include <cstdint>

class MemoryReader {
public:
    MemoryReader();
    ~MemoryReader();
    
    bool Attach();
    uintptr_t GetBaseAddress() const { return baseAddr; }
    
    // 通用读取
    template<typename T>
    T Read(uintptr_t addr) {
        T buffer{};
        if (hProcess && addr) {
            ReadProcessMemory(hProcess, (LPCVOID)addr, &buffer, sizeof(T), nullptr);
        }
        return buffer;
    }

private:
    HANDLE hProcess;
    DWORD pid;
    uintptr_t baseAddr;
};
