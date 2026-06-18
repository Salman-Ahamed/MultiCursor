#include "TrayManager.h"
#include "Core/Logger.h"

TrayManager::~TrayManager() {
    Remove();
}

bool TrayManager::Initialize(HINSTANCE hInstance, HWND targetWnd, UINT iconResId) {
    m_targetWnd = targetWnd;
    if (!m_targetWnd) return false;

    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_targetWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    m_nid.uCallbackMessage = WM_APP + 1;
    m_nid.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(iconResId));
    wcscpy_s(m_nid.szTip, L"MultiCursor");

    if (!Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        LOG_WARN(L"Failed to add tray icon: %d", GetLastError());
        return false;
    }

    m_visible = true;
    LOG_INFO(L"Tray icon initialized");
    return true;
}

void TrayManager::Remove() {
    if (m_visible) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_visible = false;
    }
}

bool TrayManager::ShowBalloon(const wchar_t* title, const wchar_t* text, DWORD infoFlags, UINT timeoutMs) {
    if (!m_visible) return false;

    m_nid.uFlags |= NIF_INFO;
    m_nid.uTimeout = timeoutMs;
    m_nid.dwInfoFlags = infoFlags;
    wcscpy_s(m_nid.szInfoTitle, title);
    wcscpy_s(m_nid.szInfo, text);

    bool ok = Shell_NotifyIconW(NIM_MODIFY, &m_nid) != FALSE;
    if (!ok) {
        LOG_WARN(L"Failed to show tray balloon: %d", GetLastError());
    }
    return ok;
}
