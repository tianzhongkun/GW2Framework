#include "ConfigManager.h"
#include "Logger.h"
#include "../vendor/json/json.hpp"
#include <fstream>
#include <sstream>
#include <windows.h>

using json = nlohmann::json;

class ConfigManager::Impl {
public:
    json data;
    std::string path;
    bool dirty = false;
};

ConfigManager::ConfigManager() : m_impl(new Impl()) {}
ConfigManager::~ConfigManager() = default;

bool ConfigManager::Initialize() {
    // Get DLL directory for config file
    char dllPath[MAX_PATH] = {0};
    HMODULE hm = nullptr;
    
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)this, &hm)) {
        GetModuleFileNameA(hm, dllPath, MAX_PATH);
    }
    
    std::string path(dllPath);
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) {
        m_impl->path = path.substr(0, pos) + "\\gw2framework_config.json";
    } else {
        m_impl->path = "gw2framework_config.json";
    }
    
    LOG_INFO("Config", "Config file: " + m_impl->path);
    
    // Initialize with defaults
    m_impl->data = json::object();
    
    // Try load existing
    if (Load()) {
        LOG_INFO("Config", "Loaded existing config");
    } else {
        // Create default structure
        LOG_INFO("Config", "Creating default config");
        m_impl->data["core"] = {
            {"version", "1.0.0"},
            {"menu_key", "INSERT"},
            {"show_watermark", true},
            {"overlay_opacity", 0.96f}
        };
        m_impl->data["plugins"] = json::object();
        Save();
    }
    return true;
}

bool ConfigManager::Load() {
    std::ifstream file(m_impl->path);
    if (!file.is_open()) return false;
    
    try {
        file >> m_impl->data;
        file.close();
        return true;
    } catch (const json::exception& e) {
        LOG_ERROR("Config", std::string("Parse error: ") + e.what());
        return false;
    }
}

bool ConfigManager::Save() {
    std::ofstream file(m_impl->path);
    if (!file.is_open()) {
        LOG_ERROR("Config", "Cannot write: " + m_impl->path);
        return false;
    }
    
    file << m_impl->data.dump(4);
    file.close();
    m_impl->dirty = false;
    LOG_DEBUG("Config", "Saved config");
    return true;
}

// Helper: navigate to nested key, return pointer or nullptr
static json* navigate(json& root, const std::string& key, bool create = false) {
    size_t start = 0;
    size_t end = key.find('.');
    json* cur = &root;
    
    // Navigate through parent path
    std::string lastKey = key;
    while (end != std::string::npos) {
        std::string seg = key.substr(start, end - start);
        lastKey = key.substr(end + 1);
        
        if (!cur->contains(seg)) {
            if (create) (*cur)[seg] = json::object();
            else return nullptr;
        }
        cur = &(*cur)[seg];
        start = end + 1;
        end = key.find('.', start);
    }
    
    // Now cur is the parent object, lastKey is the final key name
    if (create && !cur->contains(lastKey)) {
        (*cur)[lastKey] = json();
    }
    
    return cur->contains(lastKey) ? &(*cur)[lastKey] : nullptr;
}

bool ConfigManager::GetBool(const std::string& key, bool def) const {
    json* v = navigate(m_impl->data, key);
    return (v && v->is_boolean()) ? v->get<bool>() : def;
}

int ConfigManager::GetInt(const std::string& key, int def) const {
    json* v = navigate(m_impl->data, key);
    return (v && v->is_number_integer()) ? v->get<int>() : def;
}

float ConfigManager::GetFloat(const std::string& key, float def) const {
    json* v = navigate(m_impl->data, key);
    return (v && v->is_number()) ? (float)v->get<double>() : def;
}

std::string ConfigManager::GetString(const std::string& key, const std::string& def) const {
    json* v = navigate(m_impl->data, key);
    return (v && v->is_string()) ? v->get<std::string>() : def;
}

void ConfigManager::SetBool(const std::string& key, bool val) {
    json* v = navigate(m_impl->data, key, true);
    if (v) *v = val;
    else m_impl->data[key] = val;
    m_impl->dirty = true;
}

void ConfigManager::SetInt(const std::string& key, int val) {
    json* v = navigate(m_impl->data, key, true);
    if (v) *v = val;
    else m_impl->data[key] = val;
    m_impl->dirty = true;
}

void ConfigManager::SetFloat(const std::string& key, float val) {
    json* v = navigate(m_impl->data, key, true);
    if (v) *v = (double)val;
    else m_impl->data[key] = val;
    m_impl->dirty = true;
}

void ConfigManager::SetString(const std::string& key, const std::string& val) {
    json* v = navigate(m_impl->data, key, true);
    if (v) *v = val;
    else m_impl->data[key] = val;
    m_impl->dirty = true;
}

const std::string& ConfigManager::GetPath() const {
    return m_impl->path;
}
