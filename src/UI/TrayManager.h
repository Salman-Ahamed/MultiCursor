#pragma once

#include <windows.h>
#include <shellapi.h>
#include <functional>

class TrayManager {
public:
    using CtxMenuCallback = std::function<void(UINT cmd)>;

    TrayManager() = default;
    ~TrayManager();

    bool Initialize(HINSTANCE hInstance, HWND targetWnd, UINT iconResId = 101);
    void Remove();

    bool ShowBalloon(const wchar_t* title, const wchar_t* text, DWORD infoFlags = NIIF_INFO, UINT timeoutMs = 5000);

    void SetContextCallback(CtxMenuCallback cb) { m_ctxCb = std::move(cb); }
    HWND TargetWindow() const { return m_targetWnd; }

private:
    NOTIFYICONDATAW m_nid = {};
    HWND m_targetWnd = nullptr;
    bool m_visible = false;
    CtxMenuCallback m_ctxCb;
};
