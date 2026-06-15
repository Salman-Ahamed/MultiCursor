#include "Direct2DRenderer.h"
#include "Core/Logger.h"
#include <cmath>

Direct2DRenderer::Direct2DRenderer() = default;
Direct2DRenderer::~Direct2DRenderer() { Shutdown(); }

bool Direct2DRenderer::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
    m_isWarp = false;

    RECT rc;
    GetClientRect(hwnd, &rc);
    m_width = rc.right - rc.left;
    m_height = rc.bottom - rc.top;

    if (m_width <= 0 || m_height <= 0) {
        m_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        m_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    }

    if (!CreateDeviceChain()) {
        LOG_WARN(L"DComp initialization failed, trying WARP fallback");
        return InitializeWarp(hwnd);
    }

    if (!CreateSurface()) return false;
    if (!CreateVisualTree()) return false;

    LOG_INFO(L"Direct2DRenderer initialized (GPU), size: %dx%d", m_width, m_height);
    return true;
}

bool Direct2DRenderer::InitializeWarp(HWND hwnd) {
    m_hwnd = hwnd;
    m_isWarp = true;

    RECT rc;
    GetClientRect(hwnd, &rc);
    m_width = rc.right - rc.left;
    m_height = rc.bottom - rc.top;

    if (m_width <= 0 || m_height <= 0) {
        m_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        m_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    }

    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION, &m_d3dDevice, nullptr, nullptr);

    if (FAILED(hr)) {
        LOG_ERROR(L"WARP D3D11 device creation failed: 0x%08X", hr);
        return false;
    }

    hr = m_d3dDevice.As(&m_dxgiDevice);
    if (FAILED(hr)) { LOG_ERROR(L"WARP IDXGIDevice query failed: 0x%08X", hr); return false; }

    hr = D2D1CreateDevice(m_dxgiDevice.Get(), nullptr, &m_d2dDevice);
    if (FAILED(hr)) { LOG_ERROR(L"WARP D2D device creation failed: 0x%08X", hr); return false; }

    hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);
    if (FAILED(hr)) { LOG_ERROR(L"WARP D2D context creation failed: 0x%08X", hr); return false; }

    hr = DCompositionCreateDevice3(m_dxgiDevice.Get(),
                                    IID_PPV_ARGS(m_dcompDevice.GetAddressOf()));
    if (FAILED(hr)) { LOG_ERROR(L"WARP DComp device creation failed: 0x%08X", hr); return false; }

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                              __uuidof(IDWriteFactory),
                              reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));
    if (FAILED(hr)) { LOG_ERROR(L"DWrite factory creation failed: 0x%08X", hr); return false; }

    if (!CreateSurface()) return false;
    if (!CreateVisualTree()) return false;

    LOG_INFO(L"Direct2DRenderer initialized (WARP), size: %dx%d", m_width, m_height);
    return true;
}

bool Direct2DRenderer::CreateDeviceChain() {
    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION, &m_d3dDevice, nullptr, nullptr);

    if (FAILED(hr)) return false;

    hr = m_d3dDevice.As(&m_dxgiDevice);
    if (FAILED(hr)) return false;

    hr = D2D1CreateDevice(m_dxgiDevice.Get(), nullptr, &m_d2dDevice);
    if (FAILED(hr)) return false;

    hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);
    if (FAILED(hr)) return false;

    hr = DCompositionCreateDevice3(m_dxgiDevice.Get(),
                                    IID_PPV_ARGS(m_dcompDevice.GetAddressOf()));
    if (FAILED(hr)) return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                              __uuidof(IDWriteFactory),
                              reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));
    if (FAILED(hr)) return false;

    return true;
}

bool Direct2DRenderer::CreateSurface() {
    HRESULT hr = m_dcompDevice->CreateSurface(
        m_width, m_height,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_ALPHA_MODE_PREMULTIPLIED,
        &m_surface);

    if (FAILED(hr)) {
        LOG_ERROR(L"CreateSurface failed: 0x%08X (size: %dx%d)", hr, m_width, m_height);
        return false;
    }

    return true;
}

bool Direct2DRenderer::CreateVisualTree() {
    HRESULT hr = m_dcompDevice.As(&m_dcompDesktop);
    if (FAILED(hr)) { LOG_ERROR(L"QI for IDCompositionDesktopDevice failed: 0x%08X", hr); return false; }

    hr = m_dcompDesktop->CreateTargetForHwnd(m_hwnd, true, &m_dcompTarget);
    if (FAILED(hr)) { LOG_ERROR(L"CreateTargetForHwnd failed: 0x%08X", hr); return false; }

    hr = m_dcompDesktop->CreateVisual(&m_visual);
    if (FAILED(hr)) { LOG_ERROR(L"CreateVisual failed: 0x%08X", hr); return false; }

    hr = m_visual->SetContent(m_surface.Get());
    if (FAILED(hr)) { LOG_ERROR(L"SetContent failed: 0x%08X", hr); return false; }

    hr = m_dcompTarget->SetRoot(m_visual.Get());
    if (FAILED(hr)) { LOG_ERROR(L"SetRoot failed: 0x%08X", hr); return false; }

    hr = m_dcompDevice->Commit();
    if (FAILED(hr)) { LOG_ERROR(L"Commit failed: 0x%08X", hr); return false; }

    return true;
}

bool Direct2DRenderer::BeginDraw(const RECT& rect, POINT& offset) {
    if (m_inDraw) return false;

    HRESULT hr = m_surface->BeginDraw(
        &rect,
        IID_PPV_ARGS(m_d2dContext.GetAddressOf()),
        &offset);

    if (FAILED(hr)) {
        LOG_ERROR(L"BeginDraw failed: 0x%08X", hr);
        return false;
    }

    m_inDraw = true;
    m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(
        (FLOAT)offset.x, (FLOAT)offset.y));

    return true;
}

void Direct2DRenderer::EndDraw() {
    if (!m_inDraw) return;
    m_d2dContext->EndDraw();
    m_surface->EndDraw();
    m_inDraw = false;
}

void Direct2DRenderer::Commit() {
    m_dcompDevice->Commit();
}

void Direct2DRenderer::Shutdown() {
    ReleaseResources();
}

void Direct2DRenderer::ReleaseResources() {
    m_surface.Reset();
    m_visual.Reset();
    m_dcompTarget.Reset();
    m_dcompDevice.Reset();
    m_d2dContext.Reset();
    m_d2dDevice.Reset();
    m_dxgiDevice.Reset();
    m_d3dDevice.Reset();
}

bool Direct2DRenderer::Recreate() {
    LOG_INFO(L"Recreating renderer device chain");
    ReleaseResources();

    if (m_isWarp) {
        return InitializeWarp(m_hwnd);
    }

    if (!CreateDeviceChain()) {
        LOG_WARN(L"GPU recreate failed, trying WARP");
        return InitializeWarp(m_hwnd);
    }

    if (!CreateSurface()) return false;
    if (!CreateVisualTree()) return false;

    return true;
}
