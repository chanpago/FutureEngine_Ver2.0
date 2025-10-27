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
}

FUpdateLightBufferPass::~FUpdateLightBufferPass()
{
}

void FUpdateLightBufferPass::Execute(FRenderingContext& Context)
{
    // Context에 이미 수집된 Light 컴포넌트들을 사용
    // Shadow Map 베이킹 실행
    BakeShadowMap(Context);
}

void FUpdateLightBufferPass::BakeShadowMap(FRenderingContext& Context)
{
    const auto& Renderer = URenderer::GetInstance();
    auto DeviceContext = Renderer.GetDeviceContext();
    
    // Begin refactored shadow-bake setup (clear hazards, save state)
    D3D11_VIEWPORT SavedViewport{};
    ID3D11RenderTargetView* SavedRTV = nullptr;
    ID3D11DepthStencilView* SavedDSV = nullptr;
    BeginShadowBake(DeviceContext, SavedViewport, SavedRTV, SavedDSV);
    
    // Shadow Map 렌더링용 파이프라인 설정
    if (!ShadowMapVS || !ShadowMapInputLayout)
    {
        EndShadowBake(DeviceContext, SavedViewport);
        return; // 셰이더가 없으면 스킵
    }
    
    const bool bUseCSM = (Context.ShowFlags & EEngineShowFlags::SF_CSM) != 0;
    if (bUseCSM)
    {
        RenderDirectionalCSM(Context);
    }
    else
    {
        RenderDirectionalShadowSimple(Context);
    }

    // TODO: Spotlight, point light shadow map 렌더링

    // End shadow-bake (unbind and restore viewport)
    EndShadowBake(DeviceContext, SavedViewport);
    // 원본 Render Targets 복원
    DeviceContext->OMSetRenderTargets(1, &SavedRTV, SavedDSV);
}

void FUpdateLightBufferPass::BeginShadowBake(ID3D11DeviceContext* DeviceContext, D3D11_VIEWPORT& OutOriginalViewport,
    ID3D11RenderTargetView*& OutOriginalRTV, ID3D11DepthStencilView*& OutOriginalDSV)
{
    ID3D11ShaderResourceView* NullSRV = nullptr;
    DeviceContext->PSSetShaderResources(10, 1, &NullSRV);
    UINT NumViewports = 1;
    DeviceContext->RSGetViewports(&NumViewports, &OutOriginalViewport);
    DeviceContext->OMGetRenderTargets(1, &OutOriginalRTV, &OutOriginalDSV);
}

void FUpdateLightBufferPass::EndShadowBake(ID3D11DeviceContext* DeviceContext, const D3D11_VIEWPORT& OriginalViewport)
{
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    DeviceContext->RSSetViewports(1, &OriginalViewport);
}

// === Shared small utilities ===
void FUpdateLightBufferPass::ComputeSceneWorldAABB(const FRenderingContext& Context, FVector& OutMin, FVector& OutMax) const
{
    OutMin = FVector(+FLT_MAX, +FLT_MAX, +FLT_MAX);
    OutMax = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (auto MeshComp : Context.StaticMeshes)
    {
        if (!MeshComp || !MeshComp->IsVisible()) continue;
        FVector aabbMin, aabbMax;
        MeshComp->GetWorldAABB(aabbMin, aabbMax);
        OutMin.X = std::min(OutMin.X, aabbMin.X);
        OutMin.Y = std::min(OutMin.Y, aabbMin.Y);
        OutMin.Z = std::min(OutMin.Z, aabbMin.Z);
        OutMax.X = std::max(OutMax.X, aabbMax.X);
        OutMax.Y = std::max(OutMax.Y, aabbMax.Y);
        OutMax.Z = std::max(OutMax.Z, aabbMax.Z);
    }
}

void FUpdateLightBufferPass::BindDirectionalShadowTargetsAndClear(ID3D11DeviceContext* DeviceContext,
    ID3D11DepthStencilView*& OutDSV, ID3D11RenderTargetView*& OutRTV) const
{
    const auto& Renderer = URenderer::GetInstance();
    OutDSV = Renderer.GetDeviceResources()->GetDirectionalShadowMapDSV();
    OutRTV = Renderer.GetDeviceResources()->GetDirectionalShadowMapColorRTV();

    // Unbind SRV from PS slot to avoid read-write hazard when binding RTV
    ID3D11ShaderResourceView* NullSRV = nullptr;
    DeviceContext->PSSetShaderResources(10, 1, &NullSRV);

    DeviceContext->OMSetRenderTargets(1, &OutRTV, OutDSV);
    const float ClearMoments[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
    DeviceContext->ClearRenderTargetView(OutRTV, ClearMoments);
    DeviceContext->ClearDepthStencilView(OutDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
    DeviceContext->RSSetViewports(1, &DirectionalShadowViewport);
}

FMatrix FUpdateLightBufferPass::BuildDirectionalLightViewMatrix(const UDirectionalLightComponent* Light, const FVector& LightPosition) const
{
    FVector LightFwd = Light->GetForwardVector();
    FVector LightUp = (fabsf(LightFwd.Z) > 0.99f) ? FVector(1,0,0) : FVector(0,0,1);
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
    return LightView;
}

void FUpdateLightBufferPass::ComputeDirectionalOrthoBounds(const FMatrix& LightView,
    const FVector& SceneMin, const FVector& SceneMax, float& OutMinX, float& OutMaxX,
    float& OutMinY, float& OutMaxY, float& OutMinZ, float& OutMaxZ) const
{
    OutMinX = OutMinY = OutMinZ = +FLT_MAX;
    OutMaxX = OutMaxY = OutMaxZ = -FLT_MAX;
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
        OutMinX = std::min(OutMinX, lv.X); OutMinY = std::min(OutMinY, lv.Y); OutMinZ = std::min(OutMinZ, lv.Z);
        OutMaxX = std::max(OutMaxX, lv.X); OutMaxY = std::max(OutMaxY, lv.Y); OutMaxZ = std::max(OutMaxZ, lv.Z);
    }
}

void FUpdateLightBufferPass::SnapDirectionalBoundsToTexelGrid(const FMatrix& LightView, const FVector& SceneMin, const FVector& SceneMax,
    float& InOutMinX, float& InOutMaxX, float& InOutMinY, float& InOutMaxY, const D3D11_VIEWPORT& Viewport) const
{
    float worldUnitsPerTexelX = (InOutMaxX - InOutMinX) / Viewport.Width;
    float worldUnitsPerTexelY = (InOutMaxY - InOutMinY) / Viewport.Height;

    FVector worldCenter = (SceneMin + SceneMax) * 0.5f;
    FVector4 centerInLightView = FMatrix::VectorMultiply(FVector4(worldCenter, 1.0f), LightView);

    float snappedCenterX = floor(centerInLightView.X / worldUnitsPerTexelX) * worldUnitsPerTexelX;
    float snappedCenterY = floor(centerInLightView.Y / worldUnitsPerTexelY) * worldUnitsPerTexelY;

    float offsetX = snappedCenterX - centerInLightView.X;
    float offsetY = snappedCenterY - centerInLightView.Y;

    InOutMinX += offsetX; InOutMaxX += offsetX;
    InOutMinY += offsetY; InOutMaxY += offsetY;
}

FMatrix FUpdateLightBufferPass::OrthoRowLH(float l, float r, float b, float t, float zn, float zf) const
{
    FMatrix M = FMatrix::Identity();
    M.Data[0][0] =  2.0f/(r-l);
    M.Data[1][1] =  2.0f/(t-b);
    M.Data[2][2] =  1.0f/(zf-zn);
    M.Data[3][0] =  (l+r)/(l-r);
    M.Data[3][1] =  (t+b)/(b-t);
    M.Data[3][2] =  -zn/(zf-zn);
    return M;
}

void FUpdateLightBufferPass::UpdateDirectionalLVPConstants(const UDirectionalLightComponent* Light,
    const FMatrix& LightView, const FMatrix& LightProj, float l, float r, float b, float t, const D3D11_VIEWPORT& Viewport)
{
    FShadowMapConstants LVPShadowMap;
    LVPShadowMap.EyeView   = FMatrix::Identity();
    LVPShadowMap.EyeProj   = FMatrix::Identity();
    LVPShadowMap.EyeViewProjInv  = FMatrix::Identity();

    LVPShadowMap.LightViewP[0] = LightView;
    LVPShadowMap.LightProjP[0] = LightProj;
    LVPShadowMap.LightViewPInv[0] = LightView.Inverse();

    LVPShadowMap.ShadowParams = FVector4(0.002f, 0, 0, 0);
    LVPShadowMap.LightDirWS   = (-Light->GetForwardVector()).GetNormalized();
    LVPShadowMap.bInvertedLight = 0;
    LVPShadowMap.LightOrthoParams= FVector4(l, r, b, t);
    LVPShadowMap.ShadowMapSize   = FVector2(Viewport.Width, Viewport.Height);
    LVPShadowMap.bUsePSM = 0;
    FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, LVPShadowMap);

    LightViewP = LightView;
    LightProjP = LightProj;
    CachedEyeView = FMatrix::Identity();
    CachedEyeProj = FMatrix::Identity();
    LightOrthoLTRB = FVector4(l, r, b, t);
}

void FUpdateLightBufferPass::DrawAllStaticReceivers(const FRenderingContext& Context)
{
    for (auto MeshComp : Context.StaticMeshes)
    {
        if (!MeshComp || !MeshComp->IsVisible()) continue;
        RenderPrimitive(MeshComp);
    }
}

void FUpdateLightBufferPass::UpdateDirectionalPSMConstants(const FRenderingContext& Context,
    const UDirectionalLightComponent* Light, const D3D11_VIEWPORT& Viewport)
{
    // Camera matrices used for PSM
    UCamera* MainCamera = Context.CurrentCamera;
    FMatrix CameraView = MainCamera->GetCameraViewMatrix();
    FMatrix CameraProj = MainCamera->GetCameraProjectionMatrix();

    // 1) Receiver NDC box
    FVector ndcMin, ndcMax;
    ComputeReceiverNDCBox(Context, CameraView, CameraProj, ndcMin, ndcMax);

    // 2) Fit light to NDC box (build V'_L and initial ortho P)
    FMatrix V_L_prime, P_L_prime;
    FVector4 orthoLTRB;
    float nearPlane, farPlane;
    FitLightToNDCBox(Light->GetForwardVector(), ndcMin, ndcMax, CameraView, CameraProj,
                     V_L_prime, P_L_prime, orthoLTRB, nearPlane, farPlane);

    // 3) Texel snapping in light view
    float l = orthoLTRB.X, r = orthoLTRB.Y, b = orthoLTRB.Z, t = orthoLTRB.W;
    ApplyTexelSnappingToLightView(l, r, b, t, Viewport.Width, Viewport.Height);
    P_L_prime = OrthoRowLH(l, r, b, t, nearPlane, farPlane);
    orthoLTRB = FVector4(l, r, b, t);

    // 4) Fill PSM constants
    FMatrix EyeViewInv      = CameraView.Inverse();
    FMatrix EyeProjInv      = CameraProj.Inverse();
    FMatrix EyeViewProjInv  = EyeProjInv * EyeViewInv;
    FMatrix LightViewPInv   = V_L_prime.Inverse();

    float sx = Viewport.Width;
    float sy = Viewport.Height;
    FVector LdirWS = (-Light->GetForwardVector()).GetNormalized();

    FShadowMapConstants PSM;
    PSM.EyeView   = CameraView;
    PSM.EyeProj   = CameraProj;
    PSM.EyeViewProjInv  = EyeViewProjInv;
    PSM.LightViewP[0]   = V_L_prime;
    PSM.LightProjP[0]   = P_L_prime;
    PSM.LightViewPInv[0]= LightViewPInv;
    PSM.ShadowParams    = FVector4(Light->GetShadowBias(), Light->GetShadowSlopeBias(), 0, 0);
    PSM.LightDirWS      = LdirWS;
    PSM.bInvertedLight  = 0;
    PSM.LightOrthoParams= orthoLTRB;
    PSM.ShadowMapSize   = FVector2(sx, sy);
    PSM.bUsePSM         = 1;
    FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, PSM);

    // Cache for later usage (shading stage expects these)
    LightViewP    = V_L_prime;
    LightProjP    = P_L_prime;
    CachedEyeView = CameraView;
    CachedEyeProj = CameraProj;
    LightOrthoLTRB= orthoLTRB;
}

void FUpdateLightBufferPass::ComputeReceiverNDCBox(const FRenderingContext& Context, const FMatrix& CameraView,
    const FMatrix& CameraProj, FVector& OutNdcMin, FVector& OutNdcMax) const
{
    const float BIG = 1e30f;
    OutNdcMin = FVector(+BIG,+BIG,+BIG);
    OutNdcMax = FVector(-BIG,-BIG,-BIG);

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
            if (v.W <= 0.0f) continue;                      // 카메라 뒤는 제외
            FVector ndc(v.X/v.W, v.Y/v.W, v.Z/v.W);         // D3D: z∈[0,1]
            if (ndc.X < -1.2f || ndc.X > 1.2f) continue;
            if (ndc.Y < -1.2f || ndc.Y > 1.2f) continue;
            if (ndc.Z <  0.0f || ndc.Z > 1.2f) continue;
            OutNdcMin.X = std::min(OutNdcMin.X, ndc.X);
            OutNdcMin.Y = std::min(OutNdcMin.Y, ndc.Y);
            OutNdcMin.Z = std::min(OutNdcMin.Z, ndc.Z);
            OutNdcMax.X = std::max(OutNdcMax.X, ndc.X);
            OutNdcMax.Y = std::max(OutNdcMax.Y, ndc.Y);
            OutNdcMax.Z = std::max(OutNdcMax.Z, ndc.Z);
        }
    };

    for (auto MeshComp : Context.StaticMeshes)
    {
        if (!MeshComp || !MeshComp->IsVisible()) continue;
        FVector a,b; MeshComp->GetWorldAABB(a,b);
        Accum(a,b);
    }

    if (OutNdcMin.X >  1e29f) {
        OutNdcMin = FVector(-1,-1,0);
        OutNdcMax = FVector( 1, 1,1);
    }

    const float mxy=0.02f, mz=0.001f;
    OutNdcMin.X = std::max(-1.0f, OutNdcMin.X - mxy);
    OutNdcMin.Y = std::max(-1.0f, OutNdcMin.Y - mxy);
    OutNdcMax.X = std::min( 1.0f, OutNdcMax.X + mxy);
    OutNdcMax.Y = std::min( 1.0f, OutNdcMax.Y + mxy);
    OutNdcMin.Z = std::max( 0.0f, OutNdcMin.Z - mz);
    OutNdcMax.Z = std::min( 1.0f, OutNdcMax.Z + mz);

    const float quantStep = 0.1f;
    auto Quantize = [](float value, float step) {
        return floor(value / step) * step;
    };
    OutNdcMin.X = Quantize(OutNdcMin.X, quantStep);
    OutNdcMin.Y = Quantize(OutNdcMin.Y, quantStep);
    OutNdcMin.Z = Quantize(OutNdcMin.Z, quantStep * 0.1f);
    OutNdcMax.X = Quantize(OutNdcMax.X + quantStep, quantStep);
    OutNdcMax.Y = Quantize(OutNdcMax.Y + quantStep, quantStep);
    OutNdcMax.Z = Quantize(OutNdcMax.Z + quantStep * 0.1f, quantStep * 0.1f);

    OutNdcMin.X = std::max(-1.0f, OutNdcMin.X);
    OutNdcMin.Y = std::max(-1.0f, OutNdcMin.Y);
    OutNdcMax.X = std::min( 1.0f, OutNdcMax.X);
    OutNdcMax.Y = std::min( 1.0f, OutNdcMax.Y);
}

FMatrix FUpdateLightBufferPass::BuildLookAtRowLH(const FVector& Eye, const FVector& At, const FVector& UpHint) const
{
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
    M.Data[0][0]=Right.X; M.Data[0][1]=Up.X; M.Data[0][2]=Fwd.X;
    M.Data[1][0]=Right.Y; M.Data[1][1]=Up.Y; M.Data[1][2]=Fwd.Y;
    M.Data[2][0]=Right.Z; M.Data[2][1]=Up.Z; M.Data[2][2]=Fwd.Z;
    M.Data[3][0]= -Eye.Dot(Right);
    M.Data[3][1]= -Eye.Dot(Up);
    M.Data[3][2]= -Eye.Dot(Fwd);
    M.Data[3][3]=  1.0f;
    return M;
}

void FUpdateLightBufferPass::FitLightToNDCBox(const FVector& LightDirWorld, const FVector& ndcMin, const FVector& ndcMax,
    const FMatrix& CameraView, const FMatrix& CameraProj, FMatrix& OutV, FMatrix& OutP, FVector4& OutLTRB, float& OutNear, float& OutFar) const
{
    auto NormalizeSafe = [](const FVector& v, float eps = 1e-6f){
        float len = v.Length();
        return (len > eps) ? (v / len) : FVector(0,0,0);
    };
    FVector Lv = NormalizeSafe( FMatrix::VectorMultiply(FVector4(LightDirWorld,0.0f), CameraView).XYZ() );

    FVector center = (ndcMin + ndcMax) * 0.5f;
    float   diag   = (ndcMax - ndcMin).Length();
    float   eyeDist= std::max(0.25f, diag);
    FVector Eye    = center - Lv * eyeDist;
    FVector At     = center;

    FMatrix Vlp = BuildLookAtRowLH(Eye, At, FVector(0,0,1));

    float l=+FLT_MAX, b=+FLT_MAX, n=+FLT_MAX;
    float r=-FLT_MAX, t=-FLT_MAX, f=-FLT_MAX;
    FVector c[8]={
        {ndcMin.X,ndcMin.Y,ndcMin.Z},{ndcMax.X,ndcMin.Y,ndcMin.Z},
        {ndcMin.X,ndcMax.Y,ndcMin.Z},{ndcMax.X,ndcMax.Y,ndcMin.Z},
        {ndcMin.X,ndcMin.Y,ndcMax.Z},{ndcMax.X,ndcMin.Y,ndcMax.Z},
        {ndcMin.X,ndcMax.Y,ndcMax.Z},{ndcMax.X,ndcMax.Y,ndcMax.Z}
    };
    for(int i=0;i<8;++i){
        FVector4 v = FMatrix::VectorMultiply(FVector4(c[i],1.0f), Vlp);
        l = std::min(l, v.X); r = std::max(r, v.X);
        b = std::min(b, v.Y); t = std::max(t, v.Y);
        n = std::min(n, v.Z); f = std::max(f, v.Z);
    }

    OutLTRB = FVector4(l, r, b, t);
    const float pad = 0.001f;
    OutV = Vlp;
    OutNear = n - pad;
    OutFar = f + pad;
    OutP = OrthoRowLH(l, r, b, t, OutNear, OutFar);
}

void FUpdateLightBufferPass::ApplyTexelSnappingToLightView(float& InOutL, float& InOutR, float& InOutB, float& InOutT,
    float shadowMapWidth, float shadowMapHeight) const
{
    float worldUnitsPerTexelX = (InOutR - InOutL) / shadowMapWidth;
    float worldUnitsPerTexelY = (InOutT - InOutB) / shadowMapHeight;

    float centerX = (InOutL + InOutR) * 0.5f;
    float centerY = (InOutB + InOutT) * 0.5f;

    float snappedCenterX = floor(centerX / worldUnitsPerTexelX) * worldUnitsPerTexelX;
    float snappedCenterY = floor(centerY / worldUnitsPerTexelY) * worldUnitsPerTexelY;

    float offsetX = snappedCenterX - centerX;
    float offsetY = snappedCenterY - centerY;

    InOutL += offsetX;
    InOutR += offsetX;
    InOutB += offsetY;
    InOutT += offsetY;
}

void FUpdateLightBufferPass::RenderDirectionalShadowSimple(FRenderingContext& Context)
{
    const auto& Renderer = URenderer::GetInstance();
    auto DeviceContext = Renderer.GetDeviceContext();
    
    // Directional Light Shadow Map 렌더링
    auto DirectionalLightComponent = Context.DirectionalLight;
    if (!DirectionalLightComponent)
    {
        return;
    }

    // === LVP용: 월드 전체 AABB 집계 (카메라에 독립)
    FVector SceneMin, SceneMax;
    ComputeSceneWorldAABB(Context, SceneMin, SceneMax);
    //UE_LOG_INFO("[LVP] World AABB Min(%.2f, %.2f, %.2f) Max(%.2f, %.2f, %.2f)", SceneMin.X, SceneMin.Y, SceneMin.Z, SceneMax.X, SceneMax.Y, SceneMax.Z);

    // Shadow Map DSV 설정 및 클리어
    ID3D11DepthStencilView* ShadowDSV = nullptr;
    ID3D11RenderTargetView* ShadowRTV = nullptr;
    BindDirectionalShadowTargetsAndClear(DeviceContext, ShadowDSV, ShadowRTV);

    // Shadow Map Viewport 설정
    DeviceContext->RSSetViewports(1, &DirectionalShadowViewport);
    
    // === Simple Ortho Shadow ===
    FVector LightPosition = (SceneMin + SceneMax) / 2.0f;
    LightPosition.Z += 200.0f;
    FMatrix LightView = BuildDirectionalLightViewMatrix(DirectionalLightComponent, LightPosition);

    float oMinX=+FLT_MAX, oMinY=+FLT_MAX, oMinZ=+FLT_MAX;
    float oMaxX=-FLT_MAX, oMaxY=-FLT_MAX, oMaxZ=-FLT_MAX;
    ComputeDirectionalOrthoBounds(LightView, SceneMin, SceneMax,
        oMinX, oMaxX, oMinY, oMaxY, oMinZ, oMaxZ);
    
    // ★ LVP에도 Texel Snapping 적용
    SnapDirectionalBoundsToTexelGrid(LightView, SceneMin, SceneMax,
        oMinX, oMaxX, oMinY, oMaxY, DirectionalShadowViewport);

    FMatrix LightProj = OrthoRowLH(oMinX, oMaxX, oMinY, oMaxY, oMinZ, oMaxZ);
    
    // bCastShadows에 따라 분기
    // === LVP (Shadow Maps) ===
    if (!DirectionalLightComponent->GetCastShadows())
    {
        // ★ Texel Snapping은 이미 위(line 182-204)에서 적용됨
        UpdateDirectionalLVPConstants(DirectionalLightComponent, LightView, LightProj,
            oMinX, oMaxX, oMinY, oMaxY, DirectionalShadowViewport);
    }
    else
    {
        UpdateDirectionalPSMConstants(Context, DirectionalLightComponent, DirectionalShadowViewport);
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
    DrawAllStaticReceivers(Context);
}

void FUpdateLightBufferPass::RenderDirectionalCSM(FRenderingContext& Context)
{
    UE_LOG("CSM Path Enabled: Generate Cascaded Shadow Maps.");
    const auto& Renderer = URenderer::GetInstance();
    auto DeviceContext = Renderer.GetDeviceContext();

    const UCamera* Camera = Context.CurrentCamera;
    if (!Camera) return;

    UDirectionalLightComponent* Light = !Context.DirectionalLight ? nullptr : Context.DirectionalLight;
    if (!Light) return;

    CascadedShadowMapConstants = {};
    CalculateCascadeSplits(CascadedShadowMapConstants.CascadeSplits, Camera);

    const float* pSplits = &CascadedShadowMapConstants.CascadeSplits.X;
    DeviceContext->RSSetViewports(1, &DirectionalShadowViewport);

    for (int i = 0; i < MAX_CASCADES; i++)
    {
        ID3D11DepthStencilView* CurrentDsv = Renderer.GetInstance().GetDeviceResources()->GetCascadedShadowMapDSV(i);
        DeviceContext->OMSetRenderTargets(0, nullptr, CurrentDsv);
        DeviceContext->ClearDepthStencilView(CurrentDsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

        float NearSplit = (i == 0) ? Camera->GetNearZ() : pSplits[i - 1];
        float FarSplit = pSplits[i];
        FVector FrustumCorners[8];
        Camera->GetFrustumCorners(FrustumCorners, NearSplit, FarSplit);

        FVector FrustumCenter = FVector::ZeroVector();
        for (int j = 0; j < 8; j++) { FrustumCenter += FrustumCorners[j]; }
        FrustumCenter /= 8.0f;

        FMatrix LightViewMatrix;
        {
            FVector LightDir = Light->GetForwardVector().GetNormalized();
            float ShadowDistance = 250.0f;
            FVector LightPos = FrustumCenter - LightDir * ShadowDistance;
            FVector TargetPos = FrustumCenter;
            FVector UpVector = (abs(LightDir.Z) > 0.99f) ? FVector(0, 1, 0) : FVector(0, 0, 1);

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

        FCameraConstants LightCameraConsts;
        LightCameraConsts.View = LightViewMatrix;
        LightCameraConsts.Projection = CascadeLightProj;
        FRenderResourceFactory::UpdateConstantBufferData(LightCameraConstantBuffer, LightCameraConsts);

        for (auto MeshComp : Context.StaticMeshes)
        {
            if (!MeshComp || !MeshComp->IsVisible())    continue;
            RenderPrimitive(MeshComp);
        }
    }
}


void FUpdateLightBufferPass::RenderPrimitive(UStaticMeshComponent* MeshComp)
{
    if (!MeshComp || !MeshComp->GetStaticMesh()) return;

    FStaticMesh* MeshAsset = MeshComp->GetStaticMesh()->GetStaticMeshAsset();
    if (!MeshAsset) return;

    // Shadow Map 렌더링용 파이프라인 설정
    ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState(
        { ECullMode::None, EFillMode::Solid });
    
    FPipelineInfo PipelineInfo = {
        ShadowMapInputLayout,
        ShadowMapVS,
        RS,
        URenderer::GetInstance().GetDefaultDepthStencilState(),
        ShadowMapPS,  // ★ PS 바인드 (RenderPrimitive에서도 depth write 보장)
        nullptr,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    };
    
    Pipeline->UpdatePipeline(PipelineInfo);
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

void FUpdateLightBufferPass::Release()
{
    // 셀이더는 Renderer가 관리하므로 여기서 해제하지 않음
    ShadowMapVS = nullptr;
    ShadowMapPS = nullptr;
    ShadowMapInputLayout = nullptr;
    
    // Light Camera 상수 버퍼 해제
    SafeRelease(LightCameraConstantBuffer);
}

