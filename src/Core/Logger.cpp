#include "Logger.h"
#include <cstdio>
#include <cwchar>
#include <vector>
#include <filesystem>

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    std::lock_guard lock(m_mutex);
    if (m_file.is_open())
        m_file.close();
}

void Logger::Init(const std::wstring& directory) {
    std::lock_guard lock(m_mutex);
    m_directory = directory;
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    tm local;
    localtime_s(&local, &tt);
    m_day = local.tm_mday;
    m_rotationCount = 0;
    auto path = m_directory + L"\\" + FilenameForDate();
    m_file.open(path, std::ios::app);
}

void Logger::Log(Severity sev, const wchar_t* fmt, ...) {
    if (sev < m_minSeverity) return;

    std::lock_guard lock(m_mutex);

    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    tm local;
    localtime_s(&local, &tt);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_point::time_since_epoch()) % 1000;

    RotateIfNeeded();

    if (!m_file.is_open()) {
        auto path = m_directory + L"\\" + FilenameForDate();
        m_file.open(path, std::ios::app);
    }

    wchar_t buf[4096];
    va_list args;
    va_start(args, fmt);
    vswprintf_s(buf, fmt, args);
    va_end(args);

    wchar_t line[4608];
    swprintf_s(line, L"[%04d-%02d-%02d %02d:%02d:%02d.%03d][%s] %s\n",
        1900 + local.tm_year, 1 + local.tm_mon, local.tm_mday,
        local.tm_hour, local.tm_min, local.tm_sec, (int)ms.count(),
        SeverityStr(sev), buf);

    m_file << line;
    m_file.flush();

    OutputDebugStringW(line);
    fputws(line, stdout);
    fflush(stdout);
}

const wchar_t* Logger::SeverityStr(Severity s) {
    switch (s) {
    case Severity::Debug: return L"DBG";
    case Severity::Info:  return L"INF";
    case Severity::Warn:  return L"WRN";
    case Severity::Error: return L"ERR";
    default: return L"???";
    }
}

void Logger::RotateIfNeeded() {
    if (!m_file.is_open()) return;
    auto pos = m_file.tellp();
    if (pos > kMaxFileSize) {
        m_file.close();
        m_rotationCount++;

        auto base = m_directory + L"\\" + FilenameForDate();
        std::wstring old = base + L"." + std::to_wstring(m_rotationCount - 1);
        std::wstring current = base;
        _wrename(current.c_str(), old.c_str());

        // Remove excess old files beyond the limit
        std::wstring oldPrefix = base.substr(0, base.find_last_of(L'.'));
        int count = 0;
        for (auto& entry : std::filesystem::directory_iterator(m_directory)) {
            if (entry.path().wstring().find(oldPrefix) == 0 && entry.path().extension() != L".log") {
                count++;
            }
        }
        if (count > kMaxLogFiles) {
            int toRemove = count - kMaxLogFiles;
            for (auto& entry : std::filesystem::directory_iterator(m_directory)) {
                if (toRemove <= 0) break;
                if (entry.path().wstring().find(oldPrefix) == 0 && entry.path().extension() != L".log") {
                    std::filesystem::remove(entry.path());
                    toRemove--;
                }
            }
        }
    }
}

std::wstring Logger::FilenameForDate() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    tm local;
    localtime_s(&local, &tt);
    wchar_t buf[64];
    swprintf_s(buf, L"multicursor_%04d%02d%02d.log",
        1900 + local.tm_year, 1 + local.tm_mon, local.tm_mday);
    return buf;
}
