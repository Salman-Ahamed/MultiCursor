#include "ProfileManager.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <functional>
#include <cctype>

static std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &result[0], size, nullptr, nullptr);
    return result;
}

static std::wstring UTF8ToWString(const std::string& str) {
    if (str.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &result[0], size);
    return result;
}

static std::wstring ToHex(size_t val) {
    wchar_t buf[32];
    swprintf_s(buf, L"%016zX", val);
    return buf;
}

ProfileManager::ProfileManager(const std::wstring& profileDir) : m_profileDir(profileDir) {
    std::filesystem::create_directories(profileDir);
}

std::wstring ProfileManager::HashDevicePath(const std::wstring& devicePath) const {
    std::hash<std::wstring> hasher;
    return ToHex(hasher(devicePath));
}

std::wstring ProfileManager::ProfilePath(const std::wstring& devicePath) const {
    return m_profileDir + L"\\" + HashDevicePath(devicePath) + L".json";
}

namespace {
    std::string JsonEscape(const std::string& s) {
        std::string out;
        out.reserve(s.size() + 2);
        for (char c : s) {
            switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c;
            }
        }
        return out;
    }

    std::string SerializeProfile(const CursorProfile& p) {
        std::string json = "{\n";
        json += "  \"color\": " + std::to_string(p.color) + ",\n";
        json += "  \"label\": \"" + JsonEscape(WStringToUTF8(p.label)) + "\",\n";
        json += "  \"cursorSize\": " + std::to_string(p.cursorSize) + ",\n";
        json += "  \"opacity\": " + std::to_string(p.opacity) + ",\n";
        json += "  \"speedMultiplier\": " + std::to_string(p.speedMultiplier) + ",\n";
        json += "  \"swapButtons\": " + std::string(p.swapButtons ? "true" : "false") + ",\n";
        json += "  \"reverseScroll\": " + std::string(p.reverseScroll ? "true" : "false") + ",\n";
        json += "  \"scrollLines\": " + std::to_string(p.scrollLines) + ",\n";
        json += "  \"snapToDefault\": " + std::string(p.snapToDefault ? "true" : "false") + ",\n";
        json += "  \"enhancePrecision\": " + std::string(p.enhancePrecision ? "true" : "false") + ",\n";
        json += "  \"doubleClickSpeed\": " + std::to_string(p.doubleClickSpeed) + ",\n";
        json += "  \"clickLock\": " + std::string(p.clickLock ? "true" : "false") + ",\n";
        json += "  \"monitorBinding\": \"" + JsonEscape(WStringToUTF8(p.monitorBinding)) + "\"\n";
        json += "}\n";
        return json;
    }
}

CursorProfile ProfileManager::DefaultProfile(UINT index) const {
    CursorProfile p;
    p.color = kCursorColors[index % kCursorColorCount];
    p.label = L"Mouse " + std::to_wstring(index + 1);
    return p;
}

CursorProfile ProfileManager::LoadProfile(const std::wstring& devicePath) {
    std::lock_guard lock(m_mutex);
    auto path = ProfilePath(devicePath);

    std::ifstream f(path);
    if (!f.is_open()) {
        LOG_DEBUG(L"No profile found for device, using defaults");
        return DefaultProfile(0);
    }

    std::stringstream ss;
    ss << f.rdbuf();
    std::string json = ss.str();

    CursorProfile p = DefaultProfile(0);

    auto findValue = [&](const std::string& key) -> std::string {
        auto pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return {};
        pos = json.find(':', pos + key.size() + 2);
        if (pos == std::string::npos) return {};
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
            pos++;
        if (pos >= json.size()) return {};
        if (json[pos] == '"') {
            pos++;
            std::string val;
            while (pos < json.size() && json[pos] != '"') {
                if (json[pos] == '\\' && pos + 1 < json.size()) { pos++; }
                val += json[pos];
                pos++;
            }
            return val;
        }
        if (json[pos] == 't' || json[pos] == 'f') {
            return json.substr(pos, json[pos] == 't' ? 4 : 5);
        }
        size_t end = pos;
        while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != '\n' && json[end] != '\r') end++;
        return json.substr(pos, end - pos);
    };

    auto val = findValue("color");
    if (!val.empty()) p.color = (DWORD)std::stoul(val);
    val = findValue("label");
    if (!val.empty()) p.label = UTF8ToWString(val);
    val = findValue("cursorSize");
    if (!val.empty()) p.cursorSize = std::stof(val);
    val = findValue("opacity");
    if (!val.empty()) p.opacity = std::stof(val);
    val = findValue("speedMultiplier");
    if (!val.empty()) p.speedMultiplier = std::stof(val);
    val = findValue("swapButtons");
    if (!val.empty()) p.swapButtons = (val == "true");
    val = findValue("reverseScroll");
    if (!val.empty()) p.reverseScroll = (val == "true");
    val = findValue("scrollLines");
    if (!val.empty()) p.scrollLines = std::stoi(val);
    val = findValue("snapToDefault");
    if (!val.empty()) p.snapToDefault = (val == "true");
    val = findValue("enhancePrecision");
    if (!val.empty()) p.enhancePrecision = (val == "true");
    val = findValue("doubleClickSpeed");
    if (!val.empty()) p.doubleClickSpeed = std::stoi(val);
    val = findValue("clickLock");
    if (!val.empty()) p.clickLock = (val == "true");
    val = findValue("monitorBinding");
    if (!val.empty()) p.monitorBinding = UTF8ToWString(val);

    return p;
}

void ProfileManager::SaveProfile(const std::wstring& devicePath, const CursorProfile& profile) {
    std::lock_guard lock(m_mutex);
    auto path = ProfilePath(devicePath);

    std::ofstream f(path);
    if (!f.is_open()) {
        LOG_ERROR(L"Failed to save profile: %s", path.c_str());
        return;
    }
    f << SerializeProfile(profile);
    LOG_DEBUG(L"Profile saved for device hash %s", HashDevicePath(devicePath).c_str());
}

bool ProfileManager::DeleteProfile(const std::wstring& devicePath) {
    std::lock_guard lock(m_mutex);
    return std::filesystem::remove(ProfilePath(devicePath));
}

bool ProfileManager::ProfileExists(const std::wstring& devicePath) const {
    std::lock_guard lock(m_mutex);
    return std::filesystem::exists(ProfilePath(devicePath));
}

std::unordered_map<std::wstring, CursorProfile> ProfileManager::GetAllProfiles() {
    std::lock_guard lock(m_mutex);
    std::unordered_map<std::wstring, CursorProfile> profiles;
    for (auto& entry : std::filesystem::directory_iterator(m_profileDir)) {
        if (entry.path().extension() != L".json") continue;
        auto hashOnly = entry.path().stem().wstring();
        std::ifstream f(entry.path());
        if (!f.is_open()) continue;
        std::stringstream ss;
        ss << f.rdbuf();
        std::string json = ss.str();
        CursorProfile p = DefaultProfile(0);
        auto findVal = [&](const std::string& key) -> std::string {
            auto pos = json.find("\"" + key + "\"");
            if (pos == std::string::npos) return {};
            pos = json.find(':', pos + key.size() + 2);
            if (pos == std::string::npos) return {};
            pos++;
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) pos++;
            if (pos >= json.size()) return {};
            if (json[pos] == '"') {
                pos++;
                std::string val;
                while (pos < json.size() && json[pos] != '"') {
                    if (json[pos] == '\\' && pos + 1 < json.size()) pos++;
                    val += json[pos];
                    pos++;
                }
                return val;
            }
            if (json[pos] == 't' || json[pos] == 'f') return json.substr(pos, json[pos] == 't' ? 4 : 5);
            size_t end = pos;
            while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != '\n' && json[end] != '\r') end++;
            return json.substr(pos, end - pos);
        };
        auto v = findVal("color"); if (!v.empty()) p.color = (DWORD)std::stoul(v);
        v = findVal("label"); if (!v.empty()) p.label = UTF8ToWString(v);
        v = findVal("cursorSize"); if (!v.empty()) p.cursorSize = std::stof(v);
        v = findVal("opacity"); if (!v.empty()) p.opacity = std::stof(v);
        v = findVal("speedMultiplier"); if (!v.empty()) p.speedMultiplier = std::stof(v);
        v = findVal("swapButtons"); if (!v.empty()) p.swapButtons = (v == "true");
        v = findVal("reverseScroll"); if (!v.empty()) p.reverseScroll = (v == "true");
        v = findVal("scrollLines"); if (!v.empty()) p.scrollLines = std::stoi(v);
        v = findVal("snapToDefault"); if (!v.empty()) p.snapToDefault = (v == "true");
        v = findVal("enhancePrecision"); if (!v.empty()) p.enhancePrecision = (v == "true");
        v = findVal("doubleClickSpeed"); if (!v.empty()) p.doubleClickSpeed = std::stoi(v);
        v = findVal("clickLock"); if (!v.empty()) p.clickLock = (v == "true");
        v = findVal("monitorBinding"); if (!v.empty()) p.monitorBinding = UTF8ToWString(v);
        profiles[hashOnly] = p;
    }
    return profiles;
}
