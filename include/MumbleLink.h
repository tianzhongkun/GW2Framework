// MumbleLink.h - official GW2 MumbleLink layout
#pragma once
#include <windows.h>
#include <string>
#include <cstdint>

struct FVector {
    float x, y, z;
};

#pragma pack(push, 1)
struct MumbleContext {
    uint8_t serverAddress[28];
    uint32_t mapId;
    uint32_t mapType;
    uint32_t shardId;
    uint32_t instance;
    uint32_t buildId;
    uint32_t uiState;
    uint16_t compassWidth;
    uint16_t compassHeight;
    float compassRotation;
    float playerX;
    float playerY;
    float mapCenterX;
    float mapCenterY;
    float mapScale;
    uint32_t processId;
    uint8_t mountIndex;
    uint8_t padding[187];
};

struct MumbleLinkData {
    uint32_t uiVersion;
    uint32_t uiTick;
    float fAvatarPosition[3];
    float fAvatarFront[3];
    float fAvatarTop[3];
    wchar_t name[256];
    float fCameraPosition[3];
    float fCameraFront[3];
    float fCameraTop[3];
    wchar_t identity[256];
    uint32_t context_len;
    MumbleContext context;
    wchar_t description[2048];
};
#pragma pack(pop)

class MumbleLinkReader {
public:
    MumbleLinkReader();
    ~MumbleLinkReader();
    bool Initialize();
    bool Update();
    std::wstring GetServerName() const;
    FVector GetCameraPos() const;
    FVector GetCameraDir() const;
    FVector GetCameraTop() const;
    FVector GetPlayerPos() const;
    FVector GetPlayerDir() const;
    FVector GetPlayerTop() const;
    FVector GetMapPosition() const;
    float GetMapRotation() const;
    float GetFov() const;
    uint32_t GetUiTick() const;
    uint32_t GetContextLength() const;
    std::wstring GetIdentityString() const;
    uint32_t GetMapId() const;
    uint32_t GetUiState() const;
    uint32_t GetMapType() const;
    uint8_t GetMountIndex() const;
    bool IsConnected() const { return isConnected; }

private:
    HANDLE hMapFile;
    MumbleLinkData* pData;
    bool isConnected;
};
