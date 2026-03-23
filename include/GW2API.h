#pragma once
#include <string>
#include <cstdint>

struct Vec3 { float x, y, z; };

class IGW2API {
public:
    virtual ~IGW2API() = default;
    virtual std::string GetCharacterName() const = 0;
    virtual uint32_t GetProfession() const = 0;
    virtual uint32_t GetLevel() const = 0;
    virtual float GetHealthPct() const = 0;
    virtual float GetEnergyPct() const = 0;
    virtual Vec3 GetPosition() const = 0;
    virtual Vec3 GetCameraDir() const = 0;
    virtual std::string GetTargetName() const = 0;
    virtual float GetTargetHealthPct() const = 0;
    virtual bool HasTarget() const = 0;
    virtual bool IsSkillReady(uint32_t skill_id) const = 0;
    virtual float GetSkillCooldown(uint32_t skill_id) const = 0;
    virtual uintptr_t GetBaseAddr() const = 0;
    virtual void Update() = 0; // 新增每帧更新
};

extern "C" __declspec(dllexport) IGW2API* CreateAPI();
extern "C" __declspec(dllexport) void DestroyAPI(IGW2API* api);
