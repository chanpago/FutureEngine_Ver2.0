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
    // Light 전용 Camera 상수 버퍼 생성
    LightCameraConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FCameraConstants>();
    // Shadow Map용 Viewport 초기화
    // Directional Light Shadow Map (2048x2048)
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
    // TODO: Shadow Map 리소스가 준비되면 여기서 베이킹 수행
    // 현재는 기본 구조만 작성
    
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
        if (!Light) continue;

        // Shadow Map DSV 설정 및 클리어
        ID3D11DepthStencilView* ShadowDSV = Renderer.GetDeviceResources()->GetDirectionalShadowMapDSV();
        DeviceContext->OMSetRenderTargets(0, nullptr, ShadowDSV);  // RTV는 필요 없음, DSV만 사용
        DeviceContext->ClearDepthStencilView(ShadowDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
        
        // Shadow Map Viewport 설정
        DeviceContext->RSSetViewports(1, &DirectionalShadowViewport);
        
        // Light View Matrix 계산
        FVector LightDir = Light->GetForwardVector().GetNormalized();
        FVector LightPos = -LightDir * 500.0f;  // Light를 충분히 멀리 배치 (Directional Light는 무한 멀리에 있다고 가정)
        FVector TargetPos = LightPos + LightDir;
        FVector UpVector(0, 0, 1);  // 월드 Up 방향
        
        // Light의 Forward가 거의 수직이면 Up 벡터 조정
        if (std::abs(LightDir.Z) > 0.99f)
        {
            UpVector = FVector(0, 1, 0);
        }
        
        // View Matrix: LookAt 직접 계산
        FVector ZAxis = (TargetPos - LightPos).GetNormalized();  // Forward
        FVector XAxis = (UpVector.FVector::Cross(ZAxis)).GetNormalized();  // Right
        FVector YAxis = (ZAxis.FVector::Cross(XAxis));  // Up
        
        FMatrix LightViewMatrix = FMatrix::Identity();
        LightViewMatrix.Data[0][0] = XAxis.X;
        LightViewMatrix.Data[0][1] = YAxis.X;
        LightViewMatrix.Data[0][2] = ZAxis.X;
        LightViewMatrix.Data[0][3] = 0.0f;
        
        LightViewMatrix.Data[1][0] = XAxis.Y;
        LightViewMatrix.Data[1][1] = YAxis.Y;
        LightViewMatrix.Data[1][2] = ZAxis.Y;
        LightViewMatrix.Data[1][3] = 0.0f;
        
        LightViewMatrix.Data[2][0] = XAxis.Z;
        LightViewMatrix.Data[2][1] = YAxis.Z;
        LightViewMatrix.Data[2][2] = ZAxis.Z;
        LightViewMatrix.Data[2][3] = 0.0f;
        
        LightViewMatrix.Data[3][0] = -XAxis.FVector::Dot(LightPos);
        LightViewMatrix.Data[3][1] = -YAxis.FVector::Dot(LightPos);
        LightViewMatrix.Data[3][2] = -ZAxis.FVector::Dot(LightPos);
        LightViewMatrix.Data[3][3] = 1.0f;
        
        // Projection Matrix: Orthographic 직접 계산 (Directional Light는 병렬 광선)
        float OrthoWidth = 300.0f;   // 그림자 범위 너비
        float OrthoHeight = 300.0f;  // 그림자 범위 높이
        float NearZ = 0.1f;
        float FarZ = 1000.0f;
        
        FMatrix LightProjMatrix = FMatrix::Identity();
        LightProjMatrix.Data[0][0] = 2.0f / OrthoWidth;
        LightProjMatrix.Data[1][1] = 2.0f / OrthoHeight;
        LightProjMatrix.Data[2][2] = 1.0f / (FarZ - NearZ);
        LightProjMatrix.Data[3][2] = -NearZ / (FarZ - NearZ);
        LightProjMatrix.Data[3][3] = 1.0f;
        
        // StaticMeshPass에서 사용할 수 있도록 저장
        this->LightViewMatrix = LightViewMatrix;
        this->LightProjectionMatrix = LightProjMatrix;
        
        // Light Camera 상수 버퍼 업데이트
        FCameraConstants LightCameraConstants;
        LightCameraConstants.View = LightViewMatrix;
        LightCameraConstants.Projection = LightProjMatrix;
        LightCameraConstants.ViewWorldLocation = LightPos;
        LightCameraConstants.NearClip = NearZ;
        LightCameraConstants.FarClip = FarZ;
        
        // 전용 Light Camera 상수 버퍼에 업데이트 (원본 카메라 버퍼를 건드리지 않음)
        FRenderResourceFactory::UpdateConstantBufferData(LightCameraConstantBuffer, LightCameraConstants);
        
        // 모든 Static Mesh를 Light 관점에서 렌더링
        for (auto MeshComp : Context.StaticMeshes)
        {
            if (!MeshComp || !MeshComp->IsVisible()) continue;
            RenderPrimitive(MeshComp);
        }
        
        // 현재는 하나의 Directional Light만 처리 (나중에 여러 개 지원 가능)
        break;
    }

    //// ============================================================================
    //// TODO: Spot Light Shadow Map 렌더링 (나중에 구현)
    //// ============================================================================
    ///*
    //int SpotLightIndex = 0;
    //for (auto Light : Context.SpotLights)
    //{
    //    if (!Light) continue;
    //    if (SpotLightIndex >= 16) break; // 최대 16개 제한

    //    // TODO: Spot Light Shadow Map RTV/DSV 설정 (Atlas 타일별)
    //    // SpotShadowViewport.TopLeftX = (SpotLightIndex % 4) * 1024.0f;
    //    // SpotShadowViewport.TopLeftY = (SpotLightIndex / 4) * 1024.0f;
    //    // DeviceContext->RSSetViewports(1, &SpotShadowViewport);
    //    
    //    // Light View Projection Matrix 계산
    //    FVector LightPos = Light->GetWorldLocation();
    //    FVector LightDir = Light->GetForwardVector();
    //    // TODO: Light 관점에서 View/Projection Matrix 설정
    //    
    //    // 모든 Static Mesh를 Light 관점에서 렌더링
    //    for (auto MeshComp : Context.StaticMeshes)
    //    {
    //        if (!MeshComp || !MeshComp->IsVisible()) continue;
    //        RenderPrimitive(MeshComp);
    //    }

    //    SpotLightIndex++;
    //}
    //*/

    //// ============================================================================
    //// TODO: Point Light Shadow Map 렌더링 (Cube Map - 6 faces) (나중에 구현)
    //// ============================================================================
    ///*
    //int PointLightIndex = 0;
    //for (auto Light : Context.PointLights)
    //{
    //    if (!Light) continue;
    //    if (PointLightIndex >= 16) break; // 최대 16개 제한

    //    FVector LightPos = Light->GetWorldLocation();

    //    // Cube map의 6개 face 방향
    //    static const FVector LookDirections[6] = {
    //        FVector(1, 0, 0),   // +X
    //        FVector(-1, 0, 0),  // -X
    //        FVector(0, 1, 0),   // +Y
    //        FVector(0, -1, 0),  // -Y
    //        FVector(0, 0, 1),   // +Z
    //        FVector(0, 0, -1)   // -Z
    //    };

    //    static const FVector UpDirections[6] = {
    //        FVector(0, 1, 0),   // +X face
    //        FVector(0, 1, 0),   // -X face
    //        FVector(0, 0, -1),  // +Y face
    //        FVector(0, 0, 1),   // -Y face
    //        FVector(0, 1, 0),   // +Z face
    //        FVector(0, 1, 0)    // -Z face
    //    };

    //    for (int Face = 0; Face < 6; ++Face)
    //    {
    //        // TODO: Point Light Shadow Map RTV/DSV 설정 (Cube map face별)
    //        // int CubeFaceIndex = PointLightIndex * 6 + Face;
    //        // DeviceContext->OMSetRenderTargets(1, &PointShadowRTVs[CubeFaceIndex], PointShadowDSVs[CubeFaceIndex]);
    //        // DeviceContext->RSSetViewports(1, &PointShadowViewport);
    //        
    //        // Light View Projection Matrix 계산
    //        // TODO: LookDirections[Face], UpDirections[Face]를 사용하여 View Matrix 계산
    //        
    //        // 모든 Static Mesh를 Light 관점에서 렌더링
    //        for (auto MeshComp : Context.StaticMeshes)
    //        {
    //            if (!MeshComp || !MeshComp->IsVisible()) continue;
    //            RenderPrimitive(MeshComp);
    //        }
    //    }

    //    PointLightIndex++;
    //}
    

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

    // Shadow Map 렌더링용 파이프라인 설정
    ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState(
        { ECullMode::Back, EFillMode::Solid });
    
    FPipelineInfo PipelineInfo = {
        ShadowMapInputLayout,
        ShadowMapVS,
        RS,
        URenderer::GetInstance().GetDefaultDepthStencilState(),
        ShadowMapPS,
        nullptr,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    };
    
    Pipeline->UpdatePipeline(PipelineInfo);
    Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);
    Pipeline->SetConstantBuffer(1, EShaderType::VS, LightCameraConstantBuffer);  // Light 전용 버퍼 사용

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
