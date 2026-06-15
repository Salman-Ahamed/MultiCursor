#pragma once

#include "Types.h"
#include "EventBus.h"
#include <functional>

class AppStateMachine {
public:
    using StateCallback = std::function<void(AppState, AppState)>;
    using Token = size_t;

    AppState Current() const { return m_state; }

    bool TransitionTo(AppState newState) {
        if (!IsValid(m_state, newState)) return false;
        AppState old = m_state;
        m_state = newState;
        for (auto& cb : m_callbacks) cb.cb(old, newState);
        return true;
    }

    Token OnTransition(StateCallback cb) {
        m_callbacks.push_back({ ++m_nextToken, std::move(cb) });
        return m_nextToken;
    }

    void Unsubscribe(Token token) {
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
};
