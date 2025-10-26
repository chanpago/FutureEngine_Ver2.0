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

    // Reset selected shadow caster to directional by default
    ShadowCasterType = 0;
    SpotShadowCasterIndex = -1;

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
        //UE_LOG_INFO("[LVP] World AABB Min(%.2f, %.2f, %.2f) Max(%.2f, %.2f, %.2f)", SceneMin.X, SceneMin.Y, SceneMin.Z, SceneMax.X, SceneMax.Y, SceneMax.Z);
        
        // Shadow Map DSV 설정 및 클리어
        ID3D11DepthStencilView* ShadowDSV = Renderer.GetDeviceResources()->GetDirectionalShadowMapDSV();
        DeviceContext->OMSetRenderTargets(0, nullptr, ShadowDSV);  // RTV는 필요 없음, DSV만 사용
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
        LightPosition.Z += 200.0f;
            
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
            
        FMatrix LightProj = OrthoRowLH(oMinX, oMaxX, oMinY, oMaxY, oMinZ, oMaxZ);


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
        };
        
        // (2) NDC 박스를 광원 방향으로 본 라이트 V_L′, P_L′ 생성
        auto FitLightToNDCBox = [&](const FVector& LightDirWorld,
                                    const FVector& ndcMin, const FVector& ndcMax,
                                    FMatrix& OutV, FMatrix& OutP, FVector4& OutLTRB)
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
            //OutP = OrthoRowLH(l, r, b, t, std::max(0.0f, n - pad), f + pad);
            OutP = OrthoRowLH(l, r, b, t, n - pad, f + pad);
            OutLTRB = FVector4(l, r, b, t);
        };
        
        // bCastShadows에 따라 분기
        // === LVP (Shadow Maps) ===
        if (!Light->GetCastShadows())
        {
            //UE_LOG("LVP");
            FShadowMapConstants LVPShadowMap;
            LVPShadowMap.EyeView   = FMatrix::Identity();
            LVPShadowMap.EyeProj   = FMatrix::Identity();
            LVPShadowMap.EyeViewProjInv  = FMatrix::Identity(); // LVP에선 안 씀
            
            LVPShadowMap.LightViewP= LightView;
            LVPShadowMap.LightProjP= LightProj;
            LVPShadowMap.LightViewPInv   = LightView.Inverse(); // L_texel 계산에 안 쓰지만 채워두면 안전
            
            LVPShadowMap.ShadowParams = FVector4(0.002f, 0, 0, 0);
            
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
            //UE_LOG("PSM");
            // 시작값
            FVector ndcMin, ndcMax;
            ComputeReceiverNDCBox(ndcMin, ndcMax);

            FMatrix V_L_prime, P_L_prime;
            FVector4 orthoLTRB;
            FitLightToNDCBox(Light->GetForwardVector(), ndcMin, ndcMax, V_L_prime, P_L_prime, orthoLTRB);

            // 행벡터 합성/역행렬: (V_e P_e)^(-1) = P_e^{-1} * V_e^{-1}
            FMatrix EyeViewInv      = CameraView.Inverse();
            FMatrix EyeProjInv      = CameraProj.Inverse();
            FMatrix EyeViewProjInv  = EyeProjInv * EyeViewInv;

            // 라이트 뷰 역행렬
            FMatrix LightViewPInv   = V_L_prime.Inverse();

            // 섀도맵 해상도 (뷰포트 또는 텍스처 크기에서 가져와도 됨)
            float sx = DirectionalShadowViewport.Width;
            float sy = DirectionalShadowViewport.Height;

            // 바이어스 + 라이트 방향(표면→광원)
            float a = Light->GetShadowBias();
            float b = Light->GetShadowSlopeBias();
            FVector LdirWS = -Light->GetForwardVector(); // 라이트 광선 방향이 +Fwd라면, 표면→광원은 -Fwd
            LdirWS = LdirWS.GetNormalized();
            
            // === PSM (Perspective Shadow Maps) ===
            FShadowMapConstants PSM;
            PSM.EyeView   = CameraView;
            PSM.EyeProj   = CameraProj;
            PSM.EyeViewProjInv  = EyeViewProjInv;
            
            PSM.LightViewP = V_L_prime;
            PSM.LightProjP = P_L_prime;
            PSM.LightViewPInv   = LightViewPInv;
            
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

    // Spot Light Shadow Map 렌더링 (단일 스포트라이트 우선 적용)
    for (size_t si = 0; si < Context.SpotLights.size(); ++si)
    {
        USpotLightComponent* Spot = Context.SpotLights[si];
        if (!Spot || !Spot->GetVisible() || !Spot->GetLightEnabled())
            continue;

        // Bind Spot DSV and clear
        ID3D11DepthStencilView* SpotDSV = Renderer.GetDeviceResources()->GetSpotShadowMapDSV();
        if (!SpotDSV)
            break;
        DeviceContext->OMSetRenderTargets(0, nullptr, SpotDSV);
        DeviceContext->ClearDepthStencilView(SpotDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

        // Spot viewport
        DeviceContext->RSSetViewports(1, &SpotShadowViewport);

        // Build light view (row-vector LookAt)
        auto NormalizeSafe = [](const FVector& v, float eps = 1e-6f){
            float len = v.Length();
            return (len > eps) ? (v / len) : FVector(0,0,0);
        };
        auto BuildLookAtRowLH = [](const FVector& Eye, const FVector& At, const FVector& UpHint)
        {
            auto NormalizeSafeL = [](const FVector& v, float eps = 1e-6f){
                float len = v.Length();
                return (len > eps) ? (v / len) : FVector(0,0,0);
            };
            FVector Fwd = NormalizeSafeL(At - Eye);
            FVector Up  = UpHint;
            if (fabsf(Fwd.Dot(Up)) > 0.99f) Up = (fabsf(Fwd.Z) > 0.9f) ? FVector(1,0,0) : FVector(0,0,1);
            FVector Right = NormalizeSafeL(Up.Cross(Fwd));
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
        };
        auto PerspectiveRowLH = [](float fovyRad, float aspect, float zn, float zf)
        {
            float f = 1.0f / std::tanf(fovyRad * 0.5f);
            FMatrix P = FMatrix::Identity();
            P.Data[0][0] = f / aspect;
            P.Data[1][1] = f;
            P.Data[2][2] = zf / (zf - zn);
            P.Data[2][3] = 1.0f;
            P.Data[3][2] = (-zn * zf) / (zf - zn);
            P.Data[3][3] = 0.0f;
            return P;
        };

        FVector LightPos = Spot->GetWorldLocation();
        FVector LightDir = Spot->GetForwardVector().GetNormalized();
        FVector UpHint   = (fabsf(LightDir.Z) > 0.99f) ? FVector(1,0,0) : FVector(0,0,1);
        FMatrix LightView = BuildLookAtRowLH(LightPos, LightPos + LightDir, UpHint);

        float fovY = Spot->GetOuterConeAngle() * 2.0f; // use outer cone as FOV
        float aspect = 1.0f;
        float zn = 0.5f; // slightly away from zero to reduce acne
        float zf = std::max(Spot->GetAttenuationRadius(), 1.0f);
        FMatrix LightProj = PerspectiveRowLH(fovY, aspect, zn, zf);

        // Fill constants (non-PSM path)
        FShadowMapConstants S;
        S.EyeView = FMatrix::Identity();
        S.EyeProj = FMatrix::Identity();
        S.EyeViewProjInv = FMatrix::Identity();
        S.LightViewP = LightView;
        S.LightProjP = LightProj;
        S.LightViewPInv = LightView.Inverse();
        S.ShadowParams = FVector4(Spot->GetShadowBias(), 0, 0, 0);
        S.LightDirWS = (-LightDir).GetNormalized();
        S.bInvertedLight = 0;
        S.LightOrthoParams = FVector4(0,0,0,0);
        S.ShadowMapSize = FVector2(SpotShadowViewport.Width, SpotShadowViewport.Height);
        S.bUsePSM = 0;
        // Caster selection: mark spotlight
        ShadowCasterType = 1;

        // Compute SpotShadowCasterIndex by matching visible/enabled spots order
        int idx = -1; int visibleIdx = -1;
        for (size_t k=0; k<Context.SpotLights.size(); ++k)
        {
            USpotLightComponent* L = Context.SpotLights[k];
            if (!L || !L->GetVisible() || !L->GetLightEnabled()) continue;
            ++visibleIdx;
            if (L == Spot) { idx = visibleIdx; break; }
        }
        SpotShadowCasterIndex = idx;

        FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, S);

        // Cache for shading pass
        LightViewP = LightView;
        LightProjP = LightProj;
        CachedEyeView = FMatrix::Identity();
        CachedEyeProj = FMatrix::Identity();

        // Pipeline state
        FPipelineInfo PipeS = {
            ShadowMapInputLayout,
            ShadowMapVS,
            FRenderResourceFactory::GetRasterizerState({ECullMode::None, EFillMode::Solid}),
            URenderer::GetInstance().GetDefaultDepthStencilState(),
            ShadowMapPS,
            nullptr,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
        };
        Pipeline->UpdatePipeline(PipeS);
        Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);
        Pipeline->SetConstantBuffer(6, EShaderType::VS, PSMConstantBuffer);
        for (auto MeshComp : Context.StaticMeshes)
        {
            if (!MeshComp || !MeshComp->IsVisible()) continue;
            RenderPrimitive(MeshComp);
        }
        break;
    }
    // Shadow Map DSV를 unbind (다음 pass에서 SRV로 사용하기 위해)
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    
    // Viewport 복원
    DeviceContext->RSSetViewports(1, &OriginalViewport);
    
    // 원본 Render Targets 복원
    DeviceContext->OMSetRenderTargets(1, &OriginalRTVs, OriginalDSV);
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

