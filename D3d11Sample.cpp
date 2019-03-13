#include "WinFontRender.h"

#define STRICT
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>

#include <atlbase.h> // for CComPtr

#include <vector>
#include <string>
#include <algorithm>
#include <exception>
#include <memory>

#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <cassert>

#include <dxgi1_6.h>
#include <d3d11.h>

const wchar_t* const WINDOW_CLASS_NAME = L"WIN_FONT_RENDER_SAMPLE_D3D11";
const wchar_t* const WINDOW_TITLE = L"WinFontRender Direct3D 11 Sample";
const uvec2 DISPLAY_SIZE = uvec2(1280, 720);

HINSTANCE g_Instance;

class CCoInitializeGuard
{
public:
    CCoInitializeGuard() { CoInitialize(nullptr); }
    ~CCoInitializeGuard() { CoUninitialize(); }
};

class CApp
{
public:
    CApp();
    ~CApp();
    void Init(HWND wnd);
    LRESULT WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Exit();
    void Frame();

private:
    HWND m_Wnd = NULL;
    CComPtr<IDXGIFactory> m_DxgiFactory;
    CComPtr<ID3D11Device> m_Dev;
    CComPtr<ID3D11DeviceContext> m_Ctx;
    CComPtr<IDXGISwapChain> m_SwapChain;
    CComPtr<ID3D11Texture2D> m_SwapChainTexture;
    CComPtr<ID3D11RenderTargetView> m_SwapChainRTV;
};

static std::unique_ptr<CApp> g_App;

CApp::CApp()
{
}

CApp::~CApp()
{
}

void CApp::Init(HWND wnd)
{
    m_Wnd = wnd;

    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&m_DxgiFactory);
    assert(SUCCEEDED(hr));

    hr = D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr, // Software
        D3D11_CREATE_DEVICE_SINGLETHREADED,
        NULL,
        0, // FeatureLevels
        D3D11_SDK_VERSION,
        &m_Dev,
        nullptr, // pFeatureLevel
        &m_Ctx);
    assert(SUCCEEDED(hr));

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width  = DISPLAY_SIZE.x;
    swapChainDesc.BufferDesc.Height = DISPLAY_SIZE.y;
    swapChainDesc.BufferDesc.RefreshRate = {0, 0};
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 3;
    swapChainDesc.OutputWindow = wnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = 0;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    hr = m_DxgiFactory->CreateSwapChain(m_Dev.p, &swapChainDesc, &m_SwapChain);
    assert(SUCCEEDED(hr));

    hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_SwapChainTexture);
    assert(SUCCEEDED(hr));
    hr = m_Dev->CreateRenderTargetView(m_SwapChainTexture, NULL, &m_SwapChainRTV);
    assert(SUCCEEDED(hr));
}

LRESULT CApp::WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_KEYDOWN:
        if(wParam == VK_ESCAPE)
            Exit();
        return 0;
    }

    return DefWindowProc(wnd, msg, wParam, lParam);
}

void CApp::Exit()
{
    DestroyWindow(m_Wnd);
}

void CApp::Frame()
{
    vec4 clearColor = vec4(0.f, 0.f, 0.4f, 1.f);
    m_Ctx->ClearRenderTargetView(m_SwapChainRTV, clearColor);

    m_SwapChain->Present(1, 0);
}

LRESULT WINAPI WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_CREATE:
        g_App.reset(new CApp());
        g_App->Init(wnd);
        return 0;

    case WM_DESTROY:
        if(g_App)
        {
            g_App.reset();
            PostQuitMessage(0);
        }
        return 0;

    case WM_SYSCOMMAND:
        // Disable ALT application menu.
        if((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break; 

    default:
        if(g_App)
        {
            return g_App->WndProc(wnd, msg, wParam, lParam);
        }
    }

    return DefWindowProc(wnd, msg, wParam, lParam);
}

static void Main2()
{
    g_Instance = (HINSTANCE)GetModuleHandle(NULL);

    CCoInitializeGuard coInitializeObj;

    WNDCLASSEX wndClassDesc = { sizeof(WNDCLASSEX) };
    wndClassDesc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wndClassDesc.hbrBackground = NULL;
    wndClassDesc.hCursor = LoadCursor(NULL, IDC_CROSS);
    wndClassDesc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClassDesc.hInstance = g_Instance;
    wndClassDesc.lpfnWndProc = &WndProc;
    wndClassDesc.lpszClassName = WINDOW_CLASS_NAME;

    ATOM wndClass = RegisterClassEx(&wndClassDesc);
    assert(wndClass);

    const DWORD wndStyle = WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    const DWORD wndExStyle = 0;

    ivec2 pos = ivec2(CW_USEDEFAULT, CW_USEDEFAULT);

    HWND wnd = CreateWindowEx(
        wndExStyle,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        wndStyle,
        pos.x, pos.y,
        (int)DISPLAY_SIZE.x, (int)DISPLAY_SIZE.y,
        NULL,
        NULL,
        g_Instance,
        0);
    assert(wnd);
    assert(g_App);

    MSG msg;
    bool quit = false;
    while(!quit)
    {
        uint32_t messageIndex = 0;
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                quit = true;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);

            ++messageIndex;
        }

        if(g_App)
        {
            g_App->Frame();
        }
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    try
    {
        Main2();
    }
    catch(const std::exception& err)
    {
        fprintf(stderr, "ERROR: %s\n", err.what());
        return -1;
    }
    catch(...)
    {
        fprintf(stderr, "UNKNOWN ERROR\n");
        return -1;
    }

    return 0;
}
