#pragma once

#include "Plugin.h"
#include "ConfigManager.h"
#include <vector>
#include <memory>

class PluginManager {
public:
    static PluginManager& Instance() {
        static PluginManager inst;
        return inst;
    }

    // Lifecycle
    void Initialize();
    void RegisterDefaults();
    void Shutdown();
    
    // Config access
    ConfigManager& GetConfig() { return ConfigManager::Instance(); }
    
    // Plugin management
    const std::vector<std::unique_ptr<IPlugin>>& GetPlugins() const;
    IPlugin* GetPlugin(const std::string& id);
    
    // Updates
    void UpdateAll();
    void RenderOverlayAll();
    
    // Config persistence
    void LoadConfig();
    void SaveConfig();

private:
    PluginManager() = default;
    
    bool m_initialized = false;
    bool m_registered = false;
    std::vector<std::unique_ptr<IPlugin>> m_plugins;
};

// Global for legacy code compatibility
extern PluginManager* g_pluginManagerRuntime;
