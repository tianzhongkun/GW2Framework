#include "plugins/CombatPlugin.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "imgui.h"
#include "../vendor/json/json.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <windows.h>

using json = nlohmann::json;

namespace {
static std::string WStringToUtf8(const std::wstring& ws) {
    if (ws.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return {};
    std::string out(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), size, nullptr, nullptr);
    return out;
}

static const char* BoolZh(bool v) {
    return v ? "是" : "否";
}

static bool IsNearZero(float v) {
    return std::fabs(v) < 0.0001f;
}
}

CombatPlugin::CombatPlugin()
{
    m_mumbleReady = m_mumble.Initialize();
    LOG_INFO("CombatPlugin", m_mumbleReady ? "MumbleLink connected" : "MumbleLink unavailable");
}

const char* CombatPlugin::GetId() const { return "combat_debug"; }
const char* CombatPlugin::GetCategory() const { return "战斗"; }
const char* CombatPlugin::GetName() const { return "MumbleLink HUD"; }
const char* CombatPlugin::GetSubtitle() const { return "基于 MumbleLink 的实时状态 HUD、路径记录与轨迹显示"; }

std::vector<PluginFeature> CombatPlugin::GetFeatures() const
{
    return {
        {"实时地图/坐标/方向/FOV"},
        {"轻量 HUD 浮窗"},
        {"路径记录与轨迹显示"},
        {"移动速度平滑估算"},
        {"独立插件配置持久化"}
    };
}

bool CombatPlugin::IsEnabled() const { return m_enabled; }
void CombatPlugin::SetEnabled(bool enabled) { m_enabled = enabled; }
bool CombatPlugin::IsAvailable() const { return true; }

float CombatPlugin::Length(const FVector& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float CombatPlugin::Distance(const FVector& a, const FVector& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void CombatPlugin::ClearPath()
{
    m_path.clear();
}

void CombatPlugin::ParseIdentityJson()
{
    m_player.identityUtf8 = WStringToUtf8(m_player.identity);
    m_player.characterName.clear();

    if (m_player.identityUtf8.empty()) {
        return;
    }

    auto parsed = json::parse(m_player.identityUtf8, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return;
    }

    auto it = parsed.find("name");
    if (it != parsed.end() && it->is_string()) {
        m_player.characterName = it->get<std::string>();
    }

    it = parsed.find("fov");
    if (it != parsed.end() && it->is_number()) {
        m_player.fov = it->get<float>();
    }
}

void CombatPlugin::UpdatePathTracking()
{
    if (!m_recordPath || !m_player.hasLink) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (m_path.empty()) {
        m_path.push_back({ m_player.position, now });
        return;
    }

    const float dist = Distance(m_path.back().world, m_player.position);
    if (dist >= m_pathSampleMinDistance) {
        m_path.push_back({ m_player.position, now });
        while ((int)m_path.size() > m_pathMaxNodes) {
            m_path.pop_front();
        }
    }
}

void CombatPlugin::UpdatePlayerData()
{
    if (!m_mumbleReady) {
        m_mumbleReady = m_mumble.Initialize();
        if (!m_mumbleReady) {
            m_player.hasLink = false;
            return;
        }
    }

    if (!m_mumble.Update()) {
        m_player.hasLink = false;
        return;
    }

    m_prevPlayer = m_player;

    m_player.hasLink = true;
    m_player.position = m_mumble.GetPlayerPos();
    m_player.direction = m_mumble.GetPlayerDir();
    m_player.up = m_mumble.GetPlayerTop();
    m_player.cameraPosition = m_mumble.GetCameraPos();
    m_player.cameraDirection = m_mumble.GetCameraDir();
    m_player.cameraTop = m_mumble.GetCameraTop();
    m_player.mapPosition = m_mumble.GetMapPosition();
    m_player.mapId = m_mumble.GetMapId();
    m_player.mapType = m_mumble.GetMapType();
    m_player.uiTick = m_mumble.GetUiTick();
    m_player.uiState = m_mumble.GetUiState();
    m_player.contextLength = m_mumble.GetContextLength();
    m_player.mountIndex = m_mumble.GetMountIndex();
    m_player.mapRotation = m_mumble.GetMapRotation();
    m_player.fov = m_mumble.GetFov();
    m_player.serverName = m_mumble.GetServerName();
    m_player.identity = m_mumble.GetIdentityString();
    m_player.lastUpdate = std::chrono::steady_clock::now();

    m_player.hasMapPosition = !(IsNearZero(m_player.mapPosition.x) && IsNearZero(m_player.mapPosition.z));
    ParseIdentityJson();

    const auto dtMs = std::chrono::duration_cast<std::chrono::milliseconds>(m_player.lastUpdate - m_prevPlayer.lastUpdate).count();
    if (m_prevPlayer.hasLink && dtMs >= 80) {
        const float dt = dtMs / 1000.0f;
        const float dist = Distance(m_player.position, m_prevPlayer.position);
        const float rawSpeed = (dt > 0.0f) ? (dist / dt) : 0.0f;
        m_player.speed = (m_prevPlayer.speed * 0.75f) + (rawSpeed * 0.25f);
    } else if (!m_prevPlayer.hasLink) {
        m_player.speed = 0.0f;
    } else {
        m_player.speed = m_prevPlayer.speed * 0.92f;
    }

    if (m_player.speed < 0.05f) {
        m_player.speed = 0.0f;
    }

    m_player.moving = m_player.speed > 0.10f;
}

void CombatPlugin::OnUpdate()
{
    if (!m_enabled) return;
    if (m_trackPlayer) {
        UpdatePlayerData();
        UpdatePathTracking();
    }
}

void CombatPlugin::OnLoadConfig()
{
    auto& cfg = ConfigManager::Instance();
    m_enabled = cfg.GetBool("plugins.combat_debug.enabled", true);
    m_trackPlayer = cfg.GetBool("plugins.combat_debug.track_player", true);
    m_showDebugPanel = cfg.GetBool("plugins.combat_debug.show_debug_panel", true);
    m_showOverlay = cfg.GetBool("plugins.combat_debug.show_overlay", true);
    m_logIdentity = cfg.GetBool("plugins.combat_debug.log_identity", false);
    m_showPathOverlay = cfg.GetBool("plugins.combat_debug.show_path_overlay", true);
    m_recordPath = cfg.GetBool("plugins.combat_debug.record_path", true);
    m_showIdentityInOverlay = cfg.GetBool("plugins.combat_debug.show_identity_overlay", false);
    m_overlayX = cfg.GetFloat("plugins.combat_debug.overlay_x", 15.0f);
    m_overlayY = cfg.GetFloat("plugins.combat_debug.overlay_y", 180.0f);
    m_pathSampleMinDistance = cfg.GetFloat("plugins.combat_debug.path_sample_min_distance", 20.0f);
    m_pathScale = cfg.GetFloat("plugins.combat_debug.path_scale", 6.0f);
    m_pathMaxNodes = cfg.GetInt("plugins.combat_debug.path_max_nodes", 120);
}

void CombatPlugin::OnSaveConfig() const
{
    auto& cfg = ConfigManager::Instance();
    cfg.SetBool("plugins.combat_debug.enabled", m_enabled);
    cfg.SetBool("plugins.combat_debug.track_player", m_trackPlayer);
    cfg.SetBool("plugins.combat_debug.show_debug_panel", m_showDebugPanel);
    cfg.SetBool("plugins.combat_debug.show_overlay", m_showOverlay);
    cfg.SetBool("plugins.combat_debug.log_identity", m_logIdentity);
    cfg.SetBool("plugins.combat_debug.show_path_overlay", m_showPathOverlay);
    cfg.SetBool("plugins.combat_debug.record_path", m_recordPath);
    cfg.SetBool("plugins.combat_debug.show_identity_overlay", m_showIdentityInOverlay);
    cfg.SetFloat("plugins.combat_debug.overlay_x", m_overlayX);
    cfg.SetFloat("plugins.combat_debug.overlay_y", m_overlayY);
    cfg.SetFloat("plugins.combat_debug.path_sample_min_distance", m_pathSampleMinDistance);
    cfg.SetFloat("plugins.combat_debug.path_scale", m_pathScale);
    cfg.SetInt("plugins.combat_debug.path_max_nodes", m_pathMaxNodes);
}

void CombatPlugin::RenderStatusOverlay()
{
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::SetNextWindowPos(ImVec2(m_overlayX, m_overlayY), ImGuiCond_Always);
    ImGui::Begin("##combat_overlay_status", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav);

    ImGui::TextColored(ImVec4(0.45f, 0.82f, 1.00f, 1.00f), "MumbleLink 实时 HUD");
    ImGui::Separator();
    ImGui::Text("连接状态: %s", m_player.hasLink ? "在线" : "离线");
    ImGui::Text("地图 ID: %u", m_player.mapId);
    ImGui::Text("地图类型: %u", m_player.mapType);
    ImGui::Text("界面 Tick: %u", m_player.uiTick);
    ImGui::Text("角色坐标: %.1f %.1f %.1f", m_player.position.x, m_player.position.y, m_player.position.z);
    ImGui::Text("角色朝向: %.2f %.2f %.2f", m_player.direction.x, m_player.direction.y, m_player.direction.z);
    ImGui::Text("相机坐标: %.1f %.1f %.1f", m_player.cameraPosition.x, m_player.cameraPosition.y, m_player.cameraPosition.z);
    ImGui::Text("相机朝向: %.2f %.2f %.2f", m_player.cameraDirection.x, m_player.cameraDirection.y, m_player.cameraDirection.z);
    if (m_player.hasMapPosition) {
        ImGui::Text("地图坐标: %.1f %.1f", m_player.mapPosition.x, m_player.mapPosition.z);
    } else {
        ImGui::Text("地图坐标: 暂无");
    }
    ImGui::Text("移动速度: %.2f", m_player.speed);
    ImGui::Text("是否移动: %s", BoolZh(m_player.moving));
    ImGui::Text("UI 状态: %u", m_player.uiState);
    ImGui::Text("坐骑编号: %u", (unsigned)m_player.mountIndex);
    ImGui::Text("地图旋转: %.3f", m_player.mapRotation);
    ImGui::Text("视野 FOV: %.3f", m_player.fov);
    if (!m_player.characterName.empty()) {
        ImGui::Text("角色名: %s", m_player.characterName.c_str());
    }
    ImGui::End();
}

void CombatPlugin::RenderPathOverlay()
{
    if (!m_showPathOverlay || m_path.size() < 2) {
        return;
    }

    ImGui::SetNextWindowBgAlpha(0.20f);
    ImGui::SetNextWindowPos(ImVec2(m_overlayX, m_overlayY + 300.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(260.0f, 260.0f), ImGuiCond_Always);
    ImGui::Begin("##combat_overlay_path", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav);

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 center(origin.x + canvasSize.x * 0.5f, origin.y + canvasSize.y * 0.5f);

    drawList->AddRectFilled(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), IM_COL32(8, 12, 18, 180), 8.0f);
    drawList->AddRect(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), IM_COL32(60, 100, 180, 180), 8.0f);
    drawList->AddLine(ImVec2(center.x, origin.y + 8.0f), ImVec2(center.x, origin.y + canvasSize.y - 8.0f), IM_COL32(80, 120, 180, 90), 1.0f);
    drawList->AddLine(ImVec2(origin.x + 8.0f, center.y), ImVec2(origin.x + canvasSize.x - 8.0f, center.y), IM_COL32(80, 120, 180, 90), 1.0f);

    for (size_t i = 1; i < m_path.size(); ++i) {
        const FVector& a = m_path[i - 1].world;
        const FVector& b = m_path[i].world;
        ImVec2 p1(center.x + (a.x - m_player.position.x) / m_pathScale, center.y + (a.z - m_player.position.z) / m_pathScale);
        ImVec2 p2(center.x + (b.x - m_player.position.x) / m_pathScale, center.y + (b.z - m_player.position.z) / m_pathScale);
        drawList->AddLine(p1, p2, IM_COL32(80, 220, 140, 255), 2.0f);
    }

    drawList->AddCircleFilled(center, 4.0f, IM_COL32(255, 220, 80, 255));
    drawList->AddText(ImVec2(origin.x + 10.0f, origin.y + 10.0f), IM_COL32(220, 230, 255, 255), "路径轨迹");
    ImGui::End();
}

void CombatPlugin::OnRenderOverlay()
{
    if (!m_enabled || !m_player.hasLink || !m_showOverlay) {
        return;
    }

    RenderStatusOverlay();
    RenderPathOverlay();
}

void CombatPlugin::OnRenderConfig()
{
    ImGui::Checkbox("启用实时读取", &m_enabled);
    ImGui::Checkbox("跟踪玩家", &m_trackPlayer);
    ImGui::Checkbox("显示调试面板", &m_showDebugPanel);
    ImGui::Checkbox("显示 HUD Overlay", &m_showOverlay);
    ImGui::Checkbox("显示路径 Overlay", &m_showPathOverlay);
    ImGui::Checkbox("记录路径", &m_recordPath);
    ImGui::Checkbox("Overlay 中显示 identity", &m_showIdentityInOverlay);
    ImGui::Checkbox("记录 identity 到日志", &m_logIdentity);
    ImGui::SliderFloat("Overlay X", &m_overlayX, 0.0f, 1600.0f, "%.0f");
    ImGui::SliderFloat("Overlay Y", &m_overlayY, 0.0f, 1200.0f, "%.0f");
    ImGui::SliderFloat("路径采样最小距离", &m_pathSampleMinDistance, 1.0f, 200.0f, "%.1f");
    ImGui::SliderFloat("路径缩放", &m_pathScale, 1.0f, 20.0f, "%.1f");
    ImGui::SliderInt("路径最大节点数", &m_pathMaxNodes, 16, 512);

    if (ImGui::Button("清空路径")) {
        ClearPath();
    }
    ImGui::SameLine();
    if (ImGui::Button("保存插件配置")) {
        OnSaveConfig();
        ConfigManager::Instance().Save();
        if (m_logIdentity && !m_player.identityUtf8.empty()) {
            LOG_INFO("CombatPlugin", std::string("Identity updated: ") + m_player.identityUtf8);
        }
    }

    ImGui::SeparatorText("实时状态");
    ImGui::Text("连接状态: %s", m_player.hasLink ? "已连接" : "未连接");
    ImGui::Text("服务器名: %ls", m_player.serverName.empty() ? L"<unknown>" : m_player.serverName.c_str());
    ImGui::Text("地图 ID: %u", m_player.mapId);
    ImGui::Text("地图类型: %u", m_player.mapType);
    ImGui::Text("界面 Tick: %u", m_player.uiTick);
    ImGui::Text("Context 长度: %u", m_player.contextLength);
    ImGui::Text("角色坐标: X %.2f | Y %.2f | Z %.2f", m_player.position.x, m_player.position.y, m_player.position.z);
    ImGui::Text("角色朝向: X %.2f | Y %.2f | Z %.2f", m_player.direction.x, m_player.direction.y, m_player.direction.z);
    ImGui::Text("相机坐标: X %.2f | Y %.2f | Z %.2f", m_player.cameraPosition.x, m_player.cameraPosition.y, m_player.cameraPosition.z);
    ImGui::Text("相机朝向: X %.2f | Y %.2f | Z %.2f", m_player.cameraDirection.x, m_player.cameraDirection.y, m_player.cameraDirection.z);
    if (m_player.hasMapPosition) {
        ImGui::Text("地图坐标: X %.2f | Y %.2f", m_player.mapPosition.x, m_player.mapPosition.z);
    } else {
        ImGui::Text("地图坐标: 暂无");
    }
    ImGui::Text("移动速度: %.2f", m_player.speed);
    ImGui::Text("是否移动: %s", BoolZh(m_player.moving));
    ImGui::Text("UI 状态: %u | 坐骑编号: %u", m_player.uiState, (unsigned)m_player.mountIndex);
    ImGui::Text("视野 FOV: %.4f | 地图旋转: %.4f", m_player.fov, m_player.mapRotation);
    ImGui::Text("路径节点数: %d", (int)m_path.size());

    if (!m_player.characterName.empty()) {
        ImGui::SeparatorText("角色信息");
        ImGui::Text("角色名: %s", m_player.characterName.c_str());
    }

    if (!m_player.identityUtf8.empty()) {
        ImGui::SeparatorText("Identity JSON");
        ImGui::TextWrapped("%s", m_player.identityUtf8.c_str());
    }
}
