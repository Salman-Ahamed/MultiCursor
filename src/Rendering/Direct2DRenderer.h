#pragma once

#include "Core/Types.h"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Direct2DRenderer {
public:
    Direct2DRenderer();
    ~Direct2DRenderer();

    bool Initialize(HWND hwnd);
    bool InitializeWarp(HWND hwnd);
    void Shutdown();
    bool Recreate();

    IDCompositionDevice3* DCompDevice() const { return m_dcompDevice.Get(); }
    IDCompositionDesktopDevice* DCompDesktop() const { return m_dcompDesktop.Get(); }
    ID2D1DeviceContext* D2DContext() const { return m_d2dContext.Get(); }
    IDWriteFactory* DWriteFactory() const { return m_dwriteFactory.Get(); }
    IDCompositionSurface* Surface() const { return m_surface.Get(); }
    IDCompositionVisual2* Visual() const { return m_visual.Get(); }

    bool BeginDraw(const RECT& rect, POINT& offset);
    void EndDraw();
    void Commit();

    bool IsValid() const { return m_dcompDevice != nullptr; }
    bool IsWarp() const { return m_isWarp; }

    int SurfaceWidth() const { return m_width; }
    int SurfaceHeight() const { return m_height; }

private:
    bool CreateDeviceChain();
    bool CreateSurface();
    bool CreateVisualTree();
    void ReleaseResources();

    HWND m_hwnd = nullptr;
    int m_width = 0;
    int m_height = 0;
    bool m_isWarp = false;

    ComPtr<ID3D11Device> m_d3dDevice;
    ComPtr<IDXGIDevice> m_dxgiDevice;
    ComPtr<ID2D1Device> m_d2dDevice;
    ComPtr<ID2D1DeviceContext> m_d2dContext;
    ComPtr<IDCompositionDevice3> m_dcompDevice;
    ComPtr<IDCompositionDesktopDevice> m_dcompDesktop;
    ComPtr<IDCompositionTarget> m_dcompTarget;
    ComPtr<IDCompositionVisual2> m_visual;
    ComPtr<IDCompositionSurface> m_surface;
    ComPtr<IDWriteFactory> m_dwriteFactory;

    bool m_inDraw = false;
};
