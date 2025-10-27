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
    BakeShadowMap(Context);
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

    // Shadow Map DSV 설정 및 클리어
    ID3D11DepthStencilView* ShadowDSV = Renderer.GetDeviceResources()->GetDirectionalShadowMapDSV();
    ID3D11RenderTargetView* ShadowRTV = Renderer.GetDeviceResources()->GetDirectionalShadowMapColorRTV();

    // Null 체크: Shadow Map 리소스가 생성되지 않았으면 early return
    if (!ShadowDSV || !ShadowRTV)
    {
        UE_LOG_ERROR("Shadow Map resources not initialized! DSV=%p, RTV=%p", ShadowDSV, ShadowRTV);
        return;
    }

    
    // Directional Light Shadow Map 렌더링
    for (auto Light : Context.DirectionalLights)
    {
        if (!Light)
        {
            continue;
        }
        
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

        // === Simple Ortho Shadow ===
        FVector LightPosition = (SceneMin + SceneMax) / 2.0f;
        LightPosition.Z = SceneMax.Z;
            
        // Directional Light의 fwd만으로 정규 직교 기저 구성
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
        }
            
        // row-vector Ortho LH
        auto OrthoRowLH = [](float l,float r,float b,float t,float zn,float zf){
            FMatrix M = FMatrix::Identity();
            M.Data[0][0] =  2.0f/(r-l);
            M.Data[1][1] =  2.0f/(t-b);
            M.Data[2][2] =  1.0f/(zf-zn);
            M.Data[3][0] =  (l+r)/(l-r);
            M.Data[3][1] =  (t+b)/(b-t);
            M.Data[3][2] =  -zn/(zf-zn);
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

        // warp용 헬퍼 변수
        FMatrix EyeViewProj =  CameraView * CameraProj;
        FMatrix EyeViewProjInv = CameraProj.Inverse() * CameraView.Inverse();
        
        auto WarpWorldPoint = [&](const FVector4& world) -> FVector4
        {
            FVector4 eyeClip = FMatrix::VectorMultiply(world, CameraView);
            eyeClip = FMatrix::VectorMultiply(eyeClip, CameraProj);

            float w = max(1e-5f, eyeClip.W);
            FVector camNDC = FVector(eyeClip.X / w, eyeClip.Y / w, eyeClip.Z / w);

            FVector4 warped = FMatrix::VectorMultiply(FVector4(camNDC, 1.0f), EyeViewProjInv);
            warped /= max(1e-5f, warped.W);
            return warped;
        };
        
        // bCastShadows에 따라 분기
        // === LVP (Shadow Maps) ===
        if (!Light->GetCastShadows())
        {
            //UE_LOG("LVP");
            // ★ Texel Snapping은 이미 위(line 182-204)에서 적용됨

            FShadowMapConstants LVPShadowMap;
            LVPShadowMap.EyeView   = FMatrix::Identity();
            LVPShadowMap.EyeProj   = FMatrix::Identity();
            LVPShadowMap.EyeViewProjInv  = FMatrix::Identity(); // LVP에선 안 씀

            LVPShadowMap.LightViewP= LightView;
            LVPShadowMap.LightProjP= LightProj;
            LVPShadowMap.LightViewPInv   = LightView.Inverse(); // L_texel 계산에 안 쓰지만 채워두면 안전

            LVPShadowMap.ShadowParams = FVector4(0.00f, 0, 0, 0);

            LVPShadowMap.LightDirWS      = (-Light->GetForwardVector()).GetNormalized();
            LVPShadowMap.bInvertedLight = 0;

            // 오쏘 경계(l,r,b,t)와 섀도맵 해상도
            LVPShadowMap.LightOrthoParams= FVector4(oMinX, oMaxX, oMinY, oMaxY);
            LVPShadowMap.ShadowMapSize   = FVector2(DirectionalShadowViewport.Width, DirectionalShadowViewport.Height);

            LVPShadowMap.bUsePSM = 0;
            FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, LVPShadowMap);

            LightViewP = LightView;
            LightProjP = LightProj;
            CachedEyeView = FMatrix::Identity();
            CachedEyeProj = FMatrix::Identity();

            LightOrthoLTRB = FVector4(oMinX, oMaxX, oMinY, oMaxY);
        }
        else
        {
            float psmMinX=+FLT_MAX, psmMinY=+FLT_MAX;
            float psmMaxX=-FLT_MAX, psmMaxY=-FLT_MAX;
            float frustumMinZ = +FLT_MAX, frustumMaxZ = -FLT_MAX;

            // --- 1. Calculate warped AABB of the camera frustum for a stable Z range ---
            static const FVector NdcCorners[8] = {
                {-1,-1,0},{ 1,-1,0},{-1, 1,0},{ 1, 1,0},
                {-1,-1,1},{ 1,-1,1},{-1, 1,1},{ 1, 1,1}
            };

            for (int i=0; i<8; ++i)
            {
                FVector4 worldCorner = FMatrix::VectorMultiply(FVector4(NdcCorners[i], 1.0f), EyeViewProjInv);
                worldCorner /= std::max(1e-6f, worldCorner.W);

                FVector4 warpedPoint = WarpWorldPoint(worldCorner);
                FVector4 lightPos = FMatrix::VectorMultiply(warpedPoint, LightView);

                psmMinX = std::min(psmMinX, lightPos.X); psmMaxX = std::max(psmMaxX, lightPos.X);
                psmMinY = std::min(psmMinY, lightPos.Y); psmMaxY = std::max(psmMaxY, lightPos.Y);
                frustumMinZ = std::min(frustumMinZ, lightPos.Z); frustumMaxZ = std::max(frustumMaxZ, lightPos.Z);
            }

            // --- 2. Include visible casters to tighten the X/Y bounds ---
            for (auto MeshComp : Context.StaticMeshes)
            {
                if (!MeshComp || !MeshComp->IsVisible()) { continue; }

                FVector meshMin, meshMax;
                MeshComp->GetWorldAABB(meshMin, meshMax);

                FVector meshCorners[8] = {
                    {meshMin.X, meshMin.Y, meshMin.Z}, {meshMax.X, meshMin.Y, meshMin.Z},
                    {meshMin.X, meshMax.Y, meshMin.Z}, {meshMax.X, meshMax.Y, meshMin.Z},
                    {meshMin.X, meshMin.Y, meshMax.Z}, {meshMax.X, meshMin.Y, meshMax.Z},
                    {meshMin.X, meshMax.Y, meshMax.Z}, {meshMax.X, meshMax.Y, meshMax.Z}
                };

                bool isInFrustum = false;
                for (int iCorner=0; iCorner<8; ++iCorner)
                {
                    FVector4 clipPos = FMatrix::VectorMultiply(FVector4(meshCorners[iCorner], 1.0f), EyeViewProj);
                    if (clipPos.X >= -clipPos.W && clipPos.X <= clipPos.W &&
                        clipPos.Y >= -clipPos.W && clipPos.Y <= clipPos.W &&
                        clipPos.Z >= 0.0f && clipPos.Z <= clipPos.W)
                    {
                        isInFrustum = true;
                        break;
                    }
                }

                if (!isInFrustum) { continue; }

                for (int iCorner=0; iCorner<8; ++iCorner)
                {
                    FVector4 warpedPoint = WarpWorldPoint(FVector4(meshCorners[iCorner], 1.0f));
                    FVector4 lightPos = FMatrix::VectorMultiply(warpedPoint, LightView);
                    psmMinX = std::min(psmMinX, lightPos.X); psmMaxX = std::max(psmMaxX, lightPos.X);
                    psmMinY = std::min(psmMinY, lightPos.Y); psmMaxY = std::max(psmMaxY, lightPos.Y);
                    // ★ Z 범위도 업데이트! (메쉬가 frustum보다 앞/뒤에 있을 수 있음)
                    frustumMinZ = std::min(frustumMinZ, lightPos.Z);
                    frustumMaxZ = std::max(frustumMaxZ, lightPos.Z);
                }
            }

            // --- 3. Create the final robust projection matrix ---
            FMatrix LightProjPSM = OrthoRowLH(psmMinX, psmMaxX, psmMinY, psmMaxY, frustumMinZ, frustumMaxZ);

            // === PSM (Perspective Shadow Maps) ===
            FShadowMapConstants PSM;
            PSM.EyeView   = CameraView;
            PSM.EyeProj   = CameraProj;
            PSM.EyeViewProjInv  = EyeViewProjInv;
            PSM.LightViewP = LightView;
            PSM.LightProjP = LightProjPSM;
            PSM.LightViewPInv   = LightView.Inverse();
            PSM.ShadowParams = FVector4(Light->GetShadowBias(), Light->GetShadowSlopeBias(), 0, 0);
            PSM.LightDirWS      = (-Light->GetForwardVector()).GetNormalized();
            PSM.bInvertedLight = 0;
            PSM.LightOrthoParams= FVector4(psmMinX, psmMaxX, psmMinY, psmMaxY);
            PSM.ShadowMapSize   = FVector2(DirectionalShadowViewport.Width, DirectionalShadowViewport.Height);
            PSM.bUsePSM = 1;
            FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, PSM);

            LightViewP = LightView;
            LightProjP = LightProjPSM;
            CachedEyeView = CameraView;
            CachedEyeProj = CameraProj;

            LightOrthoLTRB = FVector4(psmMinX, psmMaxX, psmMinY, psmMaxY);
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
    
    // Shadow Map DSV를 unbind (다음 pass에서 SRV로 사용하기 위해)
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    
    // Viewport 복원
    DeviceContext->RSSetViewports(1, &OriginalViewport);
    
    // 원본 Render Targets 복원
    DeviceContext->OMSetRenderTargets(1, &OriginalRTVs, OriginalDSV);

    // Generate mipmaps for VSM color shadow map (for smoother filtering)
    if (auto* ColorSRV = URenderer::GetInstance().GetDeviceResources()->GetDirectionalShadowMapColorSRV())
    {
        DeviceContext->GenerateMips(ColorSRV);
    }
    
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

void FUpdateLightBufferPass::Release()
{
    // 셀이더는 Renderer가 관리하므로 여기서 해제하지 않음
    ShadowMapVS = nullptr;
    ShadowMapPS = nullptr;
    ShadowMapInputLayout = nullptr;
    
    // Light Camera 상수 버퍼 해제
    SafeRelease(LightCameraConstantBuffer);
}

