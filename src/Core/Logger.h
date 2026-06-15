#pragma once

#include "Types.h"
#include <fstream>
#include <mutex>
#include <chrono>
#include <string>

class Logger {
public:
    static Logger& Instance();
    void Init(const std::wstring& directory);
    void Log(Severity sev, const wchar_t* fmt, ...);
    void SetLevel(Severity minSev) { m_minSeverity = minSev; }

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void RotateIfNeeded();
    std::wstring FilenameForDate();
    static const wchar_t* SeverityStr(Severity s);

    std::mutex m_mutex;
    std::wofstream m_file;
    Severity m_minSeverity = Severity::Debug;
    std::wstring m_directory = L".";
    int m_day = 0;
    static constexpr int64_t kMaxFileSize = 10 * 1024 * 1024;
};

#define LOG_DEBUG(...) Logger::Instance().Log(Severity::Debug, __VA_ARGS__)
#define LOG_INFO(...)  Logger::Instance().Log(Severity::Info, __VA_ARGS__)
#define LOG_WARN(...)  Logger::Instance().Log(Severity::Warn, __VA_ARGS__)
#define LOG_ERROR(...) Logger::Instance().Log(Severity::Error, __VA_ARGS__)
