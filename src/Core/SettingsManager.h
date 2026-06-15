#pragma once

#include "Types.h"
#include <string>
#include <unordered_map>
#include <vector>

class SettingsManager {
public:
    bool Load(const std::wstring& path);
    bool Save();

    bool GetBool(const std::string& key, bool def = false) const;
    int GetInt(const std::string& key, int def = 0) const;
    float GetFloat(const std::string& key, float def = 0.0f) const;
    std::wstring GetString(const std::string& key, const std::wstring& def = L"") const;

    void SetBool(const std::string& key, bool val);
    void SetInt(const std::string& key, int val);
    void SetFloat(const std::string& key, float val);
    void SetString(const std::string& key, const std::wstring& val);

    const std::wstring& Path() const { return m_path; }

private:
    bool Parse(const std::string& json);
    std::string Serialize() const;

    struct Value {
        enum Type { Bool, Int, Float, String };
        Type type;
        union {
            bool b;
            int i;
            float f;
        };
        std::wstring s;
    };

    std::unordered_map<std::string, Value> m_values;
    std::wstring m_path;
};
