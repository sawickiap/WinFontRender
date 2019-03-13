// Defines for <Windows.h>
#define STRICT
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#define WIN_FONT_RENDER_IMPLEMENTATION
#include "WinFontRender.h"

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
const float MARGIN = 32.f;
const float TEXT_WIDTH = DISPLAY_SIZE.x - MARGIN * 2.f;

typedef uint16_t INDEX_TYPE;
constexpr DXGI_FORMAT INDEX_BUFFER_FORMAT = DXGI_FORMAT_R16_UINT;
constexpr uint32_t VB_FLAGS = VERTEX_BUFFER_FLAG_USE_INDEX_BUFFER_16BIT | VERTEX_BUFFER_FLAG_TRIANGLE_STRIP_WITH_RESTART_INDEX;

// Text generated using: https://pl.lipsum.com/
const wchar_t* const TEXT_TO_DISPLAY =
    L"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin purus ipsum, "
    L"ultricies sed ipsum sit amet, dignissim consequat risus. Pellentesque habitant "
    L"morbi tristique senectus et netus et malesuada fames ac turpis egestas. Aliquam "
    L"in rhoncus magna. Aliquam erat volutpat. Nunc dictum odio non erat consectetur "
    L"fermentum. Phasellus et justo ut purus imperdiet viverra. Curabitur a iaculis "
    L"quam, ac egestas odio. Morbi condimentum elit diam, nec viverra nibh eleifend ac. "
    L"Donec eu nibh ac massa ultrices imperdiet. Donec metus mauris, varius sed commodo "
    L"nec, cursus quis nibh. Sed bibendum vestibulum nulla eget tempor. Morbi vel ipsum "
    L"in ex scelerisque scelerisque. Curabitur varius tortor in magna sagittis, id "
    L"eleifend orci cursus. Vivamus accumsan euismod dolor, in aliquam lorem sollicitudin nec.\n"
    L"\n"
    L"Sed scelerisque urna eros, at varius sem luctus at. Suspendisse nec commodo est, "
    L"et tincidunt lectus. Nullam aliquam nunc vel dolor scelerisque, sed dignissim ipsum "
    L"rhoncus. Nunc gravida, tortor eu auctor fermentum, mauris massa porttitor quam, in "
    L"finibus mi metus vitae purus. Donec non dictum est. Quisque in ligula nec felis "
    L"suscipit efficitur. Cras eros mauris, varius semper tempus non, vestibulum sit amet "
    L"ante. Cras eget dolor dolor. Etiam vel urna bibendum, placerat lorem quis, efficitur "
    L"ante. Donec sed nibh a tortor porta sollicitudin volutpat ut metus. Orci varius "
    L"natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus.";

const wchar_t* const FONT_CREATE_FACE_NAME = L"Arial";
const int FONT_CREATE_SIZE = 30;
const uint32_t FONT_CREATE_FLAGS = SFontDesc::FLAG_BOLD;

const uint32_t FONT_DISPLAY_FLAGS = CFont::FLAG_WRAP_WORD | CFont::FLAG_HLEFT | CFont::FLAG_VTOP;
const float FONT_DISPLAY_SIZE = 30.f;

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
    std::unique_ptr<CFont> m_Font;
    CComPtr<ID3D11Texture2D> m_Texture;
    CComPtr<ID3D11ShaderResourceView> m_TextureSRV;
    CComPtr<ID3D11Buffer> m_VertexBuffer;
    CComPtr<ID3D11Buffer> m_IndexBuffer;
    size_t m_VertexCount = 0;
    size_t m_IndexCount = 0;

    void InitDxgiFactory();
    void InitDevice();
    void InitSwapChain();
    void InitStates();
    void InitShaders();
    void InitFont();
    void InitTexture();
    void InitVertexAndIndexBuffer();
    void SetOneTimeStates();
    static void PostprocessVertices(SVertex* vertices, size_t count);
};

static std::unique_ptr<CApp> g_App;

void CApp::Init(HWND wnd)
{
    m_Wnd = wnd;
    InitDxgiFactory();
    InitDevice();
    InitSwapChain();
    InitStates();
    InitShaders();
    InitFont();
    InitTexture();
    InitVertexAndIndexBuffer();
    SetOneTimeStates();
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
    assert(m_Wnd);
    DestroyWindow(m_Wnd);
}

void CApp::Frame()
{
    vec4 clearColor = vec4(0.f, 0.f, 0.333f, 1.f);
    m_Ctx->ClearRenderTargetView(m_SwapChainRTV, clearColor);

    ID3D11RenderTargetView* rtv = m_SwapChainRTV.p;
    m_Ctx->OMSetRenderTargets(1, &rtv, nullptr);
    
    m_Ctx->DrawIndexed((UINT)m_IndexCount, 0, 0);
    
    rtv = nullptr;
    m_Ctx->OMSetRenderTargets(1, &rtv, nullptr);

    m_SwapChain->Present(1, 0);
}

void CApp::InitDxgiFactory()
{
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&m_DxgiFactory);
    assert(SUCCEEDED(hr));
}

void CApp::InitDevice()
{
    HRESULT hr = D3D11CreateDevice(
        NULL, // pAdapter
        D3D_DRIVER_TYPE_HARDWARE, // DriverType
        nullptr, // Software
        D3D11_CREATE_DEVICE_SINGLETHREADED, // Flags
        NULL, // pFeatureLevels
        0, // FeatureLevels
        D3D11_SDK_VERSION, // SDKVersion
        &m_Dev, // ppDevice
        nullptr, // pFeatureLevel
        &m_Ctx); // ppImmediateContext
    assert(SUCCEEDED(hr));
}

void CApp::InitSwapChain()
{
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
    swapChainDesc.OutputWindow = m_Wnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = 0;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    HRESULT hr = m_DxgiFactory->CreateSwapChain(m_Dev.p, &swapChainDesc, &m_SwapChain);
    assert(SUCCEEDED(hr));

    hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_SwapChainTexture);
    assert(SUCCEEDED(hr));
    hr = m_Dev->CreateRenderTargetView(m_SwapChainTexture, NULL, &m_SwapChainRTV);
    assert(SUCCEEDED(hr));
}

void CApp::InitStates()
{
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = TRUE;
    HRESULT hr = m_Dev->CreateRasterizerState(&rasterizerDesc, &m_RasterizerState);
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
}

void CApp::InitShaders()
{
    void* shaderCode;
    uint32_t shaderCodeSize;
    GetExeBinaryResource(shaderCode, shaderCodeSize, L"IDR_SHADER_MAIN_VS", L"Binary");
    HRESULT hr = m_Dev->CreateVertexShader(shaderCode, shaderCodeSize, nullptr, &m_MainVs);
    assert(SUCCEEDED(hr));

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
}

void CApp::InitFont()
{
    m_Font = std::make_unique<CFont>();

    SFontDesc fontDesc;
    fontDesc.FaceName = FONT_CREATE_FACE_NAME;
    fontDesc.Height = FONT_CREATE_SIZE;
    fontDesc.Flags = FONT_CREATE_FLAGS;

    bool ok = m_Font->Init(fontDesc);
    assert(ok);
}

void CApp::InitTexture()
{
    assert(m_Font);

    uvec2 size = UVEC2_ZERO;
    size_t rowPitch = 0;
    const void* data = nullptr;
    m_Font->GetTextureData(data, size, rowPitch);

    CD3D11_TEXTURE2D_DESC textureDesc = CD3D11_TEXTURE2D_DESC(
        DXGI_FORMAT_A8_UNORM, size.x, size.y, 1, 1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);
    D3D11_SUBRESOURCE_DATA initialData = {data, (UINT)rowPitch, 0};
    HRESULT hr = m_Dev->CreateTexture2D(&textureDesc, &initialData, &m_Texture);
    assert(SUCCEEDED(hr));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = { textureDesc.Format, D3D_SRV_DIMENSION_TEXTURE2D };
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    hr = m_Dev->CreateShaderResourceView(m_Texture, &srvDesc, &m_TextureSRV);
    assert(SUCCEEDED(hr));

    m_Font->FreeTextureData();
}

void CApp::InitVertexAndIndexBuffer()
{
    assert(m_Font);

    const size_t quadCount = m_Font->CalcQuadCount(TEXT_TO_DISPLAY, FONT_DISPLAY_SIZE, FONT_DISPLAY_FLAGS, TEXT_WIDTH);

    QuadCountToVertexCount<VB_FLAGS>(m_VertexCount, m_IndexCount, quadCount);

    std::vector<SVertex> vertices(m_VertexCount);
    std::vector<uint16_t> indices(m_IndexCount);

    const vec2 pos = vec2(MARGIN, MARGIN);

    SVertexBufferDesc fontVbDesc;
    fontVbDesc.FirstPosition = &vertices[0].Pos;
    fontVbDesc.FirstTexCoord = &vertices[0].TexCoord;
    fontVbDesc.PositionStrideBytes = sizeof(SVertex);
    fontVbDesc.TexCoordStrideBytes = sizeof(SVertex);
    fontVbDesc.FirstIndex = indices.data();
    m_Font->GetTextVertices<VB_FLAGS>(fontVbDesc, pos, TEXT_TO_DISPLAY, FONT_DISPLAY_SIZE, FONT_DISPLAY_FLAGS, TEXT_WIDTH);

    PostprocessVertices(vertices.data(), vertices.size());
    
    CD3D11_BUFFER_DESC vbDesc = CD3D11_BUFFER_DESC(
        (UINT)(m_VertexCount * sizeof(SVertex)), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE);
    D3D11_SUBRESOURCE_DATA vbInitialData = {vertices.data()};
    HRESULT hr = m_Dev->CreateBuffer(&vbDesc, &vbInitialData, &m_VertexBuffer);
    assert(SUCCEEDED(hr));

    CD3D11_BUFFER_DESC ibDesc = CD3D11_BUFFER_DESC(
        (UINT)(m_IndexCount * sizeof(INDEX_TYPE)), D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE);
    D3D11_SUBRESOURCE_DATA ibInitialData = {indices.data()};
    hr = m_Dev->CreateBuffer(&ibDesc, &ibInitialData, &m_IndexBuffer);
    assert(SUCCEEDED(hr));
}

void CApp::SetOneTimeStates()
{
    m_Ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    assert(m_InputLayout);
    m_Ctx->IASetInputLayout(m_InputLayout);

    assert(m_VertexBuffer);
    const UINT vertexStride = sizeof(SVertex);
    const UINT offset = 0;
    m_Ctx->IASetVertexBuffers(0, 1, &m_VertexBuffer.p, &vertexStride, &offset);

    assert(m_IndexBuffer);
    m_Ctx->IASetIndexBuffer(m_IndexBuffer.p, INDEX_BUFFER_FORMAT, 0);

    assert(m_MainVs);
    m_Ctx->VSSetShader(m_MainVs, nullptr, 0);

    D3D11_VIEWPORT viewport = {};
    viewport.Width  = (float)DISPLAY_SIZE.x;
    viewport.Height = (float)DISPLAY_SIZE.y;
    viewport.MaxDepth = 1.f;
    m_Ctx->RSSetViewports(1, &viewport);

    assert(m_RasterizerState);
    m_Ctx->RSSetState(m_RasterizerState);

    assert(m_MainPs);
    m_Ctx->PSSetShader(m_MainPs, nullptr, 0);

    assert(m_TextureSRV);
    m_Ctx->PSSetShaderResources(0, 1, &m_TextureSRV.p);

    assert(m_SamplerState);
    m_Ctx->PSSetSamplers(0, 1, &m_SamplerState.p);

    assert(m_DepthStencilState);
    m_Ctx->OMSetDepthStencilState(m_DepthStencilState.p, 0);

    assert(m_BlendState);
    m_Ctx->OMSetBlendState(m_BlendState.p, nullptr, 0xffffffff);
}

void CApp::PostprocessVertices(SVertex* vertices, size_t count)
{
    const vec2 displaySizeInv = vec2(
        1.f / (float)DISPLAY_SIZE.x,
        1.f / (float)DISPLAY_SIZE.y);

    for(size_t i = 0; i < count; ++i)
    {
        // Transform Pos from source coordinate system, which is from left-top (0, 0) in pixels,
        // to destination coordinate system, which is from left-bottom (-1, -1) to right-top (1, 1).
        vertices[i].Pos.x = vertices[i].Pos.x * (displaySizeInv.x * 2.f) - 1.f;
        vertices[i].Pos.y = 1.f - vertices[i].Pos.y * (displaySizeInv.y * 2.f);

        // Fill Color, as it was uninitialized before.
        vertices[i].Color = 0xFFE0E0E0;
    }
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
            return g_App->WndProc(wnd, msg, wParam, lParam);
    }

    return DefWindowProc(wnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    HINSTANCE instance = (HINSTANCE)GetModuleHandle(NULL);

    CCoInitializeGuard coInitializeObj;

    WNDCLASSEX wndClassDesc = { sizeof(WNDCLASSEX) };
    wndClassDesc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wndClassDesc.hbrBackground = NULL;
    wndClassDesc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClassDesc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClassDesc.hInstance = instance;
    wndClassDesc.lpfnWndProc = &WndProc;
    wndClassDesc.lpszClassName = WINDOW_CLASS_NAME;

    ATOM wndClass = RegisterClassEx(&wndClassDesc);
    assert(wndClass);

    const DWORD wndStyle = WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    const DWORD wndExStyle = 0;

    ivec2 pos = ivec2(CW_USEDEFAULT, CW_USEDEFAULT);
    RECT tmpRect = {0, 0, (int)DISPLAY_SIZE.x, (int)DISPLAY_SIZE.y};
    AdjustWindowRectEx(&tmpRect, wndStyle, FALSE, wndExStyle);
    ivec2 size = ivec2(tmpRect.right - tmpRect.left, tmpRect.bottom - tmpRect.top);

    HWND wnd = CreateWindowEx(
        wndExStyle,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        wndStyle,
        pos.x, pos.y,
        size.x, size.y,
        NULL,
        NULL,
        instance,
        0);
    assert(wnd && g_App);

    MSG msg;
    bool quit = false;
    while(!quit)
    {
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                quit = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(g_App)
            g_App->Frame();
    }

    return 0;
}
