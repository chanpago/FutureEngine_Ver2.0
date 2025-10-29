#include "pch.h"
#include "Render/RenderPass/Public/UpdateLightBufferPass.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Component/Public/SpotLightComponent.h"
#include "Component/Public/PointLightComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Level/Public/Level.h"



FUpdateLightBufferPass::FUpdateLightBufferPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, 
    ID3D11Buffer* InConstantBufferModel, ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout)
    : FRenderPass(InPipeline, InConstantBufferCamera, InConstantBufferModel)
    , ShadowMapVS(InVS)
    , ShadowMapPS(InPS)
    , ShadowMapInputLayout(InLayout)
{
    // PSM용 상수버퍼
    PSMConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FShadowMapConstants>();
    
    // Light 전용 Camera 상수 버퍼 생성
    LightCameraConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FCameraConstants>();
    // Shadow Map용 Viewport 초기화
    // Directional Light Shadow Map (2048x2048)
    // TODO: 이거 나중에 details에서 동적으로 변경 가능하게 수정(width, height)
    DirectionalShadowViewport.Width = 2048.0f;
    DirectionalShadowViewport.Height = 2048.0f;
    DirectionalShadowViewport.MinDepth = 0.0f;
    DirectionalShadowViewport.MaxDepth = 1.0f;
    DirectionalShadowViewport.TopLeftX = 0.0f;
    DirectionalShadowViewport.TopLeftY = 0.0f;

    // Spot Light Shadow Map (1024x1024, atlas 사용 시 타일별로 설정)
    SpotShadowViewport.Width = 1024.0f;
    SpotShadowViewport.Height = 1024.0f;
    SpotShadowViewport.MinDepth = 0.0f;
    SpotShadowViewport.MaxDepth = 1.0f;
    SpotShadowViewport.TopLeftX = 0.0f;
    SpotShadowViewport.TopLeftY = 0.0f;

    // Point Light Shadow Map (cube map face당 1024x1024)
    PointShadowViewport.Width = 1024.0f;
    PointShadowViewport.Height = 1024.0f;
    PointShadowViewport.MinDepth = 0.0f;
    PointShadowViewport.MaxDepth = 1.0f;
    PointShadowViewport.TopLeftX = 0.0f;
    PointShadowViewport.TopLeftY = 0.0f;

    // Create structured buffer for spot shadow atlas entries
    SpotShadowAtlasStructuredBuffer = FRenderResourceFactory::CreateStructuredBuffer<FSpotShadowAtlasEntry>(256);
    FRenderResourceFactory::CreateStructuredShaderResourceView(SpotShadowAtlasStructuredBuffer, &SpotShadowAtlasSRV);

    // Point shadow tier mapping buffer (maps global point light index -> tier + tier-local index)
    PointShadowCubeIndexStructuredBuffer = FRenderResourceFactory::CreateStructuredBuffer<FPointShadowTierMapping>(1024);
    FRenderResourceFactory::CreateStructuredShaderResourceView(PointShadowCubeIndexStructuredBuffer, &PointShadowCubeIndexSRV);
}

FUpdateLightBufferPass::~FUpdateLightBufferPass()
{
}

void FUpdateLightBufferPass::Execute(FRenderingContext& Context)
{
    // Context에 이미 수집된 Light 컴포넌트들을 사용
    // Shadow Map 베이킹 실행
    //BakeShadowMap(Context);
    NewBakeShadowMap(Context);
    BakeSpotShadowMap(Context);
    BakePointShadowMap(Context);
}

void FUpdateLightBufferPass::NewBakeShadowMap(FRenderingContext& Context)
{
    // +-+-+ RETURN IMMEDIATELY IF SHADOW RENDERING IS DISABLED +-+-+
    if (!(Context.ShowFlags & EEngineShowFlags::SF_Shadow)) { return; }
    const auto& Renderer = URenderer::GetInstance();
    auto DeviceContext = Renderer.GetDeviceContext();

    // +-+-+ CHECK CURRENT SETTINGS (PROJECTION + FILTER) +-+-+
    const EShadowProjectionType ProjectionType = Context.ShadowProjectionType;
    const EShadowFilterType FilterType = Context.ShadowFilterType;
    //UE_LOG("BakeShadowMap: Projection = %s, Filter = %s", ENUM_TO_STRING(ProjectionType), ENUM_TO_STRING(FilterType));

    // +-+-+ INITIALIZE RENDER TARGET / BASIC SETUP +-+-+
    UINT NumViewports = 1;
    D3D11_VIEWPORT OriginalViewport;                    // Store the original viewport
    DeviceContext->RSGetViewports(&NumViewports, &OriginalViewport);
    ID3D11RenderTargetView* OriginalRTVs = nullptr;     // Store the original Render Targets
    ID3D11DepthStencilView* OriginalDSV = nullptr;
    DeviceContext->OMGetRenderTargets(1, &OriginalRTVs, &OriginalDSV);

    // +-+-+ SET UP THE PIPELINE FOR SHADOW MAP RENDERING +-+-+
    ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState({ ECullMode::None, EFillMode::Solid });
    FPipelineInfo PipelineInfo = {
        ShadowMapInputLayout,
        ShadowMapVS,
        RS,
        URenderer::GetInstance().GetDefaultDepthStencilState(),
        URenderer::GetInstance().GetPixelShader(FilterType),  // ★ PS 바인드 (RenderPrimitive에서도 depth write 보장)
        nullptr,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    };
    Pipeline->UpdatePipeline(PipelineInfo);

    // +-+-+ GENERATE SHADOWS BASED ON THE PROJECTION METHOD +-+-+
    FShadowCalculationData LightData;
    CalculateShadowMatrices(ProjectionType, Context, LightData);

    for (int Idx = 0; Idx < LightData.LightViews.size(); Idx++)
    {
        // Set render target
        SetShadowRenderTarget(ProjectionType, FilterType, Idx);

        // Update constant buffer (b6)
        UpdateShadowCasterConstants(ProjectionType, LightData, Idx, Context);

        // Render all objects in the scene.
        for (auto MeshComp : Context.StaticMeshes)
        {
            if (!MeshComp || !MeshComp->IsVisible()) continue;
            RenderPrimitive(MeshComp);
        }
    }

    // +-+-+ CLEANUP: RESTORE RESOURCES AND VIEWPORT +-+-+
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    DeviceContext->RSSetViewports(1, &OriginalViewport);
    DeviceContext->OMSetRenderTargets(1, &OriginalRTVs, OriginalDSV);
}

void FUpdateLightBufferPass::BakePointShadowMap(FRenderingContext& Context)
{
    // +-+-+ RETURN IMMEDIATELY IF SHADOW RENDERING IS DISABLED +-+-+
    if (!(Context.ShowFlags & EEngineShowFlags::SF_Shadow)) { return; }
    const auto& Renderer = URenderer::GetInstance();
    auto DeviceContext = Renderer.GetDeviceContext();

    // +-+-+ CHECK CURRENT SETTINGS (PROJECTION + FILTER) +-+-+
    const EShadowProjectionType ProjectionType = Context.ShadowProjectionType;
    const EShadowFilterType FilterType = Context.ShadowFilterType;
    //UE_LOG("BakeShadowMap: Projection = %s, Filter = %s", ENUM_TO_STRING(ProjectionType), ENUM_TO_STRING(FilterType));

    // +-+-+ INITIALIZE RENDER TARGET / BASIC SETUP +-+-+
    ID3D11ShaderResourceView* NullSRV = nullptr;
    DeviceContext->PSSetShaderResources(14, 1, &NullSRV);
    UINT NumViewports = 1;
    D3D11_VIEWPORT OriginalViewport;                    // Store the original viewport
    DeviceContext->RSGetViewports(&NumViewports, &OriginalViewport);
    ID3D11DepthStencilView* OriginalDSV = nullptr;
    ID3D11RenderTargetView* OriginalRTVs = nullptr;
    DeviceContext->OMGetRenderTargets(1, &OriginalRTVs, &OriginalDSV);

    // +-+-+ SET UP THE PIPELINE FOR SHADOW MAP RENDERING +-+-+
    ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState({ ECullMode::None, EFillMode::Solid });
    FPipelineInfo PipelineInfo = {
        ShadowMapInputLayout,
        ShadowMapVS,
        RS,
        URenderer::GetInstance().GetDefaultDepthStencilState(),
        URenderer::GetInstance().GetPixelShader(FilterType),
        nullptr,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    };
    Pipeline->UpdatePipeline(PipelineInfo);

    // Build filtered list of point lights (visible + enabled)
    TArray<UPointLightComponent*> FilteredLights;
    for (auto* PL : Context.PointLights)
    {
        if (PL && PL->GetVisible() && PL->GetLightEnabled())
        {
            FilteredLights.push_back(PL);
        }
    }

    // +-+-+ CLASSIFY LIGHTS INTO 3 TIERS BASED ON SHADOW RESOLUTION SCALE +-+-+
    const UINT MaxLightsPerTier = Renderer.GetDeviceResources()->GetMaxLightsPerTier();

    struct TierGroup {
        TArray<UPointLightComponent*> Lights;
        UINT Resolution;
        const char* Name;
    };

    TierGroup Tiers[3] = {
        { {}, 512,  "Low" },   // Tier 0: Scale 0.25~0.75
        { {}, 1024, "Mid" },   // Tier 1: Scale 0.76~1.5
        { {}, 2048, "High" }   // Tier 2: Scale 1.51~4.0
    };

    // Classify lights into tiers
    for (auto* PL : FilteredLights)
    {
        if (!PL) continue;

        float scale = std::clamp(PL->GetShadowResolutionScale(), 0.25f, 4.0f);

        uint32 tierIndex;
        if (scale <= 0.75f)
            tierIndex = 0; // Low
        else if (scale <= 1.5f)
            tierIndex = 1; // Mid
        else
            tierIndex = 2; // High

        // Check if tier has capacity
        if (Tiers[tierIndex].Lights.size() < MaxLightsPerTier)
        {
            Tiers[tierIndex].Lights.push_back(PL);
        }
        else
        {
            UE_LOG_WARNING("Tier %s is full (%d/%d), light will not cast shadows",
                Tiers[tierIndex].Name, Tiers[tierIndex].Lights.size(), MaxLightsPerTier);
        }
    }

    UE_LOG("Point Light Shadow Tier Classification:");
    UE_LOG("  Low  Tier (512):  %d lights", Tiers[0].Lights.size());
    UE_LOG("  Mid  Tier (1024): %d lights", Tiers[1].Lights.size());
    UE_LOG("  High Tier (2048): %d lights", Tiers[2].Lights.size());

    // Prepare tier mapping: global point light index -> (tier, tier-local-index)
    TArray<FPointShadowTierMapping> TierMapping;
    TierMapping.resize(Context.PointLights.size());
    // Initialize all to invalid
    for (size_t i = 0; i < TierMapping.size(); ++i)
    {
        TierMapping[i].Tier = 0xFFFFFFFFu;
        TierMapping[i].TierLocalIndex = 0xFFFFFFFFu;
    }

    // Clear CPU mapping
    PointCubeIndexCPU.clear();
    const float zn = 0.1f;

    // Helper lambdas for matrix calculations
    auto PerspectiveRowLH = [](float fovY, float aspect, float zn_, float zf_) {
        FMatrix P = FMatrix::Identity();
        const float yScale = 1.0f / tanf(fovY * 0.5f);
        const float xScale = yScale / aspect;
        P.Data[0][0] = xScale;
        P.Data[1][1] = yScale;
        P.Data[2][2] = zf_ / (zf_ - zn_);
        P.Data[3][2] = -zn_ * zf_ / (zf_ - zn_);
        P.Data[2][3] = 1.0f;
        P.Data[3][3] = 0.0f;
        return P;
    };

    auto LookAtRow = [](const FVector& Eye, const FVector& Fwd, const FVector& UpRef) {
        const FVector f = Fwd.GetNormalized();
        const FVector r = UpRef.Cross(f).GetNormalized();
        const FVector u = f.Cross(r);
        FMatrix V = FMatrix::Identity();
        V.Data[0][0]=r.X; V.Data[0][1]=u.X; V.Data[0][2]=f.X;
        V.Data[1][0]=r.Y; V.Data[1][1]=u.Y; V.Data[1][2]=f.Y;
        V.Data[2][0]=r.Z; V.Data[2][1]=u.Z; V.Data[2][2]=f.Z;
        V.Data[3][0]= -Eye.Dot(r);
        V.Data[3][1]= -Eye.Dot(u);
        V.Data[3][2]= -Eye.Dot(f);
        return V;
    };

    // Cube face directions
    const FVector fwd[6] = {
        FVector( 1, 0, 0), FVector(-1, 0, 0),
        FVector( 0, 1, 0), FVector( 0,-1, 0),
        FVector( 0, 0, 1), FVector( 0, 0,-1)
    };
    const FVector upv[6] = {
        FVector(0, 1, 0), FVector(0, 1, 0),
        FVector(0, 0,-1), FVector(0, 0, 1),
        FVector(0, 1, 0), FVector(0, 1, 0)
    };

    // +-+-+ BAKE EACH TIER +-+-+
    for (uint32 tierIndex = 0; tierIndex < 3; ++tierIndex)
    {
        TierGroup& tier = Tiers[tierIndex];
        if (tier.Lights.empty()) continue;

        UE_LOG("Baking %s Tier (%dx%d) with %d lights...",
            tier.Name, tier.Resolution, tier.Resolution, tier.Lights.size());

        // Setup viewport for this tier
        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<float>(tier.Resolution);
        vp.Height = static_cast<float>(tier.Resolution);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0.0f;
        vp.TopLeftY = 0.0f;

        // Bake each light in this tier
        for (uint32 tierLocalIdx = 0; tierLocalIdx < (uint32)tier.Lights.size(); ++tierLocalIdx)
        {
            UPointLightComponent* PL = tier.Lights[tierLocalIdx];
            if (!PL) continue;

            const FVector eye = PL->GetWorldLocation();
            const float zf = std::max(zn + 0.1f, PL->GetAttenuationRadius());
            const FMatrix P = PerspectiveRowLH(1.57079633f, 1.0f, zn, zf); // 90° fov

            // Find global index for this light
            uint32 globalIndex = 0xFFFFFFFFu;
            for (uint32 gi = 0; gi < (uint32)Context.PointLights.size(); ++gi)
            {
                if (Context.PointLights[gi] == PL) { globalIndex = gi; break; }
            }

            // Record mapping
            if (globalIndex != 0xFFFFFFFFu)
            {
                TierMapping[globalIndex].Tier = tierIndex;
                TierMapping[globalIndex].TierLocalIndex = tierLocalIdx;
                PointCubeIndexCPU[PL] = tierLocalIdx; // For UI (tier info stored separately)
            }

            // Render 6 cube faces
            for (int face = 0; face < 6; ++face)
            {
                const UINT sliceIndex = tierLocalIdx * 6 + face;

                // Get DSV/RTV for the appropriate tier
                ID3D11DepthStencilView* dsv = nullptr;
                ID3D11RenderTargetView* rtv = nullptr;
                ID3D11ShaderResourceView* colorSRV = nullptr;

                switch (tierIndex)
                {
                case 0: // Low Tier
                    dsv = Renderer.GetDeviceResources()->GetPointShadowLowTierDSV((int)sliceIndex);
                    if (FilterType == EShadowFilterType::VSM)
                    {
                        rtv = Renderer.GetDeviceResources()->GetPointShadowLowTierRTV((int)sliceIndex);
                        colorSRV = Renderer.GetDeviceResources()->GetPointShadowLowTierColorSRV();
                    }
                    break;
                case 1: // Mid Tier
                    dsv = Renderer.GetDeviceResources()->GetPointShadowMidTierDSV((int)sliceIndex);
                    if (FilterType == EShadowFilterType::VSM)
                    {
                        rtv = Renderer.GetDeviceResources()->GetPointShadowMidTierRTV((int)sliceIndex);
                        colorSRV = Renderer.GetDeviceResources()->GetPointShadowMidTierColorSRV();
                    }
                    break;
                case 2: // High Tier
                    dsv = Renderer.GetDeviceResources()->GetPointShadowHighTierDSV((int)sliceIndex);
                    if (FilterType == EShadowFilterType::VSM)
                    {
                        rtv = Renderer.GetDeviceResources()->GetPointShadowHighTierRTV((int)sliceIndex);
                        colorSRV = Renderer.GetDeviceResources()->GetPointShadowHighTierColorSRV();
                    }
                    break;
                }

                if (!dsv) continue;

                // Setup render targets based on filter type
                if (FilterType == EShadowFilterType::VSM && rtv)
                {
                    // Unbind SRVs to avoid hazards
                    ID3D11ShaderResourceView* NullSRVs[2] = { nullptr, nullptr };
                    DeviceContext->PSSetShaderResources(16, 1, &NullSRVs[0]);
                    DeviceContext->PSSetShaderResources(17, 1, &NullSRVs[1]);
                    DeviceContext->OMSetRenderTargets(1, &rtv, dsv);

                    const float ClearMoments[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
                    DeviceContext->ClearRenderTargetView(rtv, ClearMoments);
                    DeviceContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
                }
                else
                {
                    // Depth-only
                    DeviceContext->OMSetRenderTargets(0, nullptr, dsv);
                    DeviceContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
                }

                DeviceContext->RSSetViewports(1, &vp);

                // Update constants for this face
                FShadowMapConstants C = {};
                C.LightViewP[0] = LookAtRow(eye, fwd[face], upv[face]);
                C.LightProjP[0] = P;
                C.LightViewPInv[0] = C.LightViewP[0].Inverse();
                C.ShadowMapSize = FVector2(vp.Width, vp.Height);
                C.bUsePSM = 0; C.bUseVSM = 0; C.bUsePCF = 0; C.bUseCSM = 0;
                FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, C);
                Pipeline->SetConstantBuffer(6, EShaderType::VS, PSMConstantBuffer);

                // Render all meshes from this face's perspective
                for (auto* MeshComp : Context.StaticMeshes)
                {
                    if (!MeshComp || !MeshComp->IsVisible()) continue;
                    RenderPrimitive(MeshComp);
                }
            }

            // Generate mips for VSM after all 6 faces
            if (FilterType == EShadowFilterType::VSM)
            {
                ID3D11ShaderResourceView* srv = nullptr;
                switch (tierIndex)
                {
                case 0: srv = Renderer.GetDeviceResources()->GetPointShadowLowTierColorSRV(); break;
                case 1: srv = Renderer.GetDeviceResources()->GetPointShadowMidTierColorSRV(); break;
                case 2: srv = Renderer.GetDeviceResources()->GetPointShadowHighTierColorSRV(); break;
                }
                if (srv) { DeviceContext->GenerateMips(srv); }
            }
        }
    }

    // Upload tier mapping to GPU
    if (PointShadowCubeIndexStructuredBuffer)
    {
        FRenderResourceFactory::UpdateStructuredBuffer(PointShadowCubeIndexStructuredBuffer, TierMapping);
    }

    // Restore bindings
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    DeviceContext->RSSetViewports(1, &OriginalViewport);
    DeviceContext->OMSetRenderTargets(1, &OriginalRTVs, OriginalDSV);

    UE_LOG("Point Light Shadow Map Baking Complete (3-Tier System)");
}

bool FUpdateLightBufferPass::GetPointCubeIndexCPU(const UPointLightComponent* Comp, uint32& OutCubeIdx) const
{
    auto it = PointCubeIndexCPU.find(Comp);
    if (it == PointCubeIndexCPU.end()) return false;
    OutCubeIdx = it->second;
    return true;
}

void FUpdateLightBufferPass::BakeSpotShadowMap(FRenderingContext& Context)
{
    // +-+-+ RETURN IMMEDIATELY IF SHADOW RENDERING IS DISABLED +-+-+
    if (!(Context.ShowFlags & EEngineShowFlags::SF_Shadow)) { return; }
    const auto& Renderer = URenderer::GetInstance();
    auto DeviceContext = Renderer.GetDeviceContext();

    // +-+-+ CHECK CURRENT SETTINGS (PROJECTION + FILTER) +-+-+
    const EShadowProjectionType ProjectionType = Context.ShadowProjectionType;
    const EShadowFilterType FilterType = Context.ShadowFilterType;
    //UE_LOG("BakeShadowMap: Projection = %s, Filter = %s", ENUM_TO_STRING(ProjectionType), ENUM_TO_STRING(FilterType));

    // +-+-+ INITIALIZE RENDER TARGET / BASIC SETUP +-+-+
    ID3D11ShaderResourceView* NullSRV = nullptr;
    DeviceContext->PSSetShaderResources(12, 1, &NullSRV);
    UINT NumViewports = 1;
    D3D11_VIEWPORT OriginalViewport;                    // Store the original viewport
    DeviceContext->RSGetViewports(&NumViewports, &OriginalViewport);
    ID3D11DepthStencilView* OriginalDSV = nullptr;
    ID3D11RenderTargetView* OriginalRTVs = nullptr;  
    DeviceContext->OMGetRenderTargets(1, &OriginalRTVs, &OriginalDSV);

    // +-+-+ SET UP THE PIPELINE FOR SHADOW MAP RENDERING +-+-+
    ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState({ ECullMode::None, EFillMode::Solid });
    FPipelineInfo PipelineInfo = {
        ShadowMapInputLayout,
        ShadowMapVS,
        RS,
        URenderer::GetInstance().GetDefaultDepthStencilState(),
        URenderer::GetInstance().GetPixelShader(FilterType),  // ★ PS 바인드 (RenderPrimitive에서도 depth write 보장)
        nullptr,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    };
    Pipeline->UpdatePipeline(PipelineInfo);


    // +-+-+ BAKE SPOTLIGHT SHADOW MAP (Multi-Resolution Atlas, 3-Tier System) +-+-+
    // Build filtered spot list matching LightPass ordering (visible + enabled)
    TArray<USpotLightComponent*> FilteredSpots;
    for (auto* SL : Context.SpotLights)
    {
        // TODO: also check SL->GetCastShadows()
        if (SL && SL->GetVisible() && SL->GetLightEnabled())
        {
            FilteredSpots.push_back(SL);
        }
    }
    const int32 NumSpotLights = static_cast<int32>(FilteredSpots.size());
    UE_LOG("BakeSpotShadowMap: NumSpotLights = %d (3-Tier System)", NumSpotLights);

    // Classify lights into tiers based on ShadowResolutionScale
    struct FSpotLightTierInfo
    {
        USpotLightComponent* Light;
        uint32 Tier;           // 0=Low, 1=Mid, 2=High
        uint32 TierLocalIndex; // Index within tier
        uint32 GlobalIndex;    // Original index in FilteredSpots
    };
    TArray<FSpotLightTierInfo> TierClassified[3]; // [0]=Low, [1]=Mid, [2]=High

    for (uint32 idx = 0; idx < (uint32)NumSpotLights; ++idx)
    {
        auto* SL = FilteredSpots[idx];
        if (!SL) continue;

        float scale = SL->GetShadowResolutionScale();
        uint32 tier = 0; // Default to Low

        if (scale > FSpotShadowAtlasLayout::MidThreshold)
            tier = 2; // High
        else if (scale > FSpotShadowAtlasLayout::LowThreshold)
            tier = 1; // Mid
        else
            tier = 0; // Low

        // Check if tier is full
        uint32 maxLights = (tier == 2) ? FSpotShadowAtlasLayout::MaxHighLights :
                          (tier == 1) ? FSpotShadowAtlasLayout::MaxMidLights :
                                       FSpotShadowAtlasLayout::MaxLowLights;

        if (TierClassified[tier].size() >= maxLights)
        {
            UE_LOG("Warning: Spot Light tier %d is full, skipping light %d", tier, idx);
            continue;
        }

        FSpotLightTierInfo info;
        info.Light = SL;
        info.Tier = tier;
        info.TierLocalIndex = static_cast<uint32>(TierClassified[tier].size());
        info.GlobalIndex = idx;
        TierClassified[tier].push_back(info);
    }

    UE_LOG("Spot Shadow Tier Distribution: Low=%d, Mid=%d, High=%d",
           TierClassified[0].size(), TierClassified[1].size(), TierClassified[2].size());
    
    if (NumSpotLights > 0)
    {
        // Unbind any SRVs that alias the spot shadow atlas to avoid D3D11 read-write hazards
        // StaticMeshPass binds the spot atlas at PS t12 and the atlas entries at t13
        ID3D11ShaderResourceView* NullSRVs[2] = { nullptr, nullptr };
        DeviceContext->PSSetShaderResources(12, 2, NullSRVs);

        ID3D11DepthStencilView* dsv = Renderer.GetDeviceResources()->GetSpotShadowMapDSV();
        UE_LOG("SpotShadowMapDSV = %p", dsv);
        if (FilterType == EShadowFilterType::VSM)
        {
            ID3D11RenderTargetView* rtv = Renderer.GetDeviceResources()->GetSpotShadowMapColorRTV();
            // Unbind SRV to avoid write conflict
            ID3D11ShaderResourceView* NullSRV = nullptr;
            DeviceContext->PSSetShaderResources(12, 1, &NullSRV);
            DeviceContext->OMSetRenderTargets(1, &rtv, dsv);
            const float ClearMoments[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
            DeviceContext->ClearRenderTargetView(rtv, ClearMoments);
            DeviceContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        }
        else
        {
            DeviceContext->OMSetRenderTargets(0, nullptr, dsv);
            // Clear entire atlas once
            DeviceContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        }

        // Build entries to upload to structured buffer (index by global spot index)
        TArray<FSpotShadowAtlasEntry> Entries;
        Entries.resize(NumSpotLights);

        const float atlasW = static_cast<float>(FSpotShadowAtlasLayout::AtlasWidth);
        const float atlasH = static_cast<float>(FSpotShadowAtlasLayout::AtlasHeight);

        // Render each tier
        for (uint32 tierIndex = 0; tierIndex < 3; ++tierIndex)
        {
            const auto& TierLights = TierClassified[tierIndex];
            if (TierLights.empty()) continue;

            // Get tier parameters
            uint32 tileSize, tilesX, offsetY;
            const char* tierName;

            if (tierIndex == 2) // High
            {
                tileSize = FSpotShadowAtlasLayout::HighTileSize;
                tilesX = FSpotShadowAtlasLayout::HighTilesX;
                offsetY = FSpotShadowAtlasLayout::HighOffsetY;
                tierName = "High";
            }
            else if (tierIndex == 1) // Mid
            {
                tileSize = FSpotShadowAtlasLayout::MidTileSize;
                tilesX = FSpotShadowAtlasLayout::MidTilesX;
                offsetY = FSpotShadowAtlasLayout::MidOffsetY;
                tierName = "Mid";
            }
            else // Low
            {
                tileSize = FSpotShadowAtlasLayout::LowTileSize;
                tilesX = FSpotShadowAtlasLayout::LowTilesX;
                offsetY = FSpotShadowAtlasLayout::LowOffsetY;
                tierName = "Low";
            }

            const float tileW = static_cast<float>(tileSize);
            const float tileH = static_cast<float>(tileSize);

            UE_LOG("Rendering Spot Shadow Tier %s: %d lights at %dx%d", tierName, TierLights.size(), tileSize, tileSize);

            // Render each light in this tier
            for (const auto& TierInfo : TierLights)
            {
                USpotLightComponent* SpotCaster = TierInfo.Light;
                if (!SpotCaster) continue;

                const uint32 localIdx = TierInfo.TierLocalIndex;
                const uint32 globalIdx = TierInfo.GlobalIndex;

                FMatrix V, P, PSMMatrix = FMatrix::Identity();

                // Check if PSM is enabled for this spotlight
                // ⚠️ PSM은 SpotLight에 부적합: 카메라 의존성 때문에 그림자가 불안정
                // 권장: Cast Shadows를 꺼서 표준 Perspective Shadow Mapping 사용
                if (SpotCaster->GetCastShadows())
                {
                    // === PSM Path (Experimental - Camera Dependent!) ===
                    UE_LOG("SpotLight[%d] using PSM (Warning: Camera dependent!)", globalIdx);
                    UCamera* MainCamera = Context.CurrentCamera;
                    if (!MainCamera) continue;

                    FMatrix CameraView = MainCamera->GetCameraViewMatrix();
                    FMatrix CameraProj = MainCamera->GetCameraProjectionMatrix();

                    const FVector eye = SpotCaster->GetWorldLocation();
                    const FVector fwd = SpotCaster->GetForwardVector().GetNormalized();
                    const float outerAngle = FVector::GetDegreeToRadian(SpotCaster->GetOuterConeAngle());

                    BuildSpotLightPSM(CameraView, CameraProj, eye, fwd, outerAngle,
                        (int)tileW, (int)tileH, V, P, PSMMatrix);
                    //UE_LOG("  LightPos=(%.2f,%.2f,%.2f) Dir=(%.2f,%.2f,%.2f)", eye.X, eye.Y, eye.Z, fwd.X, fwd.Y, fwd.Z);
                }
                else
                {
                    // === Standard Path (LVP) ===
                    UE_LOG("SpotLight[%d] using Standard Shadow (Camera Independent)", globalIdx);
                    // Compute view matrix from light basis
                    const FVector eye = SpotCaster->GetWorldLocation();
                    const FVector fwd = SpotCaster->GetForwardVector().GetNormalized();
                    UE_LOG("  Light Pos=(%.2f, %.2f, %.2f), Fwd=(%.2f, %.2f, %.2f)",
                        eye.X, eye.Y, eye.Z, fwd.X, fwd.Y, fwd.Z);
                    const FVector tmpUp = (fabsf(fwd.Z) > 0.99f) ? FVector(1,0,0) : FVector(0,0,1);
                    const FVector right = tmpUp.Cross(fwd).GetNormalized();
                    const FVector up = fwd.Cross(right);

                    V = FMatrix::Identity();
                    V.Data[0][0]=right.X; V.Data[0][1]=up.X; V.Data[0][2]=fwd.X;
                    V.Data[1][0]=right.Y; V.Data[1][1]=up.Y; V.Data[1][2]=fwd.Y;
                    V.Data[2][0]=right.Z; V.Data[2][1]=up.Z; V.Data[2][2]=fwd.Z;
                    V.Data[3][0]= -eye.Dot(right);
                    V.Data[3][1]= -eye.Dot(up);
                    V.Data[3][2]= -eye.Dot(fwd);

                    // Perspective projection (row-vector LH)
                    const float fov = std::max(0.001f, SpotCaster->GetOuterConeAngle()) * 2.0f;
                    const float aspect = 1.0f;
                    const float zn = 0.1f;
                    const float zf = std::max(zn + 0.1f, SpotCaster->GetAttenuationRadius());
                    P = FMatrix::Identity();
                    const float yScale = 1.0f / tanf(fov * 0.5f);
                    const float xScale = yScale / aspect;
                    P.Data[0][0] = xScale;
                    P.Data[1][1] = yScale;
                    P.Data[2][2] = zf / (zf - zn);
                    P.Data[3][2] = -zn * zf / (zf - zn);
                    P.Data[2][3] = 1.0f;
                    P.Data[3][3] = 0.0f;
                }

                // Update constant buffer used by shadow depth pass (b6 VS)
                FShadowMapConstants SpotCasterConsts = {};

                // PSM 경로일 때는 V와 P가 PSM 변환을 포함
                // 일반 경로일 때는 V와 P가 표준 Light View/Proj
                SpotCasterConsts.LightViewP[0] = V;
                SpotCasterConsts.LightProjP[0] = P;
                SpotCasterConsts.LightViewPInv[0] = V.Inverse();
                SpotCasterConsts.CameraClipToLightClip = PSMMatrix;  // PSM: World → Light Clip (전체 변환)
                SpotCasterConsts.EyeView = Context.CurrentCamera ? Context.CurrentCamera->GetCameraViewMatrix() : FMatrix::Identity();
                SpotCasterConsts.EyeProj = Context.CurrentCamera ? Context.CurrentCamera->GetCameraProjectionMatrix() : FMatrix::Identity();
                SpotCasterConsts.ShadowMapSize = FVector2(tileW, tileH);
                SpotCasterConsts.ShadowParams = FVector4(SpotCaster->GetShadowBias(), SpotCaster->GetShadowSlopeBias(), 0, 0);
                // LightDirWS: "표면 → 광원" 방향 (bias를 위해)
                SpotCasterConsts.LightDirWS = -SpotCaster->GetForwardVector().GetNormalized();
                SpotCasterConsts.bInvertedLight = 0;
                SpotCasterConsts.bUsePSM = SpotCaster->GetCastShadows() ? 1 : 0;
                FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, SpotCasterConsts);

                // Calculate atlas tile position based on tier layout
                const uint32 tileX = localIdx % tilesX;
                const uint32 tileY = localIdx / tilesX;

                D3D11_VIEWPORT vp = {};
                vp.TopLeftX = tileW * static_cast<float>(tileX);
                vp.TopLeftY = static_cast<float>(offsetY) + tileH * static_cast<float>(tileY);
                vp.Width = tileW;
                vp.Height = tileH;
                vp.MinDepth = 0.0f;
                vp.MaxDepth = 1.0f;
                DeviceContext->RSSetViewports(1, &vp);

                // Render scene from this spot light POV into its tile
                for (auto MeshComp : Context.StaticMeshes)
                {
                    if (!MeshComp || !MeshComp->IsVisible()) continue;
                    RenderPrimitive(MeshComp);
                }

                // First one cached for backwards compatibility (used by StaticMeshPass)
                if (globalIdx == 0)
                {
                    SpotLightView = V;
                    SpotLightProj = P;
                }

                // Compute atlas transform with 1.5 texel inner padding to avoid bleeding
                const float sx = tileW / atlasW;
                const float sy = tileH / atlasH;
                const float ox = vp.TopLeftX / atlasW;
                const float oy = vp.TopLeftY / atlasH;
                const float padU = 1.5f / atlasW;
                const float padV = 1.5f / atlasH;

                FSpotShadowAtlasEntry entry;
                entry.View = V;
                entry.Proj = P;
                entry.AtlasScale = FVector2(std::max(0.0f, sx - 2.0f * padU), std::max(0.0f, sy - 2.0f * padV));
                entry.AtlasOffset = FVector2(ox + padU, oy + padV);
                entry.PSMMatrix = PSMMatrix;
                entry.bUsePSM = SpotCaster->GetCastShadows() ? 1 : 0;
                entry.Padding = FVector(0, 0, 0);

                // Write at the same index used by clustered lighting for this spotlight
                Entries[globalIdx] = entry;
            }
        }

        // Upload entries to GPU structured buffer
        if (SpotShadowAtlasStructuredBuffer)
        {
            FRenderResourceFactory::UpdateStructuredBuffer(SpotShadowAtlasStructuredBuffer, Entries);
        }
    }
    
    // +-+-+ CLEANUP: RESTORE RESOURCES AND VIEWPORT +-+-+
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    DeviceContext->RSSetViewports(1, &OriginalViewport);
    DeviceContext->OMSetRenderTargets(1, &OriginalRTVs, OriginalDSV);
}

void FUpdateLightBufferPass::BakeShadowMap(FRenderingContext& Context)
{
    
    const auto& Renderer = URenderer::GetInstance();
    auto DeviceContext = Renderer.GetDeviceContext();

    ID3D11ShaderResourceView* NullSRV = nullptr;
    DeviceContext->PSSetShaderResources(10, 1, &NullSRV);
    
    // Shadow Map 렌더링용 파이프라인 설정
    if (!ShadowMapVS || !ShadowMapInputLayout)
    {
        return; // 셰이더가 없으면 스킵
    }

    // 원본 viewport 저장
    UINT NumViewports = 1;
    D3D11_VIEWPORT OriginalViewport;
    DeviceContext->RSGetViewports(&NumViewports, &OriginalViewport);

    // 원본 Render Targets 저장
    ID3D11RenderTargetView* OriginalRTVs =  nullptr;
    ID3D11DepthStencilView* OriginalDSV = nullptr;
    DeviceContext->OMGetRenderTargets(1, &OriginalRTVs, &OriginalDSV);

    const bool bUseCSM = (Context.ShowFlags & EEngineShowFlags::SF_CSM) != 0;
    if (bUseCSM)
    {
        //UE_LOG("CSM Path Enabled: Generate Cascaded Shadow Maps.");

        //const UCamera* Camera = Context.CurrentCamera;
        //if (!Camera) return;

        //UDirectionalLightComponent* Light = Context.DirectionalLights.empty() ? nullptr : Context.DirectionalLights[0];
        //if (!Light) return;

        CascadedShadowMapConstants = {};
        //CalculateCascadeSplits(CascadedShadowMapConstants.CascadeSplits, Camera);

        //const float* pSplits = &CascadedShadowMapConstants.CascadeSplits.X;
        DeviceContext->RSSetViewports(1, &DirectionalShadowViewport);

        for (int i = 0; i < MAX_CASCADES; i++)
        {
            /*ID3D11DepthStencilView* CurrentDsv = Renderer.GetInstance().GetDeviceResources()->GetCascadedShadowMapDSV(i);
            DeviceContext->OMSetRenderTargets(0, nullptr, CurrentDsv);
            DeviceContext->ClearDepthStencilView(CurrentDsv, D3D11_CLEAR_DEPTH, 1.0f, 0);*/

            //// Calculate current cascade slices' corner
            //float NearSplit = (i == 0) ? Camera->GetNearZ() : pSplits[i - 1];
            //float FarSplit = pSplits[i];
            //FVector FrustumCorners[8];
            //Camera->GetFrustumCorners(FrustumCorners, NearSplit, FarSplit);

            //// Calculate the center of cascade slice
            //FVector FrustumCenter = FVector::ZeroVector();
            //for (int j = 0; j < 8; j++)
            //{
            //    FrustumCenter += FrustumCorners[j];
            //}
            //FrustumCenter /= 8.0f;

            // Calculate the light's View matrix based on the frustum's center point 
            //FMatrix LightViewMatrix;
            //{
            //    // Set light position
            //    FVector LightDir = Light->GetForwardVector().GetNormalized();
            //    float ShadowDistance = 250.0f;
            //    /*float CameraRange = Camera->GetFarZ() - Camera->GetNearZ();
            //    float ShadowDistance = std::min(500.0f, CameraRange * 0.5f);*/
            //    FVector LightPos = FrustumCenter - LightDir * ShadowDistance;

            //    // light source targets the center of slice
            //    FVector TargetPos = FrustumCenter;
            //    FVector UpVector = (abs(LightDir.Z) > 0.99f) ? FVector(0, 1, 0) : FVector(0, 0, 1);

            //    // LookAt matrix
            //    FVector ZAxis = (TargetPos - LightPos).GetNormalized();
            //    FVector XAxis = (UpVector.Cross(ZAxis)).GetNormalized();
            //    FVector YAxis = ZAxis.Cross(XAxis);

            //    LightViewMatrix = FMatrix::Identity();
            //    LightViewMatrix.Data[0][0] = XAxis.X;   LightViewMatrix.Data[1][0] = XAxis.Y;   LightViewMatrix.Data[2][0] = XAxis.Z;
            //    LightViewMatrix.Data[0][1] = YAxis.X;   LightViewMatrix.Data[1][1] = YAxis.Y;   LightViewMatrix.Data[2][1] = YAxis.Z;
            //    LightViewMatrix.Data[0][2] = ZAxis.X;   LightViewMatrix.Data[1][2] = ZAxis.Y;   LightViewMatrix.Data[2][2] = ZAxis.Z;
            //    LightViewMatrix.Data[3][0] = -XAxis.FVector::Dot(LightPos);
            //    LightViewMatrix.Data[3][1] = -YAxis.FVector::Dot(LightPos);
            //    LightViewMatrix.Data[3][2] = -ZAxis.FVector::Dot(LightPos);
            //}

            //// Calculate a tight Projection matrix
            /*FMatrix CascadeLightProj;
            {
                FVector FrustumCornersLightView[8];
                for (int j = 0; j < 8; j++)
                {
                    FrustumCornersLightView[j] = FVector4(FrustumCorners[j], 1.0f) * LightViewMatrix;
                }

                FVector MinVec = FrustumCornersLightView[0];
                FVector MaxVec = FrustumCornersLightView[0];
                for (int j = 0; j < 8; j++)
                {
                    MinVec.X = std::min(MinVec.X, FrustumCornersLightView[j].X);
                    MinVec.Y = std::min(MinVec.Y, FrustumCornersLightView[j].Y);
                    MinVec.Z = std::min(MinVec.Z, FrustumCornersLightView[j].Z);
                    MaxVec.X = std::max(MaxVec.X, FrustumCornersLightView[j].X);
                    MaxVec.Y = std::max(MaxVec.Y, FrustumCornersLightView[j].Y);
                    MaxVec.Z = std::max(MaxVec.Z, FrustumCornersLightView[j].Z);
                }

                CascadeLightProj = FMatrix::Identity();
                CascadeLightProj.Data[0][0] = 2.0f / (MaxVec.X - MinVec.X);
                CascadeLightProj.Data[1][1] = 2.0f / (MaxVec.Y - MinVec.Y);
                CascadeLightProj.Data[2][2] = 1.0f / (MaxVec.Z - MinVec.Z);
                CascadeLightProj.Data[3][0] = -(MaxVec.X + MinVec.X) / (MaxVec.X - MinVec.X);
                CascadeLightProj.Data[3][1] = -(MaxVec.Y + MinVec.Y) / (MaxVec.Y - MinVec.Y);
                CascadeLightProj.Data[3][2] = -MinVec.Z / (MaxVec.Z - MinVec.Z);
            }*/
            //CascadedShadowMapConstants.LightViewP[i] = LightViewMatrix;
            //CascadedShadowMapConstants.LightProjP[i] = CascadeLightProj;

            /*FCameraConstants LightCameraConsts;
            LightCameraConsts.View = LightViewMatrix;
            LightCameraConsts.Projection = CascadeLightProj;
            FRenderResourceFactory::UpdateConstantBufferData(LightCameraConstantBuffer, LightCameraConsts);*/

            for (auto MeshComp : Context.StaticMeshes)
            {
                if (!MeshComp || !MeshComp->IsVisible())    continue;
                RenderPrimitive(MeshComp);
            }
        }
    }
    else
    {
        // Directional Light Shadow Map 렌더링
        for (auto Light : Context.DirectionalLights)
        {
            //if (!Light)
            //{
            //    continue;
            //}
        
            //// === LVP용: 월드 전체 AABB 집계 (카메라에 독립)
            //FVector SceneMin(+FLT_MAX, +FLT_MAX, +FLT_MAX);
            //FVector SceneMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            //for (auto MeshComp : Context.StaticMeshes)
            //{
            //    if (!MeshComp || !MeshComp->IsVisible()) continue;
            //    FVector aabbMin, aabbMax;
            //    MeshComp->GetWorldAABB(aabbMin, aabbMax);
            //    SceneMin.X = std::min(SceneMin.X, aabbMin.X);
            //    SceneMin.Y = std::min(SceneMin.Y, aabbMin.Y);
            //    SceneMin.Z = std::min(SceneMin.Z, aabbMin.Z);
            //    SceneMax.X = std::max(SceneMax.X, aabbMax.X);
            //    SceneMax.Y = std::max(SceneMax.Y, aabbMax.Y);
            //    SceneMax.Z = std::max(SceneMax.Z, aabbMax.Z);
            //}
            //UE_LOG_INFO("[LVP] World AABB Min(%.2f, %.2f, %.2f) Max(%.2f, %.2f, %.2f)", SceneMin.X, SceneMin.Y, SceneMin.Z, SceneMax.X, SceneMax.Y, SceneMax.Z);
        
            // Shadow Map DSV 설정 및 클리어
            ID3D11DepthStencilView* ShadowDSV = Renderer.GetDeviceResources()->GetDirectionalShadowMapDSV();
            ID3D11RenderTargetView* ShadowRTV = Renderer.GetDeviceResources()->GetDirectionalShadowMapColorRTV();
            // Unbind SRV from PS slot to avoid read-write hazard when binding RTV
            ID3D11ShaderResourceView* NullSRV = nullptr;
            DeviceContext->PSSetShaderResources(10, 1, &NullSRV);
            DeviceContext->OMSetRenderTargets(1, &ShadowRTV, ShadowDSV);  // 색상 정보는 ShadowRTV에, 깊이 정보는 ShadowDSV에 기록, GPU는 두개의 목적지를 모두 출력 대상으로 인식
            const float ClearMoments[4] = { 1.0f, 1.0f, 0.0f, 0.0f }; // VSM Default
            DeviceContext->ClearRenderTargetView(ShadowRTV, ClearMoments);
            DeviceContext->ClearDepthStencilView(ShadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
        
                // Shadow Map Viewport 설정
                DeviceContext->RSSetViewports(1, &DirectionalShadowViewport);
        
        
            // === (1) 카메라 행렬/파라미터 ===
            UCamera* MainCamera = Context.CurrentCamera;
            FMatrix CameraView = MainCamera->GetCameraViewMatrix();
            FMatrix CameraProj = MainCamera->GetCameraProjectionMatrix();
            FVector CameraPos = MainCamera->GetLocation();
            FVector CameraForward = MainCamera->GetForward();
            float Near = MainCamera->GetNearZ();
            float Far = MainCamera->GetFarZ();
            float FovY = MainCamera->GetFovY();
            float Aspect = MainCamera->GetAspect();

            //// === Simple Ortho Shadow ===
            //FVector LightPosition = (SceneMin + SceneMax) / 2.0f;
            //LightPosition.Z += 200.0f;
            //
            //// Directional Light의 fwd만으로 정규 직교 기저 구성
            //FVector LightFwd = Light->GetForwardVector();
            //FVector LightUp = (fabsf(LightFwd.Z) > 0.99f) ? FVector(1,0,0) : FVector(0,0,1);
            //FVector LightRight = LightUp.Cross(LightFwd).GetNormalized();
            //LightUp = LightFwd.Cross(LightRight);
        
            /*FMatrix LightView = FMatrix::Identity();
            LightView.Data[0][0] = LightRight.X; LightView.Data[0][1] = LightUp.X; LightView.Data[0][2] = LightFwd.X;
            LightView.Data[1][0] = LightRight.Y; LightView.Data[1][1] = LightUp.Y; LightView.Data[1][2] = LightFwd.Y;
            LightView.Data[2][0] = LightRight.Z; LightView.Data[2][1] = LightUp.Z; LightView.Data[2][2] = LightFwd.Z;
            LightView.Data[3][0] = -LightPosition.Dot(LightRight);
            LightView.Data[3][1] = -LightPosition.Dot(LightUp);
            LightView.Data[3][2] = -LightPosition.Dot(LightFwd);
            LightView.Data[3][3] = 1.0f;

            float oMinX=+FLT_MAX, oMinY=+FLT_MAX, oMinZ=+FLT_MAX;
            float oMaxX=-FLT_MAX, oMaxY=-FLT_MAX, oMaxZ=-FLT_MAX;
            FVector Corners[8] =
            {
                {SceneMin.X, SceneMin.Y, SceneMin.Z}, {SceneMax.X, SceneMin.Y, SceneMin.Z},
                {SceneMin.X, SceneMax.Y, SceneMin.Z}, {SceneMax.X, SceneMax.Y, SceneMin.Z},
                {SceneMin.X, SceneMin.Y, SceneMax.Z}, {SceneMax.X, SceneMin.Y, SceneMax.Z},
                {SceneMin.X, SceneMax.Y, SceneMax.Z}, {SceneMax.X, SceneMax.Y, SceneMax.Z}
            };
            for (int i=0; i<8; ++i)
            {
                FVector4 lv = FMatrix::VectorMultiply(FVector4(Corners[i], 1), LightView);
                oMinX = std::min(oMinX, lv.X); oMinY = std::min(oMinY, lv.Y); oMinZ = std::min(oMinZ, lv.Z);
                oMaxX = std::max(oMaxX, lv.X); oMaxY = std::max(oMaxY, lv.Y); oMaxZ = std::max(oMaxZ, lv.Z);
            }*/
            
            //// row-vector Ortho LH
            //auto OrthoRowLH = [](float l,float r,float b,float t,float zn,float zf){
            //    FMatrix M = FMatrix::Identity();
            //    M.Data[0][0] =  2.0f/(r-l);
            //    M.Data[1][1] =  2.0f/(t-b);
            //    M.Data[2][2] =  1.0f/(zf-zn);
            //    M.Data[3][0] =  (l+r)/(l-r);
            //    M.Data[3][1] =  (t+b)/(b-t);
            //    M.Data[3][2] =  -zn/(zf-zn);
            //    return M;
            //};
            //
            //// ★ LVP에도 Texel Snapping 적용
            //// World AABB 중심을 light view space에서 texel grid에 정렬
            //float worldUnitsPerTexelX = (oMaxX - oMinX) / DirectionalShadowViewport.Width;
            //float worldUnitsPerTexelY = (oMaxY - oMinY) / DirectionalShadowViewport.Height;

            //// World AABB 중심을 light view space로 변환
            //FVector worldCenter = (SceneMin + SceneMax) * 0.5f;
            //FVector4 centerInLightView = FMatrix::VectorMultiply(FVector4(worldCenter, 1.0f), LightView);

            //// Texel grid에 snap
            //float snappedCenterX = floor(centerInLightView.X / worldUnitsPerTexelX) * worldUnitsPerTexelX;
            //float snappedCenterY = floor(centerInLightView.Y / worldUnitsPerTexelY) * worldUnitsPerTexelY;

            //// Offset 계산 및 적용
            //float offsetX = snappedCenterX - centerInLightView.X;
            //float offsetY = snappedCenterY - centerInLightView.Y;

            //oMinX += offsetX;
            //oMaxX += offsetX;
            //oMinY += offsetY;
            //oMaxY += offsetY;

            //FMatrix LightProj = OrthoRowLH(oMinX, oMaxX, oMinY, oMaxY, oMinZ, oMaxZ);


            // === 헬퍼 ===
            auto NormalizeSafe = [](const FVector& v, float eps = 1e-6f){
                float len = v.Length();
                return (len > eps) ? (v / len) : FVector(0,0,0);
            };

            // row-vector LookAt (LH)
            auto BuildLookAtRowLH = [](const FVector& Eye, const FVector& At, const FVector& UpHint)
            {
                // === 헬퍼 ===
                auto NormalizeSafe = [](const FVector& v, float eps = 1e-6f){
                    float len = v.Length();
                    return (len > eps) ? (v / len) : FVector(0,0,0);
                };
            
                FVector Fwd = NormalizeSafe(At - Eye);
                FVector Up  = UpHint;
                if (fabsf(Fwd.Dot(Up)) > 0.99f) Up = (fabsf(Fwd.Z) > 0.9f) ? FVector(1,0,0) : FVector(0,0,1);
        
                FVector Right = NormalizeSafe(Up.Cross(Fwd));
                Up = Fwd.Cross(Right);
        
                FMatrix M = FMatrix::Identity();
                // 회전부(행벡터 기준)
                M.Data[0][0]=Right.X; M.Data[0][1]=Up.X; M.Data[0][2]=Fwd.X;
                M.Data[1][0]=Right.Y; M.Data[1][1]=Up.Y; M.Data[1][2]=Fwd.Y;
                M.Data[2][0]=Right.Z; M.Data[2][1]=Up.Z; M.Data[2][2]=Fwd.Z;
                // 평행이동(행벡터)
                M.Data[3][0]= -Eye.Dot(Right);
                M.Data[3][1]= -Eye.Dot(Up);
                M.Data[3][2]= -Eye.Dot(Fwd);
                M.Data[3][3]=  1.0f;
                return M;
            };
        
            // row-vector Ortho LH
            auto OrthoRowLH = [](float l, float r, float b, float t, float zn, float zf) {
                FMatrix M = FMatrix::Identity();
                M.Data[0][0] = 2.0f / (r - l);
                M.Data[1][1] = 2.0f / (t - b);
                M.Data[2][2] = 1.0f / (zf - zn);
                M.Data[3][0] = (l + r) / (l - r);
                M.Data[3][1] = (t + b) / (b - t);
                M.Data[3][2] = -zn / (zf - zn);
                return M;
                };
        
            // (1) 현재 프레임 수신자들의 NDC 박스 구하기
            auto ComputeReceiverNDCBox = [&](FVector& ndcMin, FVector& ndcMax)
            {
                const float BIG = 1e30f;
                ndcMin = FVector(+BIG,+BIG,+BIG);
                ndcMax = FVector(-BIG,-BIG,-BIG);

                auto Accum = [&](const FVector& wmin, const FVector& wmax)
                {
                    FVector ws[8]={
                        {wmin.X,wmin.Y,wmin.Z},{wmax.X,wmin.Y,wmin.Z},
                        {wmin.X,wmax.Y,wmin.Z},{wmax.X,wmax.Y,wmin.Z},
                        {wmin.X,wmin.Y,wmax.Z},{wmax.X,wmin.Y,wmax.Z},
                        {wmin.X,wmax.Y,wmax.Z},{wmax.X,wmax.Y,wmax.Z}
                    };
                    for(int i=0;i<8;++i){
                        FVector4 v = FMatrix::VectorMultiply(FVector4(ws[i],1.0f), CameraView);
                        v = FMatrix::VectorMultiply(v, CameraProj);     // clip
                        if (v.W <= 0.0f) continue;                      // 카메라 뒤는 제외(필요시 보강)
                        FVector ndc(v.X/v.W, v.Y/v.W, v.Z/v.W);         // D3D: z∈[0,1]
                        // 살짝 가드밴드
                        if (ndc.X < -1.2f || ndc.X > 1.2f) continue;
                        if (ndc.Y < -1.2f || ndc.Y > 1.2f) continue;
                        if (ndc.Z <  0.0f || ndc.Z > 1.2f) continue;
                        ndcMin.X = std::min(ndcMin.X, ndc.X);
                        ndcMin.Y = std::min(ndcMin.Y, ndc.Y);
                        ndcMin.Z = std::min(ndcMin.Z, ndc.Z);
                        ndcMax.X = std::max(ndcMax.X, ndc.X);
                        ndcMax.Y = std::max(ndcMax.Y, ndc.Y);
                        ndcMax.Z = std::max(ndcMax.Z, ndc.Z);
                    }
                };

                for (auto MeshComp : Context.StaticMeshes)
                {
                    if (!MeshComp || !MeshComp->IsVisible()) continue;
                    FVector a,b; MeshComp->GetWorldAABB(a,b);
                    Accum(a,b);
                }

                if (ndcMin.X >  1e29f) { // 유효샘플 없을 때
                    ndcMin = FVector(-1,-1,0);
                    ndcMax = FVector( 1, 1,1);
                }

                const float mxy=0.02f, mz=0.001f;
                ndcMin.X = std::max(-1.0f, ndcMin.X - mxy);
                ndcMin.Y = std::max(-1.0f, ndcMin.Y - mxy);
                ndcMax.X = std::min( 1.0f, ndcMax.X + mxy);
                ndcMax.Y = std::min( 1.0f, ndcMax.Y + mxy);
                ndcMin.Z = std::max( 0.0f, ndcMin.Z - mz);
                ndcMax.Z = std::min( 1.0f, ndcMax.Z + mz);

                // ★ NDC Box Quantization으로 카메라 미세 움직임 무시
                // NDC 좌표를 0.1 단위로 snap하여 temporal stability 향상
                const float quantStep = 0.1f;  // 조정 가능: 0.05 ~ 0.2
                auto Quantize = [](float value, float step) {
                    return floor(value / step) * step;
                };

                ndcMin.X = Quantize(ndcMin.X, quantStep);
                ndcMin.Y = Quantize(ndcMin.Y, quantStep);
                ndcMin.Z = Quantize(ndcMin.Z, quantStep * 0.1f);  // Z는 더 정밀하게

                // Max는 ceiling (올림)
                ndcMax.X = Quantize(ndcMax.X + quantStep, quantStep);
                ndcMax.Y = Quantize(ndcMax.Y + quantStep, quantStep);
                ndcMax.Z = Quantize(ndcMax.Z + quantStep * 0.1f, quantStep * 0.1f);

                // 범위 체크
                ndcMin.X = std::max(-1.0f, ndcMin.X);
                ndcMin.Y = std::max(-1.0f, ndcMin.Y);
                ndcMax.X = std::min( 1.0f, ndcMax.X);
                ndcMax.Y = std::min( 1.0f, ndcMax.Y);
            };
        
            // (2) NDC 박스를 광원 방향으로 본 라이트 V_L′, P_L′ 생성
            // n, f 값도 반환하도록 수정 (texel snapping에서 사용)
            auto FitLightToNDCBox = [&](const FVector& LightDirWorld,
                                        const FVector& ndcMin, const FVector& ndcMax,
                                        FMatrix& OutV, FMatrix& OutP, FVector4& OutLTRB,
                                        float& OutNear, float& OutFar)
            {
                // 월드→뷰 방향 변환 (행벡터, w=0)
                FVector Lv = NormalizeSafe( FMatrix::VectorMultiply(FVector4(LightDirWorld,0.0f), CameraView).XYZ() );

                FVector center = (ndcMin + ndcMax) * 0.5f;
                float   diag   = (ndcMax - ndcMin).Length();
                float   eyeDist= std::max(0.25f, diag);
                FVector Eye    = center - Lv * eyeDist;
                FVector At     = center;

                FMatrix Vlp = BuildLookAtRowLH(Eye, At, FVector(0,0,1));

                // NDC 박스 8코너를 Vlp로 변환 후 직교 경계 산출
                float l=+FLT_MAX, b=+FLT_MAX, n=+FLT_MAX;
                float r=-FLT_MAX, t=-FLT_MAX, f=-FLT_MAX;
                FVector c[8]={
                    {ndcMin.X,ndcMin.Y,ndcMin.Z},{ndcMax.X,ndcMin.Y,ndcMin.Z},
                    {ndcMin.X,ndcMax.Y,ndcMin.Z},{ndcMax.X,ndcMax.Y,ndcMin.Z},
                    {ndcMin.X,ndcMin.Y,ndcMax.Z},{ndcMax.X,ndcMin.Y,ndcMax.Z},
                    {ndcMin.X,ndcMax.Y,ndcMax.Z},{ndcMax.X,ndcMax.Y,ndcMax.Z}
                };
                for(int i=0;i<8;++i){
                    FVector4 v = FMatrix::VectorMultiply(FVector4(c[i],1.0f), Vlp); // NDC→light-view
                    l = std::min(l, v.X); r = std::max(r, v.X);
                    b = std::min(b, v.Y); t = std::max(t, v.Y);
                    n = std::min(n, v.Z); f = std::max(f, v.Z);
                }


                // pad 포함 전 경계
                OutLTRB = FVector4(l, r, b, t);

                // 소량 패딩
                const float pad = 0.001f;
                OutV = Vlp;
                OutNear = n - pad;
                OutFar = f + pad;
                //OutP = OrthoRowLH(l, r, b, t, std::max(0.0f, n - pad), f + pad);
                OutP = OrthoRowLH(l, r, b, t, OutNear, OutFar);
                OutLTRB = FVector4(l, r, b, t);
            };

            // (3) World-Space Texel Snapping으로 Temporal Stability 개선
            // Light view space의 frustum 중심을 texel grid에 정렬
            auto ApplyTexelSnappingToLightView = [&](float& InOutL, float& InOutR,
                                                      float& InOutB, float& InOutT,
                                                      float shadowMapWidth, float shadowMapHeight)
            {
                // 1. World units per texel 계산 (light view space 기준)
                float worldUnitsPerTexelX = (InOutR - InOutL) / shadowMapWidth;
                float worldUnitsPerTexelY = (InOutT - InOutB) / shadowMapHeight;

                // 2. Light view space frustum의 중심 계산
                float centerX = (InOutL + InOutR) * 0.5f;
                float centerY = (InOutB + InOutT) * 0.5f;

                // 3. 중심을 texel grid에 snap
                float snappedCenterX = floor(centerX / worldUnitsPerTexelX) * worldUnitsPerTexelX;
                float snappedCenterY = floor(centerY / worldUnitsPerTexelY) * worldUnitsPerTexelY;

                // 4. Offset 계산
                float offsetX = snappedCenterX - centerX;
                float offsetY = snappedCenterY - centerY;

                // 5. Frustum 경계를 offset만큼 이동
                InOutL += offsetX;
                InOutR += offsetX;
                InOutB += offsetY;
                InOutT += offsetY;
            };
            
            // bCastShadows에 따라 분기
            // === LVP (Shadow Maps) ===
            if (!Light->GetCastShadows())
            {
                //UE_LOG("LVP");
                // ★ Texel Snapping은 이미 위(line 182-204)에서 적용됨

                FShadowMapConstants LVPShadowMap;
                //LVPShadowMap.EyeView   = FMatrix::Identity();
                //LVPShadowMap.EyeProj   = FMatrix::Identity();
                //LVPShadowMap.EyeViewProjInv  = FMatrix::Identity(); // LVP에선 안 씀

                //LVPShadowMap.LightViewP[0] = LightView;
                //LVPShadowMap.LightProjP[0] = LightProj;
                //LVPShadowMap.LightViewPInv[0] = LightView.Inverse(); // L_texel 계산에 안 쓰지만 채워두면 안전

                //LVPShadowMap.ShadowParams = FVector4(0.002f, 0, 0, 0);

                //LVPShadowMap.LightDirWS      = (-Light->GetForwardVector()).GetNormalized();
                //LVPShadowMap.bInvertedLight = 0;

                //// 오쏘 경계(l,r,b,t)와 섀도맵 해상도
                //LVPShadowMap.LightOrthoParams= FVector4(oMinX, oMaxX, oMinY, oMaxY);
                //LVPShadowMap.ShadowMapSize   = FVector2(DirectionalShadowViewport.Width, DirectionalShadowViewport.Height);

                //LVPShadowMap.bUsePSM = 0;
                //FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, LVPShadowMap);

                //LightViewP = LightView;
                //LightProjP = LightProj;
                //CachedEyeView = FMatrix::Identity();
                //CachedEyeProj = FMatrix::Identity();

                //LightOrthoLTRB = FVector4(oMinX, oMaxX, oMinY, oMaxY);
            }
            else
            {
                //UE_LOG("PSM");
                // 시작값
                FVector ndcMin, ndcMax;
                ComputeReceiverNDCBox(ndcMin, ndcMax);

                FMatrix V_L_prime, P_L_prime;
                FVector4 orthoLTRB;
                float nearPlane, farPlane;
                FitLightToNDCBox(Light->GetForwardVector(), ndcMin, ndcMax, V_L_prime, P_L_prime, orthoLTRB, nearPlane, farPlane);

                // ★ Texel Snapping 적용 (Temporal Stability 개선)
                // Light view space frustum을 texel grid에 정렬
                float l = orthoLTRB.X;
                float r = orthoLTRB.Y;
                float b = orthoLTRB.Z;
                float t = orthoLTRB.W;

                ApplyTexelSnappingToLightView(l, r, b, t,
                                             DirectionalShadowViewport.Width, DirectionalShadowViewport.Height);

                // Snapped l/r/b/t로 Ortho projection 재생성
                // near/far는 FitLightToNDCBox에서 계산된 값 사용
                P_L_prime = OrthoRowLH(l, r, b, t, nearPlane, farPlane);
                orthoLTRB = FVector4(l, r, b, t);

                // 행벡터 합성/역행렬: (V_e P_e)^(-1) = P_e^{-1} * V_e^{-1}
                FMatrix EyeViewInv      = CameraView.Inverse();
                FMatrix EyeProjInv      = CameraProj.Inverse();
                FMatrix EyeViewProjInv  = EyeProjInv * EyeViewInv;

                // 라이트 뷰 역행렬 (Texel snapping 이후 재계산)
                FMatrix LightViewPInv   = V_L_prime.Inverse();

                // 섀도맵 해상도 (뷰포트 또는 텍스처 크기에서 가져와도 됨)
                float sx = DirectionalShadowViewport.Width;
                float sy = DirectionalShadowViewport.Height;

                // 바이어스 + 라이트 방향(표면→광원)
                //float A = Light->GetShadowBias();
                //float b = Light->GetShadowSlopeBias();
                FVector LdirWS = -Light->GetForwardVector(); // 라이트 광선 방향이 +Fwd라면, 표면→광원은 -Fwd
                LdirWS = LdirWS.GetNormalized();

                // === PSM (Perspective Shadow Maps) ===
                FShadowMapConstants PSM;
                PSM.EyeView   = CameraView;
                PSM.EyeProj   = CameraProj;
                PSM.EyeViewProjInv  = EyeViewProjInv;

                PSM.LightViewP[0] = V_L_prime;
                PSM.LightProjP[0] = P_L_prime;
                PSM.LightViewPInv[0] = LightViewPInv;

                
                PSM.ShadowParams = FVector4(Light->GetShadowBias(), Light->GetShadowSlopeBias(), 0, 0);
             
                PSM.LightDirWS      = LdirWS;
                PSM.bInvertedLight = 0;

                PSM.LightOrthoParams= orthoLTRB;             // (l, r, b, t)
                PSM.ShadowMapSize   = FVector2(sx, sy);
            
                PSM.bUsePSM = 1;
                FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, PSM);
            
                LightViewP = V_L_prime;
                LightProjP = P_L_prime;
                CachedEyeView = CameraView;
                CachedEyeProj = CameraProj;

                LightOrthoLTRB = orthoLTRB;
            }



            // 파이프라인: b0=Model, b6=PSM, PS 필수(depth write를 위해)
            FPipelineInfo Pipe = {
                ShadowMapInputLayout,
                ShadowMapVS,
                FRenderResourceFactory::GetRasterizerState({ECullMode::None, EFillMode::Solid}), // Cull OFF to avoid winding flip in PSM space
                URenderer::GetInstance().GetDefaultDepthStencilState(),
                ShadowMapPS,  // ★ PS 바인드 필수 (비어있어도 Depth write 활성화)
                nullptr,
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
            };
            Pipeline->UpdatePipeline(Pipe);
            Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);
            Pipeline->SetConstantBuffer(6, EShaderType::VS, PSMConstantBuffer);
        
        
        
      
        
            // 모든 Static Mesh를 Light 관점에서 렌더링
            for (auto MeshComp : Context.StaticMeshes)
            {
                if (!MeshComp || !MeshComp->IsVisible()) continue;
                RenderPrimitive(MeshComp);
            }
        
            // 현재는 하나의 Directional Light만 처리 (나중에 여러 개 지원 가능)
            break;
        }
    }

    // Shadow Map DSV를 unbind (다음 pass에서 SRV로 사용하기 위해)
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    
    // Viewport 복원
    DeviceContext->RSSetViewports(1, &OriginalViewport);
    
    // 원본 Render Targets 복원
    DeviceContext->OMSetRenderTargets(1, &OriginalRTVs, OriginalDSV);
    
    // OMGetRenderTargets가 AddRef를 호출했으므로 Release 필요
    //for (int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    //{
    //    if (OriginalRTVs[i]) OriginalRTVs[i]->Release();
    //}
    //if (OriginalDSV) OriginalDSV->Release();
}

void FUpdateLightBufferPass::RenderPrimitive(UStaticMeshComponent* MeshComp)
{
    if (!MeshComp || !MeshComp->GetStaticMesh()) return;

    FStaticMesh* MeshAsset = MeshComp->GetStaticMesh()->GetStaticMeshAsset();
    if (!MeshAsset) return;
    
    Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);
    Pipeline->SetConstantBuffer(6, EShaderType::VS, PSMConstantBuffer);  // Light 전용 버퍼 사용

    // Mesh 렌더링
    Pipeline->SetVertexBuffer(MeshComp->GetVertexBuffer(), sizeof(FNormalVertex));
    Pipeline->SetIndexBuffer(MeshComp->GetIndexBuffer(), 0);
    
    // World Transform 업데이트
    FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, MeshComp->GetWorldTransformMatrix());
    
    // Draw
    for (const FMeshSection& Section : MeshAsset->Sections)
    {
        Pipeline->DrawIndexed(Section.IndexCount, Section.StartIndex, 0);
    }
}

void FUpdateLightBufferPass::CalculateShadowMatrices(EShadowProjectionType ProjType, FRenderingContext& Context, FShadowCalculationData& OutShadowData)
{
    OutShadowData.LightViews.clear();
    OutShadowData.LightProjs.clear();

    UDirectionalLightComponent* Light = Context.DirectionalLights.empty() ? nullptr : Context.DirectionalLights[0];
    if (!Light) return;

    switch (ProjType)
    {
    case EShadowProjectionType::Default:
    {
        // === LVP용: 월드 전체 AABB 집계 (카메라에 독립)
        FVector SceneMin(+FLT_MAX, +FLT_MAX, +FLT_MAX);
        FVector SceneMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (auto MeshComp : Context.StaticMeshes)
        {
            if (!MeshComp || !MeshComp->IsVisible()) continue;
            FVector aabbMin, aabbMax;
            MeshComp->GetWorldAABB(aabbMin, aabbMax);
            SceneMin.X = std::min(SceneMin.X, aabbMin.X);
            SceneMin.Y = std::min(SceneMin.Y, aabbMin.Y);
            SceneMin.Z = std::min(SceneMin.Z, aabbMin.Z);
            SceneMax.X = std::max(SceneMax.X, aabbMax.X);
            SceneMax.Y = std::max(SceneMax.Y, aabbMax.Y);
            SceneMax.Z = std::max(SceneMax.Z, aabbMax.Z);
        }
        //UE_LOG_INFO("[LVP] World AABB Min(%.2f, %.2f, %.2f) Max(%.2f, %.2f, %.2f)", SceneMin.X, SceneMin.Y, SceneMin.Z, SceneMax.X, SceneMax.Y, SceneMax.Z);

        // === Simple Ortho Shadow ===
        FVector LightPosition = (SceneMin + SceneMax) / 2.0f;
        LightPosition.Z += 200.0f;

        // Directional Light의 fwd만으로 정규 직교 기저 구성
        FVector LightFwd = Light->GetForwardVector();
        FVector LightUp = (fabsf(LightFwd.Z) > 0.99f) ? FVector(1, 0, 0) : FVector(0, 0, 1);
        FVector LightRight = LightUp.Cross(LightFwd).GetNormalized();
        LightUp = LightFwd.Cross(LightRight);

        FMatrix LightView = FMatrix::Identity();
        LightView.Data[0][0] = LightRight.X; LightView.Data[0][1] = LightUp.X; LightView.Data[0][2] = LightFwd.X;
        LightView.Data[1][0] = LightRight.Y; LightView.Data[1][1] = LightUp.Y; LightView.Data[1][2] = LightFwd.Y;
        LightView.Data[2][0] = LightRight.Z; LightView.Data[2][1] = LightUp.Z; LightView.Data[2][2] = LightFwd.Z;
        LightView.Data[3][0] = -LightPosition.Dot(LightRight);
        LightView.Data[3][1] = -LightPosition.Dot(LightUp);
        LightView.Data[3][2] = -LightPosition.Dot(LightFwd);
        LightView.Data[3][3] = 1.0f;

        float oMinX = +FLT_MAX, oMinY = +FLT_MAX, oMinZ = +FLT_MAX;
        float oMaxX = -FLT_MAX, oMaxY = -FLT_MAX, oMaxZ = -FLT_MAX;
        FVector Corners[8] =
        {
            {SceneMin.X, SceneMin.Y, SceneMin.Z}, {SceneMax.X, SceneMin.Y, SceneMin.Z},
            {SceneMin.X, SceneMax.Y, SceneMin.Z}, {SceneMax.X, SceneMax.Y, SceneMin.Z},
            {SceneMin.X, SceneMin.Y, SceneMax.Z}, {SceneMax.X, SceneMin.Y, SceneMax.Z},
            {SceneMin.X, SceneMax.Y, SceneMax.Z}, {SceneMax.X, SceneMax.Y, SceneMax.Z}
        };
        for (int i = 0; i < 8; ++i)
        {
            FVector4 lv = FMatrix::VectorMultiply(FVector4(Corners[i], 1), LightView);
            oMinX = std::min(oMinX, lv.X); oMinY = std::min(oMinY, lv.Y); oMinZ = std::min(oMinZ, lv.Z);
            oMaxX = std::max(oMaxX, lv.X); oMaxY = std::max(oMaxY, lv.Y); oMaxZ = std::max(oMaxZ, lv.Z);
        }

        // row-vector Ortho LH
        auto OrthoRowLH = [](float l, float r, float b, float t, float zn, float zf) {
            FMatrix M = FMatrix::Identity();
            M.Data[0][0] = 2.0f / (r - l);
            M.Data[1][1] = 2.0f / (t - b);
            M.Data[2][2] = 1.0f / (zf - zn);
            M.Data[3][0] = (l + r) / (l - r);
            M.Data[3][1] = (t + b) / (b - t);
            M.Data[3][2] = -zn / (zf - zn);
            return M;
        };

        // ★ LVP에도 Texel Snapping 적용
        // World AABB 중심을 light view space에서 texel grid에 정렬
        float worldUnitsPerTexelX = (oMaxX - oMinX) / DirectionalShadowViewport.Width;
        float worldUnitsPerTexelY = (oMaxY - oMinY) / DirectionalShadowViewport.Height;

        // World AABB 중심을 light view space로 변환
        FVector worldCenter = (SceneMin + SceneMax) * 0.5f;
        FVector4 centerInLightView = FMatrix::VectorMultiply(FVector4(worldCenter, 1.0f), LightView);

        // Texel grid에 snap
        float snappedCenterX = floor(centerInLightView.X / worldUnitsPerTexelX) * worldUnitsPerTexelX;
        float snappedCenterY = floor(centerInLightView.Y / worldUnitsPerTexelY) * worldUnitsPerTexelY;

        // Offset 계산 및 적용
        float offsetX = snappedCenterX - centerInLightView.X;
        float offsetY = snappedCenterY - centerInLightView.Y;

        oMinX += offsetX;
        oMaxX += offsetX;
        oMinY += offsetY;
        oMaxY += offsetY;

        FMatrix LightProj = OrthoRowLH(oMinX, oMaxX, oMinY, oMaxY, oMinZ, oMaxZ);

        OutShadowData.LightOrthoParams = FVector4(oMinX, oMaxX, oMinY, oMaxY);
        OutShadowData.LightViews.push_back(LightView);
        OutShadowData.LightProjs.push_back(LightProj);

        LightViewP = LightView;
        LightProjP = LightProj;
        CachedEyeView = FMatrix::Identity();
        CachedEyeProj = FMatrix::Identity();
        LightOrthoLTRB = FVector4(oMinX, oMaxX, oMinY, oMaxY);
        
        break;
    }
    case EShadowProjectionType::PSM:
        break;
    case EShadowProjectionType::CSM:
    {
        const UCamera* Camera = Context.CurrentCamera;
        if (!Camera) return;

        CalculateCascadeSplits(OutShadowData.CascadeSplits, Camera);
        CascadedShadowMapConstants.CascadeSplits = OutShadowData.CascadeSplits;
        const float* pSplits = &OutShadowData.CascadeSplits.X;

        for (int i = 0; i < MAX_CASCADES; i++)
        {
            // Calculate current cascade slices' corner
            float NearSplit = (i == 0) ? Camera->GetNearZ() : pSplits[i - 1];
            float FarSplit = pSplits[i];
            FVector FrustumCorners[8];
            Camera->GetFrustumCorners(FrustumCorners, NearSplit, FarSplit);

            // Calculate the center of cascade slice
            FVector FrustumCenter = FVector::ZeroVector();
            for (int j = 0; j < 8; j++)
            {
                FrustumCenter += FrustumCorners[j];
            }
            FrustumCenter /= 8.0f;

            // Calculate the light's View matrix based on the frustum's center point 
            FMatrix LightViewMatrix;
            {
                // Set light position
                FVector LightDir = Light->GetForwardVector().GetNormalized();
                float ShadowDistance = 200.0f;
                /*float CameraRange = Camera->GetFarZ() - Camera->GetNearZ();
                float ShadowDistance = std::min(500.0f, CameraRange * 0.5f);*/
                FVector LightPos = FrustumCenter - LightDir * ShadowDistance;

                // light source targets the center of slice
                FVector TargetPos = FrustumCenter;
                FVector UpVector = (abs(LightDir.Z) > 0.99f) ? FVector(0, 1, 0) : FVector(0, 0, 1);

                // LookAt matrix
                FVector ZAxis = (TargetPos - LightPos).GetNormalized();
                FVector XAxis = (UpVector.Cross(ZAxis)).GetNormalized();
                FVector YAxis = ZAxis.Cross(XAxis);

                LightViewMatrix = FMatrix::Identity();
                LightViewMatrix.Data[0][0] = XAxis.X;   LightViewMatrix.Data[1][0] = XAxis.Y;   LightViewMatrix.Data[2][0] = XAxis.Z;
                LightViewMatrix.Data[0][1] = YAxis.X;   LightViewMatrix.Data[1][1] = YAxis.Y;   LightViewMatrix.Data[2][1] = YAxis.Z;
                LightViewMatrix.Data[0][2] = ZAxis.X;   LightViewMatrix.Data[1][2] = ZAxis.Y;   LightViewMatrix.Data[2][2] = ZAxis.Z;
                LightViewMatrix.Data[3][0] = -XAxis.FVector::Dot(LightPos);
                LightViewMatrix.Data[3][1] = -YAxis.FVector::Dot(LightPos);
                LightViewMatrix.Data[3][2] = -ZAxis.FVector::Dot(LightPos);
            }

            // Calculate a tight Projection matrix
            FMatrix CascadeLightProj;
            {
                FVector FrustumCornersLightView[8];
                for (int j = 0; j < 8; j++)
                {
                    FrustumCornersLightView[j] = FVector4(FrustumCorners[j], 1.0f) * LightViewMatrix;
                }

                FVector MinVec = FrustumCornersLightView[0];
                FVector MaxVec = FrustumCornersLightView[0];
                for (int j = 0; j < 8; j++)
                {
                    MinVec.X = std::min(MinVec.X, FrustumCornersLightView[j].X);
                    MinVec.Y = std::min(MinVec.Y, FrustumCornersLightView[j].Y);
                    MinVec.Z = std::min(MinVec.Z, FrustumCornersLightView[j].Z);
                    MaxVec.X = std::max(MaxVec.X, FrustumCornersLightView[j].X);
                    MaxVec.Y = std::max(MaxVec.Y, FrustumCornersLightView[j].Y);
                    MaxVec.Z = std::max(MaxVec.Z, FrustumCornersLightView[j].Z);
                }

                // +-+-+ Texel Snapping (prevents shadow shimmering) +-+-+
                {
                    const float ShadowMapResolution = DirectionalShadowViewport.Width;
                    // stabilize shadow map edges
                    float worldUnitsPerTexelX = (MaxVec.X - MinVec.X) / ShadowMapResolution;
                    float worldUnitsPerTexelY = (MaxVec.Y - MinVec.Y) / ShadowMapResolution;

                    // Snap based on the center
                    FVector Center = (MinVec + MaxVec) * 0.5f;

                    // Snap the center coordinates to texel units in light view space
                    Center.X = std::floor(Center.X / worldUnitsPerTexelX) * worldUnitsPerTexelX;
                    Center.Y = std::floor(Center.Y / worldUnitsPerTexelY) * worldUnitsPerTexelY;

                    // Recalculate min/max based on the snapped center
                    const float halfRangeX = (MaxVec.X - MinVec.X) * 0.5f;
                    const float halfRangeY = (MaxVec.Y - MinVec.Y) * 0.5f;
                    MinVec.X = Center.X - halfRangeX;
                    MinVec.Y = Center.Y - halfRangeY;
                    MaxVec.X = Center.X + halfRangeX;
                    MaxVec.Y = Center.Y + halfRangeY;
                }

                CascadeLightProj = FMatrix::Identity();
                CascadeLightProj.Data[0][0] = 2.0f / (MaxVec.X - MinVec.X);
                CascadeLightProj.Data[1][1] = 2.0f / (MaxVec.Y - MinVec.Y);
                CascadeLightProj.Data[2][2] = 1.0f / (MaxVec.Z - MinVec.Z);
                CascadeLightProj.Data[3][0] = -(MaxVec.X + MinVec.X) / (MaxVec.X - MinVec.X);
                CascadeLightProj.Data[3][1] = -(MaxVec.Y + MinVec.Y) / (MaxVec.Y - MinVec.Y);
                CascadeLightProj.Data[3][2] = -MinVec.Z / (MaxVec.Z - MinVec.Z);
            }

            CascadedShadowMapConstants.LightViewP[i] = LightViewMatrix;
            CascadedShadowMapConstants.LightProjP[i] = CascadeLightProj;

            OutShadowData.LightViews.push_back(LightViewMatrix);
            OutShadowData.LightProjs.push_back(CascadeLightProj);
        }
        break;
    }
    default:
        break;
    }
}

void FUpdateLightBufferPass::SetShadowRenderTarget(EShadowProjectionType ProjType, EShadowFilterType FilterType, int CascadeIndex)
{
    const auto& Renderer = URenderer::GetInstance();
    auto DeviceContext = Renderer.GetDeviceContext();
    
    // Always Unbind
    ID3D11ShaderResourceView* NullSRV = nullptr;
    DeviceContext->PSSetShaderResources(10, 1, &NullSRV);   // DirectionalShadowMapTexture (t10)
    DeviceContext->PSSetShaderResources(11, 1, &NullSRV);   // CascadedShadowMapTexture (t11)

    if (ProjType == EShadowProjectionType::CSM)
    {
        ID3D11DepthStencilView* CurrentDsv = Renderer.GetDeviceResources()->GetCascadedShadowMapDSV(CascadeIndex);

        // CSM + filter (get color RTV from array)
        if (FilterType == EShadowFilterType::VSM)
        {
            ID3D11RenderTargetView* CurrentRtv = Renderer.GetDeviceResources()->GetCascadedShadowMapColorRTV(CascadeIndex);
            DeviceContext->OMSetRenderTargets(1, &CurrentRtv, CurrentDsv);

            const float ClearMoments[4] = { 1.0f, 1.0f, 0.0f, 0.0f };   // VSM Default
            DeviceContext->ClearRenderTargetView(CurrentRtv, ClearMoments);
            DeviceContext->ClearDepthStencilView(CurrentDsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        }
        else  // PCF or None
        {
            DeviceContext->OMSetRenderTargets(0, nullptr, CurrentDsv);
            DeviceContext->ClearDepthStencilView(CurrentDsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        }
    }
    else
    {
        if (FilterType == EShadowFilterType::VSM)
        {
            // Set Shadow Map DSV
            ID3D11DepthStencilView* ShadowDSV = Renderer.GetDeviceResources()->GetDirectionalShadowMapDSV();
            ID3D11RenderTargetView* ShadowRTV = Renderer.GetDeviceResources()->GetDirectionalShadowMapColorRTV();
            // Unbind SRV from PS slot to avoid read-write hazard when binding RTV
            DeviceContext->OMSetRenderTargets(1, &ShadowRTV, ShadowDSV);  // 색상 정보는 ShadowRTV에, 깊이 정보는 ShadowDSV에 기록, GPU는 두개의 목적지를 모두 출력 대상으로 인식
            const float ClearMoments[4] = { 1.0f, 1.0f, 0.0f, 0.0f };   // VSM Default
            DeviceContext->ClearRenderTargetView(ShadowRTV, ClearMoments);
            DeviceContext->ClearDepthStencilView(ShadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
        }
        else  // None OR PCF 
        {
            ID3D11DepthStencilView* ShadowDSV = Renderer.GetDeviceResources()->GetDirectionalShadowMapDSV();
            DeviceContext->OMSetRenderTargets(0, nullptr, ShadowDSV);
            DeviceContext->ClearDepthStencilView(ShadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
        }
    }
    DeviceContext->RSSetViewports(1, &DirectionalShadowViewport);
}

void FUpdateLightBufferPass::UpdateShadowCasterConstants(EShadowProjectionType ProjType, const FShadowCalculationData& InShadowData, int CascadeIndex, FRenderingContext& Context)
{
    UDirectionalLightComponent* Light = Context.DirectionalLights.empty() ? nullptr : Context.DirectionalLights[0];
    if (!Light) return;

    FShadowMapConstants CasterConsts = {};

    if (ProjType == EShadowProjectionType::Default)
    {
        CasterConsts.EyeView = FMatrix::Identity();
        CasterConsts.EyeProj = FMatrix::Identity();
        CasterConsts.EyeViewProjInv = FMatrix::Identity(); // LVP에선 안 씀

        CasterConsts.LightViewP[0] = InShadowData.LightViews[0];
        CasterConsts.LightProjP[0] = InShadowData.LightProjs[0];
        CasterConsts.LightViewPInv[0] = InShadowData.LightViews[0].Inverse(); // L_texel 계산에 안 쓰지만 채워두면 안전

        //CasterConsts.CameraClipToLightClip = FMatrix::Identity();
        
        CasterConsts.ShadowParams = FVector4(0.002f, 0, 0, 0);

        CasterConsts.LightDirWS = (-Light->GetForwardVector()).GetNormalized();
        CasterConsts.bInvertedLight = 0;

        // 오쏘 경계(l,r,b,t)와 섀도맵 해상도
        CasterConsts.LightOrthoParams = InShadowData.LightOrthoParams;
        CasterConsts.ShadowMapSize = FVector2(DirectionalShadowViewport.Width, DirectionalShadowViewport.Height);
        CasterConsts.bUsePSM = 0;
    }
    else if (ProjType == EShadowProjectionType::PSM)
    {

    }
    else if (ProjType == EShadowProjectionType::CSM)
    {
        CasterConsts.bUseCSM = 1;

        CasterConsts.LightViewP[0] = InShadowData.LightViews[CascadeIndex];
        CasterConsts.LightProjP[0] = InShadowData.LightProjs[CascadeIndex];

        CasterConsts.CascadeSplits = InShadowData.CascadeSplits;
    }

    FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, CasterConsts);
}

void FUpdateLightBufferPass::CalculateCascadeSplits(FVector4& OutSplits, const UCamera* InCamera)
{
    const float NearClip = InCamera->GetNearZ();
    const float FarClip = InCamera->GetFarZ();
    const float ClipRange = FarClip - NearClip;

    const float lambda = 0.8f;

    for (int i = 0; i < MAX_CASCADES; i++)
    {
        float p = (i + 1) / static_cast<float>(MAX_CASCADES);

        // Interpolate linear split and logmetric split into lambda
        float LogSplit = NearClip * powf(FarClip / NearClip, p);
        float UniformSplit = NearClip + ClipRange * p;
        float Distance = lambda * LogSplit + (1.0f - lambda) * UniformSplit;

        switch (i)
        {
            case 0: OutSplits.X = Distance; break;
            case 1: OutSplits.Y = Distance; break;
            case 2: OutSplits.Z = Distance; break;
            case 3: OutSplits.W = Distance; break;
        }
    }
}

void FUpdateLightBufferPass::BuildSpotLightPSM(const FMatrix& EyeView, const FMatrix& EyeProj,
    const FVector& SpotLightPosWS, const FVector& SpotLightDirWS,
    float SpotOuterAngle, int ShadowMapWidth, int ShadowMapHeight,
    FMatrix& OutLightView, FMatrix& OutLightProj, FMatrix& OutPSMMatrix)
{
    // Standard PSM formula: lightView * warpPSM * spotPerspective * crop
    // 1. lightView: World → Light View Space (광원 기준)
    // 2. warpPSM: 카메라 frustum을 고려한 워프 변환
    // 3. spotPerspective: SpotLight의 원근 투영
    // 4. crop (optional): Light space에서 tight bounds
    
    // === 1. Light View Matrix (World → Light View) ===
    const FVector eye = SpotLightPosWS;
    const FVector fwd = SpotLightDirWS.GetNormalized();
    const FVector tmpUp = (fabsf(fwd.Z) > 0.99f) ? FVector(1,0,0) : FVector(0,0,1);
    const FVector right = tmpUp.Cross(fwd).GetNormalized();
    const FVector up = fwd.Cross(right);
    
    FMatrix lightView = FMatrix::Identity();
    lightView.Data[0][0]=right.X; lightView.Data[0][1]=up.X; lightView.Data[0][2]=fwd.X;
    lightView.Data[1][0]=right.Y; lightView.Data[1][1]=up.Y; lightView.Data[1][2]=fwd.Y;
    lightView.Data[2][0]=right.Z; lightView.Data[2][1]=up.Z; lightView.Data[2][2]=fwd.Z;
    lightView.Data[3][0]= -eye.Dot(right);
    lightView.Data[3][1]= -eye.Dot(up);
    lightView.Data[3][2]= -eye.Dot(fwd);
    
    
    // === 3. Warp PSM Matrix (카메라 기준 워프) ===
    // PSM 핵심: Light View Space를 카메라 방향으로 재정렬하고 원근 워프
    FMatrix EyeViewInv = EyeView.Inverse();
    FMatrix EyeProjInv = EyeProj.Inverse();
    
    // 카메라 위치를 Light View Space로 변환
    FVector4 cameraWorldPos4 = FMatrix::VectorMultiply(FVector4(0, 0, 0, 1.0f), EyeViewInv);
    FVector cameraWorldPos(cameraWorldPos4.X, cameraWorldPos4.Y, cameraWorldPos4.Z);
    FVector4 cameraLightViewPos4 = FMatrix::VectorMultiply(FVector4(cameraWorldPos, 1.0f), lightView);
    FVector cameraLightViewPos(cameraLightViewPos4.X, cameraLightViewPos4.Y, cameraLightViewPos4.Z);
    
    float distToCamera = cameraLightViewPos.Length();
    
    UE_LOG("︠ PSM Camera in Light Space: (%.2f, %.2f, %.2f), dist=%.2f",
        cameraLightViewPos.X, cameraLightViewPos.Y, cameraLightViewPos.Z, distToCamera);
    
    // WarpPSM: Light View Space를 카메라 방향 기준으로 회전한 후 원근 투영
    // 1) 카메라 방향으로 좌표계 회전 (부영한 경우 생략)
    FVector lightToCamDir = cameraLightViewPos.GetNormalized();
    
    // 2) 카메라까지의 거리를 기준으로 near/far 계산
    // n은 광원에 가까운 면, f는 카메라 넘어 씨에까지
    float n = std::max(0.5f, distToCamera * 0.1f);   // near: 광원에서 10% 거리
    float f = std::max(n * 2.0f, distToCamera * 2.0f);  // far: 카메라 너머2배 거리
    
    // 3) 원근 투영 행렬 (DirectX LH)
    // 주의: warpPSM의 FOV는 적당히 넓게 (카메라 frustum을 다 커버)
    // 너무 좌우면 aliasing 감소 효과 저하, 너무 많으면 왜곡 심함
    float warpFOV = FVector::GetDegreeToRadian(120.0f);  // 60도 (spotPerspective보다 약간 넓게)
    float warpScale = 1.0f / tanf(warpFOV * 0.5f);
    
    FMatrix warpPSM = FMatrix::Identity();
    warpPSM.Data[0][0] = warpScale;
    warpPSM.Data[1][1] = warpScale;
    warpPSM.Data[2][2] = f / (f - n);
    warpPSM.Data[3][2] = -n * f / (f - n);
    warpPSM.Data[2][3] = 1.0f;  // perspective divide 활성화
    warpPSM.Data[3][3] = 0.0f;
    
    // === 4. Camera Frustum Bounds in Warped Space ===
    // Camera frustum을 lightView * warpPSM 공간으로 변환하여 crop 계산
    FVector ndcCorners[8] = {
        {-1,-1, 0}, { 1,-1, 0}, {-1, 1, 0}, { 1, 1, 0},  // near plane
        {-1,-1, 1}, { 1,-1, 1}, {-1, 1, 1}, { 1, 1, 1}   // far plane
    };
    
    FVector minWarped(+FLT_MAX, +FLT_MAX, +FLT_MAX);
    FVector maxWarped(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    
    for (int i = 0; i < 8; ++i)
    {
        // NDC → Camera View Space
        FVector4 clipSpace = FVector4(ndcCorners[i], 1.0f);
        FVector4 viewSpace = FMatrix::VectorMultiply(clipSpace, EyeProjInv);
        if (viewSpace.W < 0 )
        {
            continue;
        }
        if (fabsf(viewSpace.W) > 1e-6f) {
            viewSpace.X /= viewSpace.W;
            viewSpace.Y /= viewSpace.W;
            viewSpace.Z /= viewSpace.W;
            viewSpace.W = 1.0f;
        }
        
        // Camera View → World Space
        FVector4 worldSpace = FMatrix::VectorMultiply(viewSpace, EyeViewInv);
        
        // World → Light View Space
        FVector4 lightViewSpace = FMatrix::VectorMultiply(worldSpace, lightView);
        
        // Light View → Warped Space (warpPSM 적용 후 perspective divide)
        FVector4 warpedClip = FMatrix::VectorMultiply(lightViewSpace, warpPSM);
        if (fabsf(warpedClip.W) > 1e-6f) {
            FVector warpedNDC(
                warpedClip.X / warpedClip.W,
                warpedClip.Y / warpedClip.W,
                warpedClip.Z / warpedClip.W
            );

            warpedNDC.X = std::max(-5.0f, std::min(5.0f, warpedNDC.X));
            warpedNDC.Y = std::max(-5.0f, std::min(5.0f, warpedNDC.Y));
            warpedNDC.Z = std::max(-1.0f, std::min(2.0f, warpedNDC.Z));
            
            minWarped.X = std::min(minWarped.X, warpedNDC.X);
            minWarped.Y = std::min(minWarped.Y, warpedNDC.Y);
            minWarped.Z = std::min(minWarped.Z, warpedNDC.Z);
            maxWarped.X = std::max(maxWarped.X, warpedNDC.X);
            maxWarped.Y = std::max(maxWarped.Y, warpedNDC.Y);
            maxWarped.Z = std::max(maxWarped.Z, warpedNDC.Z);
        }
    }
    
    UE_LOG("  PSM Warped Frustum NDC: min=(%.2f,%.2f,%.2f) max=(%.2f,%.2f,%.2f)",
        minWarped.X, minWarped.Y, minWarped.Z, maxWarped.X, maxWarped.Y, maxWarped.Z);
    
    // === 5. Crop Matrix (Warped Space에서 tight fitting) ===
    float centerX = (minWarped.X + maxWarped.X) * 0.5f;
    float centerY = (minWarped.Y + maxWarped.Y) * 0.5f;
    float scaleX = 2.0f / std::max(0.001f, maxWarped.X - minWarped.X);
    float scaleY = 2.0f / std::max(0.001f, maxWarped.Y - minWarped.Y);
    
    FMatrix crop = FMatrix::Identity();
    crop.Data[0][0] = scaleX;
    crop.Data[1][1] = scaleY;
    crop.Data[3][0] = -centerX * scaleX;
    crop.Data[3][1] = -centerY * scaleY;
    
    // === Final PSM Matrix: lightView * warpPSM * crop ===
    // warpPSM이 원근 워프를 통해 카메라 기준 해상도 재분배
    // spotPerspective는 제거하고 warpPSM만 사용 (중복 방지)
    OutLightView = lightView;
    OutLightProj = warpPSM * crop;
    OutPSMMatrix = lightView * warpPSM * crop;
}

void FUpdateLightBufferPass::Release()
{
    // 셀이더는 Renderer가 관리하므로 여기서 해제하지 않음
    ShadowMapVS = nullptr;
    ShadowMapPS = nullptr;
    ShadowMapInputLayout = nullptr;
    
    // Light Camera 상수 버퍼 해제
    SafeRelease(LightCameraConstantBuffer);
    SafeRelease(SpotShadowAtlasStructuredBuffer);
    SafeRelease(PointShadowCubeIndexSRV);
    SafeRelease(PointShadowCubeIndexStructuredBuffer);
}

