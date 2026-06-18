#pragma once

#include "Types.h"
#include <functional>
#include <vector>
#include <algorithm>
#include <mutex>

template<typename T>
class EventBus {
public:
    using Token = size_t;
    using Callback = std::function<void(const T&)>;

    Token Subscribe(Callback cb) {
        std::lock_guard lock(m_mutex);
        m_callbacks.push_back({ ++m_nextToken, std::move(cb) });
        return m_nextToken;
    }

    void Unsubscribe(Token token) {
        std::lock_guard lock(m_mutex);
        auto it = std::remove_if(m_callbacks.begin(), m_callbacks.end(),
            [token](const Entry& e) { return e.token == token; });
        m_callbacks.erase(it, m_callbacks.end());
    }

    void Publish(const T& event) {
        std::vector<Entry> cbs;
        {
            std::lock_guard lock(m_mutex);
            cbs = m_callbacks;
        }
        for (auto& entry : cbs) {
            entry.cb(event);
        }
    }

private:
    struct Entry {
        Token token;
        Callback cb;
    };
    std::vector<Entry> m_callbacks;
    Token m_nextToken = 0;
    mutable std::mutex m_mutex;
};

using DeviceEventBus = EventBus<DeviceEvent>;
using CursorEventBus = EventBus<CursorEvent>;
using ErrorEventBus = EventBus<ErrorEvent>;
using SettingsEventBus = EventBus<AppSettings>;
using StateEventBus = EventBus<StateEvent>;
using KeyboardEventBus = EventBus<KeyboardEvent>;
