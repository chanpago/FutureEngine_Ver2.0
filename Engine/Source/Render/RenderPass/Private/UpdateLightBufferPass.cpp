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
    PSMConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FPSMConstants>();
    
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

    // Directional Light Shadow Map 렌더링
    for (auto Light : Context.DirectionalLights)
    {
        if (!Light)
        {
            continue;
        }
        
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
            
        // === (2) 현재 카메라 프러스텀 8코너 (월드) ===
        FVector FrustumW[8];
        {
            const float tanH = tanf(FVector::GetDegreeToRadian(FovY) * 0.5f);
            const float nh = Near * tanH, nw = nh * Aspect;
            const float fh = Far  * tanH, fw = fh * Aspect;
            const FVector Right = MainCamera->GetRight();
            const FVector Up    = MainCamera->GetUp();

            const FVector Nc = CameraPos + CameraForward * Near;
            const FVector Fc = CameraPos + CameraForward * Far;

            // Near
            FrustumW[0] = Nc - Right * nw - Up * nh; // L B
            FrustumW[1] = Nc + Right * nw - Up * nh; // R B
            FrustumW[2] = Nc + Right * nw + Up * nh; // R T
            FrustumW[3] = Nc - Right * nw + Up * nh; // L T
            // Far
            FrustumW[4] = Fc - Right * fw - Up * fh;
            FrustumW[5] = Fc + Right * fw - Up * fh;
            FrustumW[6] = Fc + Right * fw + Up * fh;
            FrustumW[7] = Fc - Right * fw + Up * fh;
        }

        // === (3) ★ 프러스텀을 PSM 공간으로 보냄: p' = p * V_e * P_e → NDC divide ===
        struct P3 { float x,y,z; };
        P3 FrustumPSM[8];
        for (int i=0;i<8;++i)
        {
            const FVector4 p = FVector4(FrustumW[i], 1);
            FVector4 q = FMatrix::VectorMultiply(p, CameraView);
            q          = FMatrix::VectorMultiply(q, CameraProj);
            const float rw = (q.W != 0.0f) ? q.W : 1.0f;
            FrustumPSM[i] = { q.X/rw, q.Y/rw, q.Z/rw }; // NDC: 대략 [-1,1]
        }

        // === (4) ★ PSM 공간에서 라이트 방향 계산 ===
        // 방법: 프러스텀 중심 C_w와 C_w + L_world를 각각 PSM 공간으로 보낸 뒤 차이로 L'
        auto Avg = [&](const P3* a, int n){
            P3 r{0,0,0}; for(int i=0;i<n;++i){ r.x+=a[i].x; r.y+=a[i].y; r.z+=a[i].z; }
            r.x/=n; r.y/=n; r.z/=n; return r;
        };
        const P3 CenterPSM = Avg(FrustumPSM, 8);

        const FVector Lw = Light->GetForwardVector().GetNormalized(); // 방향광 월드 방향
        const FVector Cw = (FrustumW[0] + FrustumW[1] + FrustumW[2] + FrustumW[3] +
                            FrustumW[4] + FrustumW[5] + FrustumW[6] + FrustumW[7]) * (1.0f/8.0f);
        auto ToPSM = [&](const FVector& world)->P3{
            FVector4 q = FMatrix::VectorMultiply(FVector4(world,1), CameraView);
            q          = FMatrix::VectorMultiply(q, CameraProj);
            float rw   = (q.W!=0.0f)? q.W : 1.0f;
            return { q.X/rw, q.Y/rw, q.Z/rw };
        };
        const P3 A = ToPSM(Cw);
        const P3 B = ToPSM(Cw + Lw * 10.0f); // 임의 스케일
        auto Norm = [](P3 v){ float m = sqrtf(v.x*v.x+v.y*v.y+v.z*v.z)+1e-8f; v.x/=m; v.y/=m; v.z/=m; return v; };
        P3 Lp = Norm( P3{ B.x - A.x, B.y - A.y, B.z - A.z } ); // ★ PSM 공간 라이트 방향 L'

        // === (5) ★ PSM 공간에서 LightView'(look-along -L') 구성 ===
        auto Cross = [](const P3& a, const P3& b){
            return P3{ a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
        };
        auto Dot = [](const P3& a, const P3& b){ return a.x*b.x + a.y*b.y + a.z*b.z; };

        P3 Fwd = { -Lp.x, -Lp.y, -Lp.z }; // forward = -L'
        P3 Up0 = (fabsf(Fwd.z) > 0.99f) ? P3{0,1,0} : P3{0,0,1};
        P3 RightP = Norm( Cross(Up0, Fwd) );   // LH: Right = Up × Forward
        P3 UpP    = Cross(Fwd, RightP);        // Up = Forward × Right

        // PSM 공간의 '원점'을 프러스텀 중심으로
        const P3 Center = CenterPSM;
        float maxProj = 0.f;
        for (int i=0;i<8;++i){
            P3 d = { FrustumPSM[i].x - Center.x, FrustumPSM[i].y - Center.y, FrustumPSM[i].z - Center.z };
            float t = fabsf(Dot(d, Fwd));
            maxProj = std::max(maxProj, t);
        }
        const float dEye = maxProj + 0.05f; // 약간 margin
        P3 EyeP = { Center.x - Fwd.x * dEye, Center.y - Fwd.y * dEye, Center.z - Fwd.z * dEye };

        // row-vector LookAt in "PSM space"
        FMatrix V_L_prime = FMatrix::Identity();
        V_L_prime.Data[0][0] = RightP.x; V_L_prime.Data[0][1] = UpP.x; V_L_prime.Data[0][2] = Fwd.x; V_L_prime.Data[0][3] = 0;
        V_L_prime.Data[1][0] = RightP.y; V_L_prime.Data[1][1] = UpP.y; V_L_prime.Data[1][2] = Fwd.y; V_L_prime.Data[1][3] = 0;
        V_L_prime.Data[2][0] = RightP.z; V_L_prime.Data[2][1] = UpP.z; V_L_prime.Data[2][2] = Fwd.z; V_L_prime.Data[2][3] = 0;
        V_L_prime.Data[3][0] = -(EyeP.x*RightP.x + EyeP.y*RightP.y + EyeP.z*RightP.z);
        V_L_prime.Data[3][1] = -(EyeP.x*UpP.x    + EyeP.y*UpP.y    + EyeP.z*UpP.z);
        V_L_prime.Data[3][2] = -(EyeP.x*Fwd.x    + EyeP.y*Fwd.y    + EyeP.z*Fwd.z);
        V_L_prime.Data[3][3] = 1.0f;

        // === (6) ★ H'(=프러스텀 PSM 점군)을 LightView'로 보내 extents 산출 ===
        float minX=+FLT_MAX, minY=+FLT_MAX, minZ=+FLT_MAX;
        float maxX=-FLT_MAX, maxY=-FLT_MAX, maxZ=-FLT_MAX;
        for (int i=0;i<8;++i)
        {
            const FVector4 ppsm = FVector4(FrustumPSM[i].x, FrustumPSM[i].y, FrustumPSM[i].z, 1);
            const FVector4 lv   = FMatrix::VectorMultiply(ppsm, V_L_prime);
            minX = std::min(minX, lv.X); minY = std::min(minY, lv.Y); minZ = std::min(minZ, lv.Z);
            maxX = std::max(maxX, lv.X); maxY = std::max(maxY, lv.Y); maxZ = std::max(maxZ, lv.Z);
        }

        // === (7) ★ Directional의 기본: Ortho P_L' 구성(PSM 공간) ===
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

        const float epsZ = 1e-4f;
        float zn = std::max(0.0f, minZ - epsZ);
        float zf =           maxZ + epsZ;
        FMatrix P_L_prime = OrthoRowLH(minX, maxX, minY, maxY, zn, zf);

        // 계산된 행렬을 멤버 변수에 저장
        LightViewP = V_L_prime;
        LightProjP = P_L_prime;

        // === (8) ★ 상수버퍼 채우기 & 파이프라인 바인딩 ===
        FPSMConstants PSM;
        PSM.EyeView   = CameraView;
        PSM.EyeProj   = CameraProj;
        PSM.LightViewP= V_L_prime;
        PSM.LightProjP= P_L_prime;
        PSM.ShadowParams = FVector4( /*bias*/ 0.0008f, 0,0,0 );
        PSM.bInvertedLight = 0; // 방향광에선 보통 0
        FRenderResourceFactory::UpdateConstantBufferData(PSMConstantBuffer, PSM);



        // 파이프라인: b0=Model, b1=PSM
        FPipelineInfo Pipe = {
            ShadowMapInputLayout,
            ShadowMapVS,
            FRenderResourceFactory::GetRasterizerState({ECullMode::Back, EFillMode::Solid}),
            URenderer::GetInstance().GetDefaultDepthStencilState(),
            nullptr, // PS 없음
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
    
}


void FUpdateLightBufferPass::RenderPrimitive(UStaticMeshComponent* MeshComp)
{
    if (!MeshComp || !MeshComp->GetStaticMesh()) return;

    FStaticMesh* MeshAsset = MeshComp->GetStaticMesh()->GetStaticMeshAsset();
    if (!MeshAsset) return;

    // Shadow Map 렌더링용 파이프라인 설정
    ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState(
        { ECullMode::Back, EFillMode::Solid });
    
    FPipelineInfo PipelineInfo = {
        ShadowMapInputLayout,
        ShadowMapVS,
        RS,
        URenderer::GetInstance().GetDefaultDepthStencilState(),
        nullptr,
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
