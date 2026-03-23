// GW2API.cpp - Final Fixed Version
#include "GW2API.h"
#include "MumbleLink.h"
#include "MemoryReader.h"
#include <sstream>

// Global static variables
static MumbleLinkReader* g_mumble = nullptr;
static MemoryReader* g_memReader = nullptr;
static bool g_initialized = false;

// Forward declaration
void InitializeOnce();

// Initialization function
void InitializeOnce() {
    if (g_initialized) return;
    
    g_mumble = new MumbleLinkReader();
    g_mumble->Initialize();
    
    g_memReader = new MemoryReader();
    g_memReader->Attach();
    
    g_initialized = true;
}

class GW2API : public IGW2API {
public:
    GW2API() {
        if (!g_initialized) InitializeOnce();
        if (g_mumble && !g_mumble->IsConnected()) {
            g_mumble->Initialize();
        }
    }
    
    ~GW2API() {}
    
    std::string GetCharacterName() const override {
        if (!g_mumble || !g_mumble->IsConnected()) return "Unknown";
        return "Player";
    }
    
    uint32_t GetProfession() const override {
        return 0;
    }
    
    uint32_t GetLevel() const override {
        return 80;
    }
    
    float GetHealthPct() const override {
        return 1.0f;
    }
    
    float GetEnergyPct() const override {
        return 1.0f;
    }
    
    Vec3 GetPosition() const override {
        if (!g_mumble || !g_mumble->IsConnected()) return {0,0,0};
        FVector pos = g_mumble->GetPlayerPos();
        return {pos.x, pos.y, pos.z};
    }
    
    Vec3 GetCameraDir() const override {
        if (!g_mumble || !g_mumble->IsConnected()) return {0,0,0};
        FVector dir = g_mumble->GetCameraDir();
        return {dir.x, dir.y, dir.z};
    }
    
    std::string GetTargetName() const override {
        return "Dummy";
    }
    
    float GetTargetHealthPct() const override {
        return 1.0f;
    }
    
    bool HasTarget() const override {
        return false;
    }
    
    bool IsSkillReady(uint32_t) const override {
        return true;
    }
    
    float GetSkillCooldown(uint32_t) const override {
        return 0.0f;
    }
    
    uintptr_t GetBaseAddr() const override {
        return g_memReader ? g_memReader->GetBaseAddress() : 0;
    }
    
    void Update() override {
        if (!g_initialized) InitializeOnce();
        if (g_mumble) g_mumble->Update();
    }
};

extern "C" __declspec(dllexport) IGW2API* CreateAPI() {
    if (!g_initialized) InitializeOnce();
    return new GW2API();
}

extern "C" __declspec(dllexport) void DestroyAPI(IGW2API* api) {
    delete api;
}
