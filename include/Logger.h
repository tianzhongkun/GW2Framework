#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void Initialize(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) m_file.close();
        m_file.open(path, std::ios::out | std::ios::app);
        m_initialized = m_file.is_open();
        if (m_initialized) {
            m_file << "\n========================================\n";
            m_file << "GW2Framework Log Started\n";
            m_file << "========================================\n";
        }
    }

    void Log(LogLevel level, const std::string& tag, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_initialized) return;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm tm_buf;
        localtime_s(&tm_buf, &time);

        m_file << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        m_file << "." << std::setfill('0') << std::setw(3) << ms.count();

        const char* levelStr = "????";
        switch (level) {
            case LogLevel::Debug:   levelStr = "DEBG"; break;
            case LogLevel::Info:    levelStr = "INFO"; break;
            case LogLevel::Warning: levelStr = "WARN"; break;
            case LogLevel::Error:   levelStr = "ERRO"; break;
        }

        m_file << " [" << levelStr << "] [" << tag << "] " << message << "\n";
        m_file.flush();
    }

    void Debug(const std::string& tag, const std::string& msg) { Log(LogLevel::Debug, tag, msg); }
    void Info(const std::string& tag, const std::string& msg) { Log(LogLevel::Info, tag, msg); }
    void Warning(const std::string& tag, const std::string& msg) { Log(LogLevel::Warning, tag, msg); }
    void Error(const std::string& tag, const std::string& msg) { Log(LogLevel::Error, tag, msg); }

private:
    Logger() = default;
    ~Logger() { if (m_file.is_open()) m_file.close(); }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream m_file;
    std::mutex m_mutex;
    bool m_initialized = false;
};

#define LOG_DEBUG(tag, msg) Logger::Instance().Debug(tag, msg)
#define LOG_INFO(tag, msg) Logger::Instance().Info(tag, msg)
#define LOG_WARN(tag, msg) Logger::Instance().Warning(tag, msg)
#define LOG_ERROR(tag, msg) Logger::Instance().Error(tag, msg)
