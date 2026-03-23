// ImGuiImpl.cpp - Safe Version
#include "ImGuiImpl.h"
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "GW2API.h"
#include <sstream>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static IGW2API* g_api = nullptr;
static bool g_initialized = false;

void ImGui_Init(ID3D11Device* device, ID3D11DeviceContext* context) {
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(nullptr);
    ImGui_ImplDX11_Init(device, context);
    
    // Delayed API creation
    if (!g_initialized) {
        g_api = CreateAPI();
        g_initialized = true;
    }
}

void ImGui_NewFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Update API every frame
    if (g_api) {
        g_api->Update();
    }
}

void ImGui_Render() {
    ImGui::Render();
}

void ImGui_Shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    if (g_api) {
        DestroyAPI(g_api);
        g_api = nullptr;
        g_initialized = false;
    }
}

void RenderDebugWindow() {
    ImGui::Begin("GW2 Framework - Yaya Edition");
    
    if (!g_initialized) {
        ImGui::Text("Status: Initializing...");
        ImGui::End();
        return;
    }
    
    ImGui::Text("Status: Framework Ready!");
    ImGui::Separator();
    
    if (g_api) {
        Vec3 pos = g_api->GetPosition();
        Vec3 dir = g_api->GetCameraDir();
        
        ImGui::Text("MumbleLink Data:");
        ImGui::Text("  Player Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
        ImGui::Text("  Camera Direction: %.2f, %.2f, %.2f", dir.x, dir.y, dir.z);
        ImGui::Text("  Character: %s", g_api->GetCharacterName().c_str());
        ImGui::Text("  Health: %.1f%%", g_api->GetHealthPct() * 100.0f);
        ImGui::Text("  Energy: %.1f%%", g_api->GetEnergyPct() * 100.0f);
    } else {
        ImGui::Text("API not available.");
    }
    
    ImGui::Separator();
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Press INSERT to toggle UI");
    ImGui::Text("Press F12 to toggle debug window");
    
    ImGui::End();
}
