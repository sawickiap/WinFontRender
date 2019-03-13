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
    CComPtr<IDXGIFactory> m_DxgiFactory;
    HWND m_Wnd = NULL;
};

static std::unique_ptr<CApp> g_App;

CApp::CApp()
{
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&m_DxgiFactory);
    assert(SUCCEEDED(hr));

    // TODO init device
}

CApp::~CApp()
{
}

void CApp::Init(HWND wnd)
{
    m_Wnd = wnd;
}

LRESULT CApp::WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_SYSCOMMAND:
        if((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break; 
    }

    return DefWindowProc(wnd, msg, wParam, lParam);
}

void CApp::Exit()
{
    DestroyWindow(m_Wnd);
}

void CApp::Frame()
{
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

    default:
        if(g_App)
        {
            return g_App->WndProc(wnd, msg, wParam, lParam);
        }
        return DefWindowProc(wnd, msg, wParam, lParam);
    }
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
    ivec2 size = ivec2(CW_USEDEFAULT, CW_USEDEFAULT);

    HWND wnd = CreateWindowEx(
        wndExStyle,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        wndStyle,
        pos.x, pos.y,
        size.x, size.y,
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
