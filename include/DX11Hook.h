// DX11Hook.h - DirectX 11 Hook 声明
#pragma once
#include <d3d11.h>
#include <dxgi.h>

typedef HRESULT (__stdcall *Present_t)(IDXGISwapChain*, UINT, UINT);

extern Present_t g_origPresent;

HRESULT __stdcall HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

void InitDX11(IDXGISwapChain* pSwapChain);

void ShutdownDX11();
