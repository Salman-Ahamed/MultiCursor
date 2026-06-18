#pragma once

#include "Types.h"
#include "EventBus.h"
#include <functional>
#include <mutex>
#include <vector>

class AppStateMachine {
public:
    using StateCallback = std::function<void(AppState, AppState)>;
    using Token = size_t;

    AppState Current() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state;
    }

    bool TransitionTo(AppState newState) {
        std::vector<Entry> cbs;
        AppState old;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!IsValid(m_state, newState)) return false;
            old = m_state;
            m_state = newState;
            cbs = m_callbacks;
        }
        for (auto& cb : cbs) cb.cb(old, newState);
        return true;
    }

    Token OnTransition(StateCallback cb) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks.push_back({ ++m_nextToken, std::move(cb) });
        return m_nextToken;
    }

    void Unsubscribe(Token token) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::remove_if(m_callbacks.begin(), m_callbacks.end(),
            [token](const Entry& e) { return e.token == token; });
        m_callbacks.erase(it, m_callbacks.end());
    }

    static bool IsValid(AppState from, AppState to);

private:
    struct Entry {
        Token token;
        StateCallback cb;
    };
    AppState m_state = AppState::Starting;
    std::vector<Entry> m_callbacks;
    Token m_nextToken = 0;
    mutable std::mutex m_mutex;
};
