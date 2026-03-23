// DX11Hook.cpp - Product-style plugin menu scaffold
#include "DX11Hook.h"
#include "PluginManager.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "MinHook.h"
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static Present_t g_origPresent = nullptr;
static bool g_hooked = false;
static bool g_imguiInitialized = false;
static bool g_menuOpen = true;
static bool g_showWatermark = true;
static HWND g_targetWindow = nullptr;
static WNDPROC g_originalWndProc = nullptr;
static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static DWORD g_lastInsertToggle = 0;
static float g_overlayOpacity = 0.96f;
static int g_selectedCategory = 0;
static int g_selectedPlugin = -1;
static PluginManager& g_pluginManager = PluginManager::Instance();
static bool g_loggedFirstPresent = false;
static unsigned long long g_presentCount = 0;

static const char* g_categories[] = {
    "总览", "战斗", "首领", "物品", "移动", "观察", "增益", "日常", "设置"
};

static LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_imguiInitialized && g_menuOpen) {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput) {
            switch (msg) {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
            case WM_XBUTTONDBLCLK:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_CHAR:
            case WM_SETCURSOR:
                return 1;
            default:
                break;
            }
        }
    }
    return CallWindowProc(g_originalWndProc, hWnd, msg, wParam, lParam);
}

static void CreateRenderTarget(IDXGISwapChain* swapChain)
{
    if (g_rtv) return;
    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
        if (SUCCEEDED(g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv))) {
            LOG_INFO("DX11", "Render target view created");
        } else {
            LOG_ERROR("DX11", "Failed to create render target view");
        }
        backBuffer->Release();
    } else {
        LOG_ERROR("DX11", "Failed to get back buffer from swap chain");
    }
}

static void CleanupRenderTarget()
{
    if (g_rtv) {
        g_rtv->Release();
        g_rtv = nullptr;
    }
}

static void SetupImGuiFont()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
}

static void HandleMenuHotkeys()
{
    DWORD now = GetTickCount();
    if ((GetAsyncKeyState(VK_INSERT) & 1) && (now - g_lastInsertToggle > 150)) {
        g_menuOpen = !g_menuOpen;
        g_lastInsertToggle = now;

        if (g_menuOpen) {
            while (ShowCursor(TRUE) < 0) {}
        } else {
            while (ShowCursor(FALSE) >= 0) {}
            ReleaseCapture();
            ClipCursor(nullptr);
        }
    }
}

static void ApplyMenuStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.ItemSpacing = ImVec2(10, 8);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.07f, 0.09f, g_overlayOpacity);
    colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.10f, 0.13f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.30f, 0.60f, 0.35f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.09f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.18f, 0.30f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.30f, 0.60f, 0.65f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.40f, 0.80f, 0.85f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.45f, 0.95f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.16f, 0.28f, 0.55f, 0.90f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.38f, 0.78f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.44f, 0.95f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.35f, 0.75f, 1.00f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.26f, 0.38f, 0.70f);
}

static bool CategoryMatches(const char* category)
{
    if (g_selectedCategory == 0) return true;
    if (g_selectedCategory == 1) return strcmp(category, "战斗") == 0;
    if (g_selectedCategory == 2) return strcmp(category, "首领") == 0;
    if (g_selectedCategory == 3) return strcmp(category, "物品") == 0;
    if (g_selectedCategory == 4) return strcmp(category, "移动") == 0;
    if (g_selectedCategory == 5) return strcmp(category, "观察") == 0;
    if (g_selectedCategory == 6) return strcmp(category, "增益") == 0;
    if (g_selectedCategory == 7) return strcmp(category, "日常") == 0;
    return false;
}

static void RenderWatermark()
{
    if (!g_showWatermark) return;
    ImGui::SetNextWindowBgAlpha(0.30f);
    ImGui::SetNextWindowPos(ImVec2(18, 18), ImGuiCond_Always);
    ImGui::Begin("##watermark", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav);
    ImGui::Text("GW2Framework | INSERT 菜单 | %.1f FPS | Present=%llu", ImGui::GetIO().Framerate, g_presentCount);
    ImGui::End();
}

// 侧边栏已内联到 RenderMainMenu 中

static void RenderOverviewPage()
{
    const auto& plugins = g_pluginManager.GetPlugins();
    ImGui::BeginChild("##overview", ImVec2(0, 0), true);
    ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.00f, 1.00f), "GW2Framework 插件中心");
    ImGui::TextWrapped("现在菜单已经接入插件管理器。后面新增插件时，不需要再把数据写死在 DX11Hook.cpp。 ");
    ImGui::Spacing();

    ImGui::SeparatorText("当前状态");
    ImGui::Text("注入状态: 成功");
    ImGui::Text("渲染状态: DX11 Hook 正常");
    ImGui::Text("输入状态: 菜单打开时接管，关闭时还给游戏");
    ImGui::Text("已注册插件数: %d", (int)plugins.size());

    ImGui::SeparatorText("插件系统骨架");
    ImGui::BulletText("IPlugin 接口已建立");
    ImGui::BulletText("PluginManager 已建立");
    ImGui::BulletText("默认插件注册已建立");
    ImGui::BulletText("菜单已从管理器读取插件数据");
    ImGui::EndChild();
}

static void RenderPluginCard(IPlugin& plugin, int index)
{
    ImGui::PushID(index);
    ImGui::BeginChild("##plugin_card", ImVec2(0, 155), true);

    ImGui::TextColored(ImVec4(0.45f, 0.82f, 1.00f, 1.00f), "%s", plugin.GetName());
    ImGui::SameLine();
    ImGui::TextDisabled("[%s]", plugin.GetCategory());
    ImGui::TextWrapped("%s", plugin.GetSubtitle());
    ImGui::Spacing();

    const auto features = plugin.GetFeatures();
    for (const auto& feature : features) {
        ImGui::BulletText("%s", feature.text.c_str());
    }

    bool enabled = plugin.IsEnabled();
    if (ImGui::Checkbox("启用", &enabled)) {
        plugin.SetEnabled(enabled);
    }
    ImGui::SameLine();
    if (ImGui::Button("配置", ImVec2(80, 0))) {
        g_selectedPlugin = index;
    }
    ImGui::SameLine();
    ImGui::TextColored(plugin.IsAvailable() ? ImVec4(0.45f, 0.90f, 0.55f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
        "%s", plugin.IsAvailable() ? "可集成" : "未就绪");

    ImGui::EndChild();
    ImGui::PopID();
}

static void RenderPluginsPage()
{
    const auto& plugins = g_pluginManager.GetPlugins();
    ImGui::BeginChild("##plugins", ImVec2(0, 0), true);
    ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.00f, 1.00f), "插件列表");
    ImGui::Separator();

    int visibleIndex = 0;
    int actualIndex = 0;
    for (const auto& plugin : plugins) {
        if (!CategoryMatches(plugin->GetCategory())) {
            ++actualIndex;
            continue;
        }
        RenderPluginCard(*plugin, actualIndex);
        ImGui::Spacing();
        ++visibleIndex;
        ++actualIndex;
    }

    if (g_selectedPlugin >= 0 && g_selectedPlugin < (int)plugins.size()) {
        IPlugin& plugin = *plugins[g_selectedPlugin];
        ImGui::SeparatorText("插件配置");
        ImGui::Text("当前插件: %s", plugin.GetName());
        plugin.OnRenderConfig();
    }

    ImGui::EndChild();
}

static void RenderSettingsPage()
{
    ImGui::BeginChild("##settings", ImVec2(0, 0), true);
    ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.00f, 1.00f), "设置");
    ImGui::Separator();
    ImGui::Text("菜单热键: INSERT");
    if (ImGui::Checkbox("显示左上角水印", &g_showWatermark)) {
        ConfigManager::Instance().SetBool("core.show_watermark", g_showWatermark);
    }
    if (ImGui::SliderFloat("全局透明度", &g_overlayOpacity, 0.45f, 1.0f, "%.2f")) {
        ConfigManager::Instance().SetFloat("core.overlay_opacity", g_overlayOpacity);
    }
    if (ImGui::Button("保存全局设置")) {
        ConfigManager::Instance().Save();
        g_pluginManager.SaveConfig();
    }
    ImGui::Spacing();
    ImGui::TextWrapped("现在已经接上 JSON 配置落盘；下一步可以继续加热键绑定、主题切换、插件热加载。 ");
    ImGui::EndChild();
}

static void RenderMainMenu()
{
    if (!g_menuOpen) return;

    ImGui::SetNextWindowSize(ImVec2(1080, 700), ImGuiCond_FirstUseEver);
    // 移除固定宽度，让窗口自适应
   // ImGui::SetNextWindowSize(ImVec2(1080, 700), ImGuiCond_FirstUseEver);
    ImGui::Begin("GW2Framework Professional Menu", &g_menuOpen, ImGuiWindowFlags_NoCollapse);

    // 在绘制任何内容前获取可用宽度
    float mainWindowWidth = ImGui::GetContentRegionAvail().x;
    
    ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.00f, 1.00f), "插件化外挂框架 | 已接入 PluginManager | 预留真实插件逻辑");
    ImGui::Separator();
    ImGui::Spacing();

    // 计算侧边栏宽度：使用可用宽度的 25% 或最小 180px
    float sidebarWidth = mainWindowWidth * 0.25f;
    if (sidebarWidth < 180.0f) sidebarWidth = 180.0f;

    // 计算最大文本宽度以确保足够空间
    float maxTextW = 0.0f;
    for (int i = 0; i < IM_ARRAYSIZE(g_categories); ++i) {
        float w = ImGui::CalcTextSize(g_categories[i]).x;
        if (w > maxTextW) maxTextW = w;
    }
    // 确保边栏宽度至少是最大文本宽度的 1.5 倍 + 内边距
    if (sidebarWidth < maxTextW * 1.5f + 40.0f) sidebarWidth = maxTextW * 1.5f + 40.0f;

// ------------------ 完美替换此部分代码 ------------------
    // 设置侧边栏样式
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
    
    // 渲染侧边栏子窗口
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f)); // 给足标准内边距
    ImGui::BeginChild("##sidebar", ImVec2(sidebarWidth, 0), true);
    ImGui::PopStyleVar(); // 弹出 WindowPadding
    
    // 标题
    ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.00f, 1.00f), "插件分类");
    ImGui::Separator();
    ImGui::Spacing();

    // 渲染分类选项：彻底移除所有 SetCursor 和 Align 强制转换，让 ImGui 原生接管！
    for (int i = 0; i < IM_ARRAYSIZE(g_categories); ++i) {
        // ImVec2(0.0f, 32.0f) 的 0.0f 会自动使用原生安全宽度，绝不溢出
        if (ImGui::Selectable(g_categories[i], g_selectedCategory == i, 0, ImVec2(0.0f, 32.0f))) {
            g_selectedCategory = i;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // 设置水印复选框
    ImGui::Checkbox("显示水印", &g_showWatermark);
    ImGui::SetNextItemWidth(-1); // 填满宽度
    ImGui::SliderFloat("##opacity", &g_overlayOpacity, 0.45f, 1.0f, "透明度 %.2f");
    
    ImGui::EndChild(); // 结束侧边栏
    
    // 关键修复：必须在 EndChild 之后 Pop 父窗口压入的样式！
    ImGui::PopStyleVar(2); // 弹出 ItemSpacing 和 FramePadding
    ImGui::SameLine();
    // --------------------------------------------------

    // 内容区域填满剩余空间
    ImGui::BeginChild("##content", ImVec2(-1, 0), false);
    if (g_selectedCategory == 0) {
        RenderOverviewPage();
    } else if (g_selectedCategory == 8) {
        RenderSettingsPage();
    } else {
        RenderPluginsPage();
    }
    ImGui::EndChild();

    ImGui::End();
}

HRESULT __stdcall HookedPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    ++g_presentCount;
    if (!g_loggedFirstPresent) {
        LOG_INFO("DX11", "HookedPresent hit for the first time");
        g_loggedFirstPresent = true;
    }

    if (!g_imguiInitialized && swapChain) {
        LOG_INFO("DX11", "Initializing ImGui from real swap chain");
        if (SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device)) && g_device) {
            g_device->GetImmediateContext(&g_context);
            DXGI_SWAP_CHAIN_DESC sd{};
            if (SUCCEEDED(swapChain->GetDesc(&sd))) {
                g_targetWindow = sd.OutputWindow;
                LOG_INFO("DX11", std::string("Swap chain output window captured, hwnd=") + std::to_string((unsigned long long)(uintptr_t)g_targetWindow));
            } else {
                LOG_WARN("DX11", "Failed to query swap chain desc");
            }
            CreateRenderTarget(swapChain);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            SetupImGuiFont();
            ImGui::StyleColorsDark();
            ApplyMenuStyle();
            ImGui_ImplWin32_Init(g_targetWindow);
            ImGui_ImplDX11_Init(g_device, g_context);
            g_pluginManager.Initialize();
            LOG_INFO("DX11", "ImGui backends initialized successfully");

            if (!g_menuOpen) {
                while (ShowCursor(FALSE) >= 0) {}
            }

            if (g_targetWindow && !g_originalWndProc) {
                g_originalWndProc = (WNDPROC)SetWindowLongPtr(g_targetWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
                LOG_INFO("DX11", g_originalWndProc ? "WndProc hook installed" : "WndProc hook install failed");
            }

            g_imguiInitialized = true;
            LOG_INFO("DX11", "ImGui initialization complete");
        } else {
            LOG_ERROR("DX11", "Failed to get D3D11 device from real swap chain");
        }
    }

    if (g_imguiInitialized) {
        HandleMenuHotkeys();
        g_pluginManager.UpdateAll();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ApplyMenuStyle();
        RenderWatermark();
        g_pluginManager.RenderOverlayAll();
        RenderMainMenu();

        ImGui::Render();
        if (g_rtv) {
            g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        } else if ((g_presentCount % 300) == 0) {
            LOG_WARN("DX11", "No render target view while rendering frame");
        }
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if ((g_presentCount % 300) == 0) {
            LOG_INFO("DX11", std::string("Rendering overlay frame, menuOpen=") + (g_menuOpen ? "true" : "false") + ", drawLists=" + std::to_string(ImGui::GetDrawData() ? ImGui::GetDrawData()->CmdListsCount : 0));
        }
    }

    return g_origPresent(swapChain, syncInterval, flags);
}

void InitDX11(IDXGISwapChain* /*pSwapChain*/)
{
    if (g_hooked) return;

    LOG_INFO("DX11", "InitDX11 started");

    WNDCLASSEXA wc = {
        sizeof(WNDCLASSEXA), CS_CLASSDC, DefWindowProcA, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        "GW2F_Dummy", nullptr
    };
    RegisterClassExA(&wc);
    HWND hwnd = CreateWindowA("GW2F_Dummy", "GW2F_Dummy", WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* dummyDevice = nullptr;
    ID3D11DeviceContext* dummyContext = nullptr;
    IDXGISwapChain* dummySwapChain = nullptr;
    D3D_FEATURE_LEVEL fl;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &dummySwapChain,
        &dummyDevice,
        &fl,
        &dummyContext);

    if (SUCCEEDED(hr) && dummySwapChain) {
        void** vtable = *(void***)dummySwapChain;
        void* present = vtable[8];
        LOG_INFO("DX11", std::string("Dummy swap chain created, present=") + std::to_string((unsigned long long)(uintptr_t)present));

        MH_STATUS mhInit = MH_Initialize();
        if (mhInit == MH_OK || mhInit == MH_ERROR_ALREADY_INITIALIZED) {
            MH_STATUS createStatus = MH_CreateHook(present, &HookedPresent, reinterpret_cast<void**>(&g_origPresent));
            if (createStatus == MH_OK || createStatus == MH_ERROR_ALREADY_CREATED) {
                if (MH_EnableHook(present) == MH_OK) {
                    g_hooked = true;
                    LOG_INFO("DX11", "Present hook enabled successfully");
                } else {
                    LOG_ERROR("DX11", "MH_EnableHook failed");
                }
            } else {
                LOG_ERROR("DX11", std::string("MH_CreateHook failed, code=") + std::to_string((int)createStatus));
            }
        } else {
            LOG_ERROR("DX11", std::string("MH_Initialize failed, code=") + std::to_string((int)mhInit));
        }
    } else {
        LOG_ERROR("DX11", std::string("D3D11CreateDeviceAndSwapChain failed, hr=") + std::to_string((long)hr));
    }

    if (dummySwapChain) dummySwapChain->Release();
    if (dummyContext) dummyContext->Release();
    if (dummyDevice) dummyDevice->Release();
    DestroyWindow(hwnd);
    UnregisterClassA("GW2F_Dummy", wc.hInstance);
}

void ShutdownDX11()
{
    if (g_targetWindow && g_originalWndProc) {
        SetWindowLongPtr(g_targetWindow, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
        g_originalWndProc = nullptr;
    }
    CleanupRenderTarget();
    if (g_imguiInitialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_imguiInitialized = false;
    }
    if (g_context) {
        g_context->Release();
        g_context = nullptr;
    }
    if (g_device) {
        g_device->Release();
        g_device = nullptr;
    }
    if (g_hooked && g_origPresent) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        g_hooked = false;
    }
}
