#pragma once

#include "Plugin.h"
#include "MumbleLink.h"
#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include <cstdint>

struct CombatEvent
{
    std::chrono::steady_clock::time_point timestamp;
    std::string type;
    float value = 0.0f;
};

struct PathNode
{
    FVector world{ 0.0f, 0.0f, 0.0f };
    std::chrono::steady_clock::time_point timestamp{};
};

struct PlayerState
{
    FVector position{ 0.0f, 0.0f, 0.0f };
    FVector direction{ 0.0f, 0.0f, 0.0f };
    FVector up{ 0.0f, 0.0f, 0.0f };
    FVector cameraPosition{ 0.0f, 0.0f, 0.0f };
    FVector cameraDirection{ 0.0f, 0.0f, 0.0f };
    FVector cameraTop{ 0.0f, 0.0f, 0.0f };
    FVector mapPosition{ 0.0f, 0.0f, 0.0f };
    uint32_t mapId = 0;
    uint32_t mapType = 0;
    uint32_t uiTick = 0;
    uint32_t uiState = 0;
    uint32_t contextLength = 0;
    uint8_t mountIndex = 0;
    float mapRotation = 0.0f;
    float fov = 0.0f;
    float speed = 0.0f;
    std::wstring serverName;
    std::wstring identity;
    std::string identityUtf8;
    std::string characterName;
    bool hasLink = false;
    bool moving = false;
    bool hasMapPosition = false;
    std::chrono::steady_clock::time_point lastUpdate{};
};

class CombatPlugin final : public IPlugin
{
public:
    CombatPlugin();

    const char* GetId() const override;
    const char* GetCategory() const override;
    const char* GetName() const override;
    const char* GetSubtitle() const override;
    std::vector<PluginFeature> GetFeatures() const override;
    bool IsEnabled() const override;
    void SetEnabled(bool enabled) override;
    bool IsAvailable() const override;

    void OnUpdate() override;
    void OnLoadConfig() override;
    void OnSaveConfig() const override;
    void OnRenderConfig() override;
    void OnRenderOverlay() override;

private:
    void UpdatePlayerData();
    void UpdatePathTracking();
    void RenderStatusOverlay();
    void RenderPathOverlay();
    void ClearPath();
    void ParseIdentityJson();
    static float Distance(const FVector& a, const FVector& b);
    static float Length(const FVector& v);

private:
    bool m_enabled = true;
    bool m_trackPlayer = true;
    bool m_showDebugPanel = true;
    bool m_showOverlay = true;
    bool m_logIdentity = false;
    bool m_showPathOverlay = true;
    bool m_recordPath = true;
    bool m_showIdentityInOverlay = false;
    float m_overlayX = 15.0f;
    float m_overlayY = 180.0f;
    float m_pathSampleMinDistance = 20.0f;
    int m_pathMaxNodes = 120;
    float m_pathScale = 6.0f;

    bool m_mumbleReady = false;
    PlayerState m_player;
    PlayerState m_prevPlayer;
    std::deque<CombatEvent> m_events;
    std::deque<PathNode> m_path;
    MumbleLinkReader m_mumble;
};
