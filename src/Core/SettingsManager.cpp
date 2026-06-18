#include "SettingsManager.h"
#include <fstream>
#include <sstream>
#include <cctype>

bool SettingsManager::Load(const std::wstring& path) {
    std::lock_guard lock(m_mutex);
    m_path = path;
    std::ifstream f(path);
    if (!f.is_open()) {
        {
            Value v; v.type = Value::Bool; v.b = true;
            m_values["overlay_enabled"] = v;
        }
        {
            Value v; v.type = Value::Int; v.i = 12;
            m_values["cursor_size"] = v;
        }
        {
            Value v; v.type = Value::Float; v.f = 0.8f;
            m_values["cursor_opacity"] = v;
        }
        {
            Value v; v.type = Value::Bool; v.b = true;
            m_values["show_labels"] = v;
        }
        {
            Value v; v.type = Value::String; v.s = L"info";
            m_values["log_level"] = v;
        }
        return Save();
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return Parse(ss.str());
}

bool SettingsManager::Save() {
    std::lock_guard lock(m_mutex);
    std::ofstream f(m_path);
    if (!f.is_open()) return false;
    f << Serialize();
    if (m_changeCb) m_changeCb();
    AppSettings settings;
    settings.overlayEnabled = GetBool("overlay_enabled", true);
    settings.showLabels = GetBool("show_labels", true);
    settings.globalOpacity = GetFloat("cursor_opacity", 0.8f);
    settings.logLevel = GetInt("log_level_int", 1);
    settings.startWithWindows = GetBool("start_with_windows", false);
    settings.minimizeToTray = GetBool("minimize_to_tray", true);
    m_settingsBus.Publish(settings);
    return true;
}

bool SettingsManager::GetBool(const std::string& key, bool def) const {
    std::lock_guard lock(m_mutex);
    auto it = m_values.find(key);
    return (it != m_values.end() && it->second.type == Value::Bool) ? it->second.b : def;
}

int SettingsManager::GetInt(const std::string& key, int def) const {
    std::lock_guard lock(m_mutex);
    auto it = m_values.find(key);
    return (it != m_values.end() && it->second.type == Value::Int) ? it->second.i : def;
}

float SettingsManager::GetFloat(const std::string& key, float def) const {
    std::lock_guard lock(m_mutex);
    auto it = m_values.find(key);
    return (it != m_values.end() && it->second.type == Value::Float) ? it->second.f : def;
}

std::wstring SettingsManager::GetString(const std::string& key, const std::wstring& def) const {
    std::lock_guard lock(m_mutex);
    auto it = m_values.find(key);
    return (it != m_values.end() && it->second.type == Value::String) ? it->second.s : def;
}

void SettingsManager::SetBool(const std::string& key, bool val) {
    std::lock_guard lock(m_mutex);
    Value v; v.type = Value::Bool; v.b = val;
    m_values[key] = v;
}

void SettingsManager::SetInt(const std::string& key, int val) {
    std::lock_guard lock(m_mutex);
    Value v; v.type = Value::Int; v.i = val;
    m_values[key] = v;
}

void SettingsManager::SetFloat(const std::string& key, float val) {
    std::lock_guard lock(m_mutex);
    Value v; v.type = Value::Float; v.f = val;
    m_values[key] = v;
}

void SettingsManager::SetString(const std::string& key, const std::wstring& val) {
    std::lock_guard lock(m_mutex);
    Value v; v.type = Value::String; v.s = val;
    m_values[key] = v;
}

void SettingsManager::Reset() {
    std::lock_guard lock(m_mutex);
    m_values.clear();
    {
        Value v; v.type = Value::Bool; v.b = true;
        m_values["overlay_enabled"] = v;
    }
    {
        Value v; v.type = Value::Int; v.i = 12;
        m_values["cursor_size"] = v;
    }
    {
        Value v; v.type = Value::Float; v.f = 0.8f;
        m_values["cursor_opacity"] = v;
    }
    {
        Value v; v.type = Value::Bool; v.b = true;
        m_values["show_labels"] = v;
    }
    {
        Value v; v.type = Value::String; v.s = L"info";
        m_values["log_level"] = v;
    }
}

bool SettingsManager::Parse(const std::string& json) {
    size_t pos = 0;
    auto skipWS = [&]() {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
            pos++;
    };
    auto parseString = [&]() -> std::string {
        if (pos >= json.size() || json[pos] != '"') return {};
        pos++;
        std::string s;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                pos++;
                switch (json[pos]) {
                case '"': s += '"'; break;
                case '\\': s += '\\'; break;
                case 'n': s += '\n'; break;
                case 'r': s += '\r'; break;
                case 't': s += '\t'; break;
                default: s += json[pos];
                }
            } else {
                s += json[pos];
            }
            pos++;
        }
        if (pos < json.size()) pos++;
        return s;
    };

    skipWS();
    if (pos >= json.size() || json[pos] != '{') return false;
    pos++; skipWS();

    while (pos < json.size() && json[pos] != '}') {
        auto key = parseString();
        skipWS();
        if (pos >= json.size() || json[pos] != ':') return false;
        pos++; skipWS();

        if (pos >= json.size()) return false;

        if (json[pos] == '"') {
            auto val = parseString();
            Value v; v.type = Value::String; v.s = std::wstring(val.begin(), val.end());
            m_values[key] = v;
        } else if (json[pos] == 't' || json[pos] == 'f') {
            bool b = (json[pos] == 't');
            Value v; v.type = Value::Bool; v.b = b;
            m_values[key] = v;
            pos += b ? 4 : 5;
        } else if (json[pos] == 'n') {
            pos += 4;
        } else {
            char* end = nullptr;
            double d = strtod(json.c_str() + pos, &end);
            if (end == json.c_str() + pos) return false;
            bool isInt = (d == (int)d);
            for (const char* p = json.c_str() + pos; p < end; p++) {
                if (*p == '.') { isInt = false; break; }
            }
            if (isInt) {
                Value v; v.type = Value::Int; v.i = (int)d;
                m_values[key] = v;
            } else {
                Value v; v.type = Value::Float; v.f = (float)d;
                m_values[key] = v;
            }
            pos = end - json.c_str();
        }

        skipWS();
        if (pos < json.size() && json[pos] == ',') { pos++; skipWS(); }
    }
    return true;
}

std::string SettingsManager::Serialize() const {
    std::string json = "{\n";
    bool first = true;
    for (auto& [key, val] : m_values) {
        if (!first) json += ",\n";
        first = false;
        json += "  \"" + key + "\": ";
        switch (val.type) {
        case Value::Bool:   json += val.b ? "true" : "false"; break;
        case Value::Int:    json += std::to_string(val.i); break;
        case Value::Float:  json += std::to_string(val.f); break;
        case Value::String: {
            std::string narrow(val.s.begin(), val.s.end());
            json += "\"" + narrow + "\"";
            break;
        }
        }
    }
    json += "\n}\n";
    return json;
}
