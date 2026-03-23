#pragma once

#include <string>
#include <memory>

class ConfigManager {
public:
    static ConfigManager& Instance() {
        static ConfigManager inst;
        return inst;
    }

    bool Initialize();
    bool Load();
    bool Save();

    bool GetBool(const std::string& key, bool def = false) const;
    int GetInt(const std::string& key, int def = 0) const;
    float GetFloat(const std::string& key, float def = 0.0f) const;
    std::string GetString(const std::string& key, const std::string& def = "") const;

    void SetBool(const std::string& key, bool val);
    void SetInt(const std::string& key, int val);
    void SetFloat(const std::string& key, float val);
    void SetString(const std::string& key, const std::string& val);

    const std::string& GetPath() const;

private:
    ConfigManager();
    ~ConfigManager();

    class Impl;
    std::unique_ptr<Impl> m_impl;
};
