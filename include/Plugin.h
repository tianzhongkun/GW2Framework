#pragma once

#include <string>
#include <vector>

struct PluginFeature
{
    std::string text;
};

class IPlugin
{
public:
    virtual ~IPlugin() = default;

    virtual const char* GetId() const = 0;
    virtual const char* GetCategory() const = 0;
    virtual const char* GetName() const = 0;
    virtual const char* GetSubtitle() const = 0;
    virtual std::vector<PluginFeature> GetFeatures() const = 0;

    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
    virtual bool IsAvailable() const = 0;

    virtual void OnUpdate() {}
    virtual void OnLoadConfig() {}
    virtual void OnSaveConfig() const {}
    virtual void OnRenderConfig() {}
    virtual void OnRenderOverlay() {}
};
