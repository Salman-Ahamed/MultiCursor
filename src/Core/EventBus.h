#pragma once

#include "Types.h"
#include <functional>
#include <vector>
#include <algorithm>

template<typename T>
class EventBus {
public:
    using Token = size_t;
    using Callback = std::function<void(const T&)>;

    Token Subscribe(Callback cb) {
        m_callbacks.push_back({ ++m_nextToken, std::move(cb) });
        return m_nextToken;
    }

    void Unsubscribe(Token token) {
        auto it = std::remove_if(m_callbacks.begin(), m_callbacks.end(),
            [token](const Entry& e) { return e.token == token; });
        m_callbacks.erase(it, m_callbacks.end());
    }

    void Publish(const T& event) {
        for (auto& entry : m_callbacks) {
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
};

using DeviceEventBus = EventBus<DeviceEvent>;
using ErrorEventBus = EventBus<ErrorEvent>;
