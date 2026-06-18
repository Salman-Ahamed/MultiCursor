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

    if (!CreateDCompDevice()) return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                              __uuidof(IDWriteFactory),
                              reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));
    if (FAILED(hr)) { LOG_ERROR(L"WARP DWrite factory creation failed: 0x%08X", hr); return false; }

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

    if (!CreateDCompDevice()) return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                              __uuidof(IDWriteFactory),
                              reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf()));
    if (FAILED(hr)) return false;

    return true;
}

bool Direct2DRenderer::CreateDCompDevice() {
    if (!m_dcompLib) {
        m_dcompLib = LoadLibraryExW(L"dcomp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!m_dcompLib) {
            LOG_ERROR(L"Failed to load dcomp.dll");
            return false;
        }
    }

    // Try DCompositionCreateDevice2 (Win8.1+) first
    using PFN_DCompCreateDevice2 = HRESULT(WINAPI*)(IUnknown*, REFIID, void**);
    auto fnCreate2 = (PFN_DCompCreateDevice2)
        GetProcAddress(m_dcompLib, "DCompositionCreateDevice2");
    if (fnCreate2) {
        HRESULT hr = fnCreate2(m_dxgiDevice.Get(),
                               __uuidof(IDCompositionDesktopDevice),
                               (void**)m_dcompDesktop.GetAddressOf());
        if (SUCCEEDED(hr)) {
            LOG_INFO(L"DComp: IDCompositionDesktopDevice (via CreateDevice2)");
            return true;
        }
        LOG_WARN(L"DCompositionCreateDevice2 failed: 0x%08X", hr);
    }

    // Try DCompositionCreateDevice3 (Win10+)
    using PFN_DCompCreateDevice3 = HRESULT(WINAPI*)(IUnknown*, REFIID, void**);
    auto fnCreate3 = (PFN_DCompCreateDevice3)
        GetProcAddress(m_dcompLib, "DCompositionCreateDevice3");
    if (fnCreate3) {
        HRESULT hr = fnCreate3(m_dxgiDevice.Get(),
                               __uuidof(IDCompositionDesktopDevice),
                               (void**)m_dcompDesktop.GetAddressOf());
        if (SUCCEEDED(hr)) {
            LOG_INFO(L"DComp: IDCompositionDesktopDevice (via CreateDevice3)");
            return true;
        }
        LOG_WARN(L"DCompositionCreateDevice3 failed: 0x%08X", hr);
    }

    // Try DCompositionCreateDevice (Win8+)
    using PFN_DCompCreateDevice = HRESULT(WINAPI*)(IDXGIDevice*, REFIID, void**);
    auto fnCreate = (PFN_DCompCreateDevice)
        GetProcAddress(m_dcompLib, "DCompositionCreateDevice");
    if (fnCreate) {
        HRESULT hr = fnCreate(m_dxgiDevice.Get(),
                              __uuidof(IDCompositionDesktopDevice),
                              (void**)m_dcompDesktop.GetAddressOf());
        if (SUCCEEDED(hr)) {
            LOG_INFO(L"DComp: IDCompositionDesktopDevice (via CreateDevice)");
            return true;
        }
        LOG_WARN(L"DCompositionCreateDevice failed: 0x%08X", hr);
    }

    LOG_ERROR(L"No working DComp creation function found on this system");
    return false;
}

bool Direct2DRenderer::CreateSurface() {
    HRESULT hr = m_dcompDesktop->CreateSurface(
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

bool Direct2DRenderer::CreateSwapChain() {
    LOG_INFO(L"Creating swap chain for composition");

    ComPtr<IDXGIDevice2> dxgiDevice2;
    HRESULT hr = m_dxgiDevice.As(&dxgiDevice2);
    if (FAILED(hr)) {
        LOG_ERROR(L"IDXGIDevice2 query failed: 0x%08X", hr);
        return false;
    }

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice2->GetAdapter(&adapter);
    if (FAILED(hr)) { LOG_ERROR(L"GetAdapter failed: 0x%08X", hr); return false; }

    ComPtr<IDXGIFactory2> factory;
    hr = adapter->GetParent(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) { LOG_ERROR(L"GetParent(IDXGIFactory2) failed: 0x%08X", hr); return false; }

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = (UINT)m_width;
    desc.Height = (UINT)m_height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    hr = factory->CreateSwapChainForComposition(m_d3dDevice.Get(), &desc, nullptr, &m_swapChain);
    if (FAILED(hr)) { LOG_ERROR(L"CreateSwapChainForComposition failed: 0x%08X", hr); return false; }

    // Create D2D bitmap from the swap chain back buffer
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) { LOG_ERROR(L"GetBuffer failed: 0x%08X", hr); return false; }

    ComPtr<IDXGISurface> dxgiSurface;
    hr = backBuffer.As(&dxgiSurface);
    if (FAILED(hr)) { LOG_ERROR(L"IDXGISurface query from back buffer failed: 0x%08X", hr); return false; }

    D2D1_BITMAP_PROPERTIES1 bp = {};
    bp.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

    hr = m_d2dContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &bp, &m_d2dTargetBitmap);
    if (FAILED(hr)) { LOG_ERROR(L"CreateBitmapFromDxgiSurface failed: 0x%08X", hr); return false; }

    m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());

    // Update visual to use swap chain instead of surface
    hr = m_visual->SetContent(m_swapChain.Get());
    if (FAILED(hr)) { LOG_ERROR(L"SetContent(swapChain) failed: 0x%08X", hr); return false; }

    LOG_INFO(L"Swap chain created successfully (%dx%d)", m_width, m_height);
    return true;
}

bool Direct2DRenderer::CreateVisualTree() {
    HRESULT hr = m_dcompDesktop->CreateTargetForHwnd(m_hwnd, true, &m_dcompTarget);
    if (FAILED(hr)) { LOG_ERROR(L"CreateTargetForHwnd failed: 0x%08X", hr); return false; }

    hr = m_dcompDesktop->CreateVisual(&m_visual);
    if (FAILED(hr)) { LOG_ERROR(L"CreateVisual failed: 0x%08X", hr); return false; }

    hr = m_visual->SetContent(m_surface.Get());
    if (FAILED(hr)) { LOG_ERROR(L"SetContent failed: 0x%08X", hr); return false; }

    hr = m_dcompTarget->SetRoot(m_visual.Get());
    if (FAILED(hr)) { LOG_ERROR(L"SetRoot failed: 0x%08X", hr); return false; }

    hr = m_dcompDesktop->Commit();
    if (FAILED(hr)) { LOG_ERROR(L"Commit failed: 0x%08X", hr); return false; }

    return true;
}

bool Direct2DRenderer::BeginDraw(const RECT& rect, POINT& offset) {
    if (m_inDraw) return false;

    if (m_useSwapChain) {
        m_d2dContext->BeginDraw();
        m_inDraw = true;
        return true;
    }

    if (!m_surface) return false;

    // Use temporary pointer for the probe call so we don't lose our D2D context on failure
    ID2D1DeviceContext* tempCtx = nullptr;
    HRESULT hr = m_surface->BeginDraw(
        &rect,
        __uuidof(ID2D1DeviceContext),
        (void**)&tempCtx,
        &offset);

    if (hr == E_NOINTERFACE) {
        LOG_WARN(L"DComp surface BeginDraw rejected D2D IIDs, trying swap chain fallback");
        if (CreateSwapChain()) {
            m_useSwapChain = true;
            m_surface.Reset();
            m_dcompDesktop->Commit();
            LOG_INFO(L"Swap chain fallback successful");
            m_inDraw = true;
            return true;
        }
    }

    if (FAILED(hr)) {
        LOG_ERROR(L"BeginDraw failed: 0x%08X", hr);
        return false;
    }

    // Successful BeginDraw - take ownership of the context
    m_d2dContext.Attach(tempCtx);
    m_inDraw = true;
    m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(
        (FLOAT)offset.x, (FLOAT)offset.y));

    return true;
}

void Direct2DRenderer::EndDraw() {
    if (!m_inDraw) return;
    m_d2dContext->EndDraw();
    if (m_useSwapChain) {
        m_swapChain->Present(1, 0);
    } else {
        m_surface->EndDraw();
    }
    m_inDraw = false;
}

void Direct2DRenderer::Commit() {
    m_dcompDesktop->Commit();
}

bool Direct2DRenderer::ResizeSurface(int width, int height) {
    if (!m_dcompDesktop) return false;
    m_width = width;
    m_height = height;

    if (m_useSwapChain) {
        m_d2dTargetBitmap.Reset();
        m_d2dContext->SetTarget(nullptr);
        HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr)) { LOG_ERROR(L"ResizeBuffers failed: 0x%08X", hr); return false; }

        ComPtr<ID3D11Texture2D> backBuffer;
        hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (FAILED(hr)) { LOG_ERROR(L"GetBuffer after resize failed: 0x%08X", hr); return false; }

        ComPtr<IDXGISurface> dxgiSurface;
        hr = backBuffer.As(&dxgiSurface);
        if (FAILED(hr)) { LOG_ERROR(L"IDXGISurface query after resize failed: 0x%08X", hr); return false; }

        D2D1_BITMAP_PROPERTIES1 bp = {};
        bp.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        hr = m_d2dContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &bp, &m_d2dTargetBitmap);
        if (FAILED(hr)) { LOG_ERROR(L"CreateBitmapFromDxgiSurface after resize failed: 0x%08X", hr); return false; }

        m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());
        m_dcompDesktop->Commit();
        LOG_INFO(L"Swap chain resized to %dx%d", m_width, m_height);
        return true;
    }

    m_surface.Reset();
    if (!CreateSurface()) return false;
    HRESULT hr = m_visual->SetContent(m_surface.Get());
    if (FAILED(hr)) { LOG_ERROR(L"ResizeSurface SetContent failed: 0x%08X", hr); return false; }
    hr = m_dcompDesktop->Commit();
    if (FAILED(hr)) { LOG_ERROR(L"ResizeSurface Commit failed: 0x%08X", hr); return false; }
    LOG_INFO(L"Surface resized to %dx%d", m_width, m_height);
    return true;
}

void Direct2DRenderer::Shutdown() {
    ReleaseResources();
}

void Direct2DRenderer::ReleaseResources() {
    m_d2dTargetBitmap.Reset();
    m_swapChain.Reset();
    m_surface.Reset();
    m_visual.Reset();
    m_dcompTarget.Reset();
    m_dcompDesktop.Reset();
    m_dcompDevice.Reset();
    m_d2dContext.Reset();
    m_d2dDevice.Reset();
    m_dxgiDevice.Reset();
    m_d3dDevice.Reset();
    m_dwriteFactory.Reset();
    m_useSwapChain = false;
    if (m_dcompLib) { FreeLibrary(m_dcompLib); m_dcompLib = nullptr; }
}

bool Direct2DRenderer::Recreate() {
    LOG_INFO(L"Recreating renderer device chain");
    m_inDraw = false;
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
