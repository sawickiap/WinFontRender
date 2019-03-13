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
typedef uint16_t INDEX_TYPE;
const DXGI_FORMAT INDEX_BUFFER_FORMAT = DXGI_FORMAT_R16_UINT;

HINSTANCE g_Instance;

class CCoInitializeGuard
{
public:
    CCoInitializeGuard() { CoInitialize(nullptr); }
    ~CCoInitializeGuard() { CoUninitialize(); }
};

void GetExeBinaryResource(void*& outPtr, uint32_t& outSize, const wstr_view& name, const wstr_view& type)
{
    HRSRC rsrc;
    HGLOBAL global = NULL;
    if((rsrc = FindResource(NULL, name.c_str(), type.c_str())) == NULL ||
        (global = LoadResource(NULL, rsrc)) == NULL)
    {
        assert(0 && "Failed to load EXE resource.");
    }

    outSize = SizeofResource(NULL, rsrc);
    outPtr = LockResource(global);
}

struct SVertex
{
    vec2 Pos;
    vec2 TexCoord;
    uint32_t Color;
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
    CComPtr<ID3D11RasterizerState> m_RasterizerState;
    CComPtr<ID3D11DepthStencilState> m_DepthStencilState;
    CComPtr<ID3D11SamplerState> m_SamplerState;
    CComPtr<ID3D11BlendState> m_BlendState;
    CComPtr<ID3D11InputLayout> m_InputLayout;
    CComPtr<ID3D11VertexShader> m_MainVs;
    CComPtr<ID3D11PixelShader> m_MainPs;
    CComPtr<ID3D11Texture2D> m_Texture;
    CComPtr<ID3D11ShaderResourceView> m_TextureSRV;
    CComPtr<ID3D11Buffer> m_VertexBuffer;
    CComPtr<ID3D11Buffer> m_IndexBuffer;

    void InitTexture();
    void InitVertexBuffer();
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

    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;
    hr = m_Dev->CreateRasterizerState(&rasterizerDesc, &m_RasterizerState);
    assert(SUCCEEDED(hr));

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    hr = m_Dev->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilState);
    assert(SUCCEEDED(hr));

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MinLOD = -FLT_MAX;
    samplerDesc.MaxLOD =  FLT_MAX;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    hr = m_Dev->CreateSamplerState(&samplerDesc, &m_SamplerState);
    assert(SUCCEEDED(hr));

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    hr = m_Dev->CreateBlendState(&blendDesc, &m_BlendState);
    assert(SUCCEEDED(hr));

    void* shaderCode;
    uint32_t shaderCodeSize;
    GetExeBinaryResource(shaderCode, shaderCodeSize, L"IDR_SHADER_MAIN_VS", L"Binary");
    hr = m_Dev->CreateVertexShader(shaderCode, shaderCodeSize, nullptr, &m_MainVs);
    assert(SUCCEEDED(hr));
    m_Ctx->VSSetShader(m_MainVs, nullptr, 0);

    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "Pos",      0, DXGI_FORMAT_R32G32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "Color",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = m_Dev->CreateInputLayout(inputElementDesc, _countof(inputElementDesc), shaderCode, shaderCodeSize, &m_InputLayout);
    assert(SUCCEEDED(hr));

    GetExeBinaryResource(shaderCode, shaderCodeSize, L"IDR_SHADER_MAIN_PS", L"Binary");
    hr = m_Dev->CreatePixelShader(shaderCode, shaderCodeSize, nullptr, &m_MainPs);
    assert(SUCCEEDED(hr));
    m_Ctx->PSSetShader(m_MainPs, nullptr, 0);

    D3D11_VIEWPORT viewport = {};
    viewport.Width  = (float)DISPLAY_SIZE.x;
    viewport.Height = (float)DISPLAY_SIZE.y;
    viewport.MaxDepth = 1.f;
    m_Ctx->RSSetViewports(1, &viewport);

    m_Ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_Ctx->IASetInputLayout(m_InputLayout);
    m_Ctx->VSSetShader(m_MainVs, nullptr, 0);
    m_Ctx->RSSetState(m_RasterizerState);
    m_Ctx->PSSetSamplers(0, 1, &m_SamplerState.p);
    m_Ctx->OMSetDepthStencilState(m_DepthStencilState, 0);
    m_Ctx->OMSetBlendState(m_BlendState.p, nullptr, 0xffffffff);

    InitTexture();
    InitVertexBuffer();
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

    ID3D11RenderTargetView* rtv = m_SwapChainRTV.p;
    m_Ctx->OMSetRenderTargets(1, &rtv, nullptr);
    
    m_Ctx->DrawIndexed(5, 0, 0);
    
    rtv = nullptr;
    m_Ctx->OMSetRenderTargets(1, &rtv, nullptr);

    m_SwapChain->Present(1, 0);
}

void CApp::InitTexture()
{
    uvec2 textureSize = uvec2(4, 4);
    uint8_t textureData[32*32] = {
        255, 0, 255, 0,
        0, 255, 0, 255,
        255, 0, 255, 0,
        0, 255, 0, 255,
    };
    uint32_t textureRowPitch = 4;

    CD3D11_TEXTURE2D_DESC textureDesc = CD3D11_TEXTURE2D_DESC(
        DXGI_FORMAT_A8_UNORM, textureSize.x, textureSize.y, 1, 1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);
    D3D11_SUBRESOURCE_DATA initialData = {textureData, textureRowPitch, 0};
    HRESULT hr = m_Dev->CreateTexture2D(&textureDesc, &initialData, &m_Texture);
    assert(SUCCEEDED(hr));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = { textureDesc.Format, D3D_SRV_DIMENSION_TEXTURE2D };
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    hr = m_Dev->CreateShaderResourceView(m_Texture, &srvDesc, &m_TextureSRV);
    assert(SUCCEEDED(hr));

    m_Ctx->PSSetShaderResources(0, 1, &m_TextureSRV.p);
}

void CApp::InitVertexBuffer()
{
    const uint32_t vertexCount = 4;
    const SVertex vertices[] = {
        { vec2(0.f, 0.f), vec2(0.f, 0.f), 0xFF0000FF },
        { vec2(1.f, 0.f), vec2(1.f, 0.f), 0xFF00FF00 },
        { vec2(0.f, 1.f), vec2(0.f, 1.f), 0xFFFF0000 },
        { vec2(1.f, 1.f), vec2(1.f, 1.f), 0xFFFFFFFF },
    };
    const uint32_t indexCount = 5;
    const INDEX_TYPE indices[] = {0, 1, 2, 3, 0xFFFF};

    CD3D11_BUFFER_DESC vbDesc = CD3D11_BUFFER_DESC(
        vertexCount * sizeof(SVertex), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE);
    D3D11_SUBRESOURCE_DATA vbInitialData = {vertices};
    HRESULT hr = m_Dev->CreateBuffer(&vbDesc, &vbInitialData, &m_VertexBuffer);
    assert(SUCCEEDED(hr));

    const UINT vertexStride = sizeof(SVertex);
    const UINT offset = 0;
    m_Ctx->IASetVertexBuffers(0, 1, &m_VertexBuffer.p, &vertexStride, &offset);

    CD3D11_BUFFER_DESC ibDesc = CD3D11_BUFFER_DESC(
        indexCount * sizeof(INDEX_TYPE), D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE);
    D3D11_SUBRESOURCE_DATA ibInitialData = {indices};
    hr = m_Dev->CreateBuffer(&ibDesc, &ibInitialData, &m_IndexBuffer);
    assert(SUCCEEDED(hr));

    m_Ctx->IASetIndexBuffer(m_IndexBuffer.p, INDEX_BUFFER_FORMAT, 0);
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
