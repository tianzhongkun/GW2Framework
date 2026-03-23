// main.cpp - DLL entry point
#include <windows.h>
#include <string>
#include "DX11Hook.h"
#include "MumbleLink.h"
#include "Logger.h"
#include "PluginManager.h"

static std::string GetRuntimeDir()
{
    char modulePath[MAX_PATH]{};
    GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
    std::string path(modulePath);
    const size_t pos = path.find_last_of("\\/");
    return pos == std::string::npos ? std::string(".") : path.substr(0, pos);
}

static DWORD WINAPI InitThread(LPVOID)
{
    Logger::Instance().Initialize(GetRuntimeDir() + "\\gw2framework.log");
    LOG_INFO("Main", "GW2Framework init thread started");

    Sleep(2000);

    PluginManager::Instance().Initialize();
    LOG_INFO("Main", "PluginManager initialized");

    MumbleLinkReader mumble;
    const bool mumbleOk = mumble.Initialize();
    LOG_INFO("Main", mumbleOk ? "MumbleLink connected" : "MumbleLink not connected yet");

    InitDX11(nullptr);
    LOG_INFO("Main", "DX11 hook initialization requested");

    MessageBoxW(
        NULL,
        mumbleOk
            ? L"GW2Framework 注入成功。\n配置系统已初始化。\nCombat Debug 插件已加载。\n请按 INSERT 查看菜单。"
            : L"GW2Framework 注入成功。\n配置系统已初始化。\nCombat Debug 插件已加载。\n但 MumbleLink 尚未连接。",
        L"GW2Framework",
        MB_ICONINFORMATION | MB_OK
    );

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    } else if (reason == DLL_PROCESS_DETACH) {
        PluginManager::Instance().Shutdown();
        ShutdownDX11();
    }
    return TRUE;
}
