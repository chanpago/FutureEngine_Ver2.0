#include "pch.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Global/Types.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Global/Memory.h"
#include "Render/Renderer/Public/Renderer.h"

IMPLEMENT_SINGLETON_CLASS(UStatOverlay, UObject)

UStatOverlay::UStatOverlay() {}
UStatOverlay::~UStatOverlay() = default;

void UStatOverlay::Initialize()
{
    auto* DeviceResources = URenderer::GetInstance().GetDeviceResources();
    DWriteFactory = DeviceResources->GetDWriteFactory();

    if (DWriteFactory)
    {
        DWriteFactory->CreateTextFormat(
            L"Consolas",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            15.0f,
            L"en-us",
            &TextFormat
        );
    }
}

void UStatOverlay::Release()
{
    SafeRelease(TextFormat);

    DWriteFactory = nullptr;
}

void UStatOverlay::Render()
{
    TIME_PROFILE(StatDrawn);

    auto* DeviceResources = URenderer::GetInstance().GetDeviceResources();
    IDXGISwapChain* SwapChain = DeviceResources->GetSwapChain();
    ID3D11Device* D3DDevice = DeviceResources->GetDevice();

    ID2D1Factory1* D2DFactory = nullptr;
    D2D1_FACTORY_OPTIONS opts{};
#ifdef _DEBUG
    opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &opts, (void**)&D2DFactory)))
        return;

    IDXGISurface* Surface = nullptr;
    SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&Surface);

    IDXGIDevice* DXGIDevice = nullptr;
    D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&DXGIDevice);

    ID2D1Device* D2DDevice = nullptr;
    D2DFactory->CreateDevice(DXGIDevice, &D2DDevice);
    if (D2DDevice == nullptr)
    {
        return;
    }

    ID2D1DeviceContext* D2DCtx = nullptr;
    D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &D2DCtx);

    D2D1_BITMAP_PROPERTIES1 BmpProps = {};
    BmpProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    BmpProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    BmpProps.dpiX = 96.0f;
    BmpProps.dpiY = 96.0f;
    BmpProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

    ID2D1Bitmap1* TargetBmp = nullptr;
    D2DCtx->CreateBitmapFromDxgiSurface(Surface, &BmpProps, &TargetBmp);

    D2DCtx->SetTarget(TargetBmp);
    D2DCtx->BeginDraw();

    if (IsStatEnabled(EStatType::FPS))     RenderFPS(D2DCtx);
    if (IsStatEnabled(EStatType::Memory))  RenderMemory(D2DCtx);
    if (IsStatEnabled(EStatType::Picking)) RenderPicking(D2DCtx);
    if (IsStatEnabled(EStatType::Time))    RenderTimeInfo(D2DCtx);
    if (IsStatEnabled(EStatType::Decal))   RenderDecalInfo(D2DCtx);

    D2DCtx->EndDraw();
    D2DCtx->SetTarget(nullptr);

    SafeRelease(TargetBmp);
    SafeRelease(D2DCtx);
    SafeRelease(D2DDevice);
    SafeRelease(DXGIDevice);
    SafeRelease(Surface);
    SafeRelease(D2DFactory);
}

void UStatOverlay::RenderFPS(ID2D1DeviceContext* D2DCtx)
{
    auto& timeManager = UTimeManager::GetInstance();
    CurrentFPS = timeManager.GetFPS();
    FrameTime = timeManager.GetDeltaTime() * 1000;

    char buf[64];
    sprintf_s(buf, sizeof(buf), "FPS: %.1f (%.2f ms)", CurrentFPS, FrameTime);
    FString text = buf;

    float r = 0.5f, g = 1.0f, b = 0.5f;
    if (CurrentFPS < 30.0f) { r = 1.0f; g = 0.0f; b = 0.0f; }
    else if (CurrentFPS < 60.0f) { r = 1.0f; g = 1.0f; b = 0.0f; }

    RenderText(D2DCtx, text, OverlayX, OverlayY, r, g, b);
}

void UStatOverlay::RenderMemory(ID2D1DeviceContext* d2dCtx)
{
    float MemoryMB = static_cast<float>(TotalAllocationBytes) / (1024.0f * 1024.0f);

    char Buf[64];
    sprintf_s(Buf, sizeof(Buf), "Memory: %.1f MB (%u objects)", MemoryMB, TotalAllocationCount);
    FString text = Buf;

    float OffsetY = 0.0f;
    if (IsStatEnabled(EStatType::FPS))    OffsetY += 20.0f;
    RenderText(d2dCtx, text, OverlayX, OverlayY + OffsetY, 1.0f, 1.0f, 0.0f);
}

void UStatOverlay::RenderPicking(ID2D1DeviceContext* D2DCtx)
{
    float AvgMs = PickAttempts > 0 ? AccumulatedPickingTimeMs / PickAttempts : 0.0f;

    char Buf[128];
    sprintf_s(Buf, sizeof(Buf), "Picking Time %.2f ms (Attempts %u, Accum %.2f ms, Avg %.2f ms)",
        LastPickingTimeMs, PickAttempts, AccumulatedPickingTimeMs, AvgMs);
    FString Text = Buf;

    float OffsetY = 0.0f;
    if (IsStatEnabled(EStatType::FPS))    OffsetY += 20.0f;
    if (IsStatEnabled(EStatType::Memory)) OffsetY += 20.0f;

    float r = 0.0f, g = 1.0f, b = 0.8f;
    if (LastPickingTimeMs > 5.0f) { r = 1.0f; g = 0.0f; b = 0.0f; }
    else if (LastPickingTimeMs > 1.0f) { r = 1.0f; g = 1.0f; b = 0.0f; }

    RenderText(D2DCtx, Text, OverlayX, OverlayY + OffsetY, r, g, b);
}

void UStatOverlay::RenderDecalInfo(ID2D1DeviceContext* D2DCtx)
{
    {
        char Buf[128];
        sprintf_s(Buf, sizeof(Buf), "Rendered Decal: %d (Collided Components: %d)",
            RenderedDecal, CollidedCompCount);
        FString Text = Buf;
    
        float OffsetY = 0.0f;
        if (IsStatEnabled(EStatType::FPS))      OffsetY += 20.0f;
        if (IsStatEnabled(EStatType::Memory))   OffsetY += 20.0f;
        if (IsStatEnabled(EStatType::Picking))  OffsetY += 20.0f;

        RenderText(D2DCtx, Text, OverlayX, OverlayY + OffsetY, 0.f, 1.f, 0.f);
    }

    {
        char Buf[128];
        sprintf_s(Buf, sizeof(Buf), "Decal Pass Time: %.4f ms", FScopeCycleCounter::GetTimeProfile("DecalPass").Milliseconds);
        FString Text = Buf;
    
        float OffsetY = 20.0f;
        if (IsStatEnabled(EStatType::FPS))      OffsetY += 20.0f;
        if (IsStatEnabled(EStatType::Memory))   OffsetY += 20.0f;
        if (IsStatEnabled(EStatType::Picking))  OffsetY += 20.0f;
        RenderText(D2DCtx, Text, OverlayX, OverlayY + OffsetY, 0.f, 1.f, 0.f);
    }
}

void UStatOverlay::RenderTimeInfo(ID2D1DeviceContext* D2DCtx)
{
    const TArray<FString> ProfileKeys = FScopeCycleCounter::GetTimeProfileKeys();

    float OffsetY = 0.0f;
    if (IsStatEnabled(EStatType::FPS))    OffsetY += 20.0f;
    if (IsStatEnabled(EStatType::Memory)) OffsetY += 20.0f;
    if (IsStatEnabled(EStatType::Picking)) OffsetY += 20.0f;
    if (IsStatEnabled(EStatType::Decal))  OffsetY += 40.0f;


    float CurrentY = OverlayY + OffsetY;
    const float LineHeight = 20.0f;

    for (const FString& Key : ProfileKeys)
    {
        const FTimeProfile& Profile = FScopeCycleCounter::GetTimeProfile(Key);

        char buf[128];
        sprintf_s(buf, sizeof(buf), "%s: %.2f ms", Key.c_str(), Profile.Milliseconds);
        FString text = buf;

        float r = 0.8f, g = 0.8f, b = 0.8f;
        if (Profile.Milliseconds > 1.0f) { r = 1.0f; g = 1.0f; b = 0.0f; }

        RenderText(D2DCtx, text, OverlayX, CurrentY, r, g, b);
        CurrentY += LineHeight;
    }
}

void UStatOverlay::RenderText(ID2D1DeviceContext* D2DCtx, const FString& Text, float x, float y, float r, float g, float b)
{
    if (!D2DCtx || Text.empty() || !TextFormat) return;

    std::wstring wText = ToWString(Text);

    ID2D1SolidColorBrush* Brush = nullptr;
    if (FAILED(D2DCtx->CreateSolidColorBrush(D2D1::ColorF(r, g, b), &Brush)))
        return;

    D2D1_RECT_F rect = D2D1::RectF(x, y, x + 800.0f, y + 20.0f);
    D2DCtx->DrawTextW(
        wText.c_str(),
        static_cast<UINT32>(wText.length()),
        TextFormat,
        &rect,
        Brush
    );

    SafeRelease(Brush);
}

std::wstring UStatOverlay::ToWString(const FString& InStr)
{
    if (InStr.empty()) return std::wstring();

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, InStr.c_str(), (int)InStr.size(), NULL, 0);
    std::wstring wStr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, InStr.c_str(), (int)InStr.size(), &wStr[0], sizeNeeded);
    return wStr;
}

void UStatOverlay::EnableStat(EStatType type) { StatMask |= static_cast<uint8>(type); }
void UStatOverlay::DisableStat(EStatType type) { StatMask &= ~static_cast<uint8>(type); }
void UStatOverlay::SetStatType(EStatType type) { StatMask = static_cast<uint8>(type); }
bool UStatOverlay::IsStatEnabled(EStatType type) const { return (StatMask & static_cast<uint8>(type)) != 0; }

void UStatOverlay::RecordPickingStats(float elapsedMs)
{
    ++PickAttempts;
    LastPickingTimeMs = elapsedMs;
    AccumulatedPickingTimeMs += elapsedMs;
}

void UStatOverlay::RecordDecalStats(uint32 InRenderedDecal, uint32 InCollidedCompCount)
{
    RenderedDecal = InRenderedDecal;
    CollidedCompCount = InCollidedCompCount;
}
