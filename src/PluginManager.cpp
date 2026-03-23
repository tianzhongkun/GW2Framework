#include "PluginManager.h"
#include "plugins/CombatPlugin.h"
#include "Logger.h"
#include <utility>

PluginManager* g_pluginManagerRuntime = nullptr;

void PluginManager::Initialize() {
    if (m_initialized) return;
    
    LOG_INFO("PluginManager", "Initializing...");
    
    // Initialize config first
    ConfigManager::Instance().Initialize();
    
    // Register default plugins
    RegisterDefaults();
    
    // Load saved config
    LoadConfig();
    
    g_pluginManagerRuntime = this;
    m_initialized = true;
    
    LOG_INFO("PluginManager", "Initialized with " + std::to_string(m_plugins.size()) + " plugins");
}

void PluginManager::Shutdown() {
    if (!m_initialized) return;
    
    LOG_INFO("PluginManager", "Shutting down...");
    
    // Save config before exit
    SaveConfig();
    
    m_plugins.clear();
    g_pluginManagerRuntime = nullptr;
    m_initialized = false;
}

namespace {
    // Basic plugin implementation for placeholder plugins
    class BasicPlugin final : public IPlugin {
    public:
        BasicPlugin(
            const char* id,
            const char* category,
            const char* name,
            const char* subtitle,
            std::vector<PluginFeature> features,
            bool available = true)
            : m_id(id), m_category(category), m_name(name), 
              m_subtitle(subtitle), m_features(std::move(features)), 
              m_available(available)
        {}

        const char* GetId() const override { return m_id.c_str(); }
        const char* GetCategory() const override { return m_category.c_str(); }
        const char* GetName() const override { return m_name.c_str(); }
        const char* GetSubtitle() const override { return m_subtitle.c_str(); }
        std::vector<PluginFeature> GetFeatures() const override { return m_features; }
        bool IsEnabled() const override { return m_enabled; }
        void SetEnabled(bool enabled) override { m_enabled = enabled; }
        bool IsAvailable() const override { return m_available; }
        
        void OnLoadConfig() override {
            m_enabled = ConfigManager::Instance().GetBool(
                std::string("plugins.") + m_id + ".enabled", false);
        }
        
        void OnSaveConfig() const override {
            ConfigManager::Instance().SetBool(
                std::string("plugins.") + m_id + ".enabled", m_enabled);
        }

    private:
        std::string m_id, m_category, m_name, m_subtitle;
        std::vector<PluginFeature> m_features;
        bool m_enabled = false;
        bool m_available;
    };
}

void PluginManager::RegisterDefaults() {
    if (m_registered) return;
    m_registered = true;

    // Combat Plugin - real implementation
    m_plugins.emplace_back(std::make_unique<CombatPlugin>());

    // Placeholder plugins
    m_plugins.emplace_back(std::make_unique<BasicPlugin>(
        "boss_timers", "首领", "Boss Mechanic Timers", 
        "首领机制计时与预警系统",
        std::vector<PluginFeature>{{"机制计时器"}, {"首领机制提醒"}, {"3D 机制可视化"}, {"团队提示占位"}},
        false));  // Not available yet

    m_plugins.emplace_back(std::make_unique<BasicPlugin>(
        "item_automation", "物品", "Item Automation", 
        "背包与交易行自动化",
        std::vector<PluginFeature>{{"自动卖到 TP"}, {"自动存材料"}, {"自动销毁垃圾"}, {"更多规则扩展"}},
        false));

    m_plugins.emplace_back(std::make_unique<BasicPlugin>(
        "teleport_jp", "移动", "Teleport & JP", 
        "地图移动与跳跳乐工具",
        std::vector<PluginFeature>{{"传送到地图任意位置"}, {"完成 jumping puzzle"}, {"路径记录"}, {"更多移动功能"}},
        false));

    m_plugins.emplace_back(std::make_unique<BasicPlugin>(
        "inspect_others", "观察", "Inspect Others", 
        "查看他人配装和特性",
        std::vector<PluginFeature>{{"查看装备"}, {"查看特性"}, {"目标面板集成"}, {"小队检查扩展"}},
        false));

    m_plugins.emplace_back(std::make_unique<BasicPlugin>(
        "boons_overlay", "增益", "Boons Overlay", 
        "头顶增益与自定义显示",
        std::vector<PluginFeature>{{"玩家头顶 boon"}, {"自定义 boon 样式"}, {"过滤器"}, {"团队状态扩展"}},
        false));

    m_plugins.emplace_back(std::make_unique<BasicPlugin>(
        "wizards_vault", "日常", "Wizards Vault Helper", 
        "日常奖励和 Buff 自动化",
        std::vector<PluginFeature>{{"自动领取巫师宝库"}, {"自动补 reinforced armor"}, {"提醒系统"}, {"更多日常助手"}},
        false));

    LOG_INFO("PluginManager", "Registered " + std::to_string(m_plugins.size()) + " plugins");
}

const std::vector<std::unique_ptr<IPlugin>>& PluginManager::GetPlugins() const {
    return m_plugins;
}

IPlugin* PluginManager::GetPlugin(const std::string& id) {
    for (const auto& plugin : m_plugins) {
        if (plugin->GetId() == id) return plugin.get();
    }
    return nullptr;
}

void PluginManager::UpdateAll() {
    for (const auto& plugin : m_plugins) {
        if (plugin->IsEnabled()) {
            plugin->OnUpdate();
        }
    }
}

void PluginManager::RenderOverlayAll() {
    for (const auto& plugin : m_plugins) {
        if (plugin->IsEnabled()) {
            plugin->OnRenderOverlay();
        }
    }
}

void PluginManager::LoadConfig() {
    for (const auto& plugin : m_plugins) {
        plugin->OnLoadConfig();
    }
    LOG_DEBUG("PluginManager", "Loaded plugin configs");
}

void PluginManager::SaveConfig() {
    for (const auto& plugin : m_plugins) {
        plugin->OnSaveConfig();
    }
    ConfigManager::Instance().Save();
    LOG_DEBUG("PluginManager", "Saved plugin configs");
}
