#pragma once

struct FRenderingContext
{
    FRenderingContext(){}

    FRenderingContext(const FCameraConstants* InViewProj, class UCamera* InCurrentCamera, EViewModeIndex InViewMode, uint64 InShowFlags, const D3D11_VIEWPORT& InViewport,
        const FVector2& InRenderTargetSize, EShadowProjectionType InShadowProjectionType, EShadowFilterType InShadowFilterType)
        : ViewProjConstants(InViewProj)
        , CurrentCamera(InCurrentCamera)
        , ViewMode(InViewMode)
        , ShowFlags(InShowFlags)
        , Viewport(InViewport)
        , RenderTargetSize(InRenderTargetSize)
        , ShadowProjectionType(InShadowProjectionType)
        , ShadowFilterType(InShadowFilterType)
    {}

    const FCameraConstants* ViewProjConstants= nullptr;
    UCamera* CurrentCamera = nullptr;
    EViewModeIndex ViewMode;
    uint64 ShowFlags;
    D3D11_VIEWPORT Viewport;
    FVector2 RenderTargetSize;
    float CSMLambda = 0.8f;

    EShadowProjectionType ShadowProjectionType = EShadowProjectionType::Default;
    EShadowFilterType ShadowFilterType = EShadowFilterType::None;

    TArray<class UPrimitiveComponent*> AllPrimitives;
    // Components By Render Pass
    TArray<class UStaticMeshComponent*> StaticMeshes;
    TArray<class UBillBoardComponent*> BillBoards;
    TArray<class UTextComponent*> Texts;
    TArray<class UUUIDTextComponent*> UUIDs;
    TArray<class UDecalComponent*> Decals;
    TArray<class UPointLightComponent*> PointLights;
    TArray<class USpotLightComponent*> SpotLights;
    TArray<class UDirectionalLightComponent*> DirectionalLights;
    TArray<class UAmbientLightComponent*> AmbientLights;
    TArray<class UHeightFogComponent*> Fogs;
};
