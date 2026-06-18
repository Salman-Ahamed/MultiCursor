#pragma once

#include "Types.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>

class ProfileManager {
public:
    ProfileManager(const std::wstring& profileDir);
    ~ProfileManager() = default;

    CursorProfile LoadProfile(const std::wstring& devicePath);
    void SaveProfile(const std::wstring& devicePath, const CursorProfile& profile);
    bool DeleteProfile(const std::wstring& devicePath);
    bool ProfileExists(const std::wstring& devicePath) const;
    std::unordered_map<std::wstring, CursorProfile> GetAllProfiles();
    CursorProfile DefaultProfile(UINT index) const;

private:
    std::wstring ProfilePath(const std::wstring& devicePath) const;
    std::wstring HashDevicePath(const std::wstring& devicePath) const;

    std::wstring m_profileDir;
    mutable std::mutex m_mutex;
};
