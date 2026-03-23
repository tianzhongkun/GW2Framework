// ImGuiImpl.h - ImGui 实现
#pragma once
#include <d3d11.h>

void ImGui_Init(ID3D11Device* device, ID3D11DeviceContext* context);
void ImGui_NewFrame();
void ImGui_Render();
void ImGui_Shutdown();

// 调试窗口
void RenderDebugWindow();
