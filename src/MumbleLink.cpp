// MumbleLink.cpp - official GW2 MumbleLink layout
#include "MumbleLink.h"
#include <cstring>

namespace {
static FVector ToVec3(const float v[3]) {
    return { v[0], v[1], v[2] };
}
}

MumbleLinkReader::MumbleLinkReader() : hMapFile(NULL), pData(NULL), isConnected(false) {}

MumbleLinkReader::~MumbleLinkReader() {
    if (pData) UnmapViewOfFile(pData);
    if (hMapFile) CloseHandle(hMapFile);
}

bool MumbleLinkReader::Initialize() {
    if (pData) { UnmapViewOfFile(pData); pData = NULL; }
    if (hMapFile) { CloseHandle(hMapFile); hMapFile = NULL; }
    isConnected = false;

    hMapFile = OpenFileMappingW(FILE_MAP_READ, FALSE, L"MumbleLink");
    if (!hMapFile) return false;

    pData = (MumbleLinkData*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(MumbleLinkData));
    if (!pData) {
        CloseHandle(hMapFile);
        hMapFile = NULL;
        return false;
    }

    isConnected = true;
    return true;
}

bool MumbleLinkReader::Update() {
    if (!isConnected || !pData) return false;
    if (pData->uiVersion != 2) return false;
    if (std::wcscmp(pData->name, L"Guild Wars 2") != 0) return false;
    return true;
}

std::wstring MumbleLinkReader::GetServerName() const {
    if (!isConnected || !pData) return L"";
    return std::wstring(pData->name);
}

FVector MumbleLinkReader::GetCameraPos() const {
    if (!isConnected || !pData) return {0,0,0};
    return ToVec3(pData->fCameraPosition);
}

FVector MumbleLinkReader::GetCameraDir() const {
    if (!isConnected || !pData) return {0,0,0};
    return ToVec3(pData->fCameraFront);
}

FVector MumbleLinkReader::GetCameraTop() const {
    if (!isConnected || !pData) return {0,0,0};
    return ToVec3(pData->fCameraTop);
}

FVector MumbleLinkReader::GetPlayerPos() const {
    if (!isConnected || !pData) return {0,0,0};
    return ToVec3(pData->fAvatarPosition);
}

FVector MumbleLinkReader::GetPlayerDir() const {
    if (!isConnected || !pData) return {0,0,0};
    return ToVec3(pData->fAvatarFront);
}

FVector MumbleLinkReader::GetPlayerTop() const {
    if (!isConnected || !pData) return {0,0,0};
    return ToVec3(pData->fAvatarTop);
}

FVector MumbleLinkReader::GetMapPosition() const {
    if (!isConnected || !pData) return {0,0,0};
    return { pData->context.playerX, 0.0f, pData->context.playerY };
}

float MumbleLinkReader::GetMapRotation() const {
    if (!isConnected || !pData) return 0.0f;
    return pData->context.compassRotation;
}

float MumbleLinkReader::GetFov() const {
    if (!isConnected || !pData) return 0.0f;
    return 0.0f;
}

uint32_t MumbleLinkReader::GetUiTick() const {
    if (!isConnected || !pData) return 0;
    return pData->uiTick;
}

uint32_t MumbleLinkReader::GetContextLength() const {
    if (!isConnected || !pData) return 0;
    return pData->context_len;
}

std::wstring MumbleLinkReader::GetIdentityString() const {
    if (!isConnected || !pData) return L"";
    return std::wstring(pData->identity);
}

uint32_t MumbleLinkReader::GetMapId() const {
    if (!isConnected || !pData) return 0;
    return pData->context.mapId;
}

uint32_t MumbleLinkReader::GetUiState() const {
    if (!isConnected || !pData) return 0;
    return pData->context.uiState;
}

uint32_t MumbleLinkReader::GetMapType() const {
    if (!isConnected || !pData) return 0;
    return pData->context.mapType;
}

uint8_t MumbleLinkReader::GetMountIndex() const {
    if (!isConnected || !pData) return 0;
    return pData->context.mountIndex;
}
