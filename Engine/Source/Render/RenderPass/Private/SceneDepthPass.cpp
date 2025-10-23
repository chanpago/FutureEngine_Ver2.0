#include "pch.h"
#include "Render/RenderPass/Public/SceneDepthPass.h"

#include "Editor/Public/Camera.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"

FSceneDepthPass::FSceneDepthPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11DepthStencilState* InDS)
    : FRenderPass(InPipeline, InConstantBufferCamera, nullptr), DS(InDS)
{
    TArray<D3D11_INPUT_ELEMENT_DESC> LayoutDesc = {};

    FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/SceneDepthShader.hlsl", LayoutDesc, &VertexShader, nullptr);
    FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/SceneDepthShader.hlsl", &PixelShader);

    SamplerState = FRenderResourceFactory::CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
    ConstantBufferPerFrame = FRenderResourceFactory::CreateConstantBuffer<FSceneDepthConstants>();
}

void FSceneDepthPass::Execute(FRenderingContext& Context)
{
    if (Context.ViewMode != EViewModeIndex::VMI_SceneDepth) { return; }
    
	const auto& Renderer = URenderer::GetInstance();
    const auto& DeviceResources = Renderer.GetDeviceResources();
    ID3D11RenderTargetView* RTV = Renderer.GetFXAA() ? DeviceResources->GetSceneColorRenderTargetView() : DeviceResources->GetRenderTargetView();
    
    ID3D11RenderTargetView* RTVs[] = { RTV };
    Pipeline->SetRenderTargets(1, RTVs, nullptr);
    auto RS = FRenderResourceFactory::GetRasterizerState( { ECullMode::None, EFillMode::Solid }); 

    FPipelineInfo PipelineInfo = { nullptr, VertexShader, RS, DS, PixelShader, nullptr, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
    Pipeline->UpdatePipeline(PipelineInfo);

    FSceneDepthConstants SceneDepthConstants;
    SceneDepthConstants.RenderTarget = FVector2(Context.RenderTargetSize.X, Context.RenderTargetSize.Y);
    SceneDepthConstants.IsOrthographic = Context.CurrentCamera->GetCameraType() == ECameraType::ECT_Orthographic;
    FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferPerFrame, SceneDepthConstants);
    Pipeline->SetConstantBuffer(0, EShaderType::PS, ConstantBufferPerFrame);
    Pipeline->SetConstantBuffer(1, EShaderType::PS, ConstantBufferCamera);
    Pipeline->SetShaderResourceView(0, EShaderType::PS, DeviceResources->GetDepthSRV());
    Pipeline->SetSamplerState(0, EShaderType::PS, SamplerState);

    Pipeline->Draw(3, 0);

    ID3D11DepthStencilView* DSV = DeviceResources->GetDepthStencilView();
    Pipeline->SetRenderTargets(1, RTVs, DSV);
}

void FSceneDepthPass::Release()
{
    SafeRelease(PixelShader);
    SafeRelease(VertexShader);
    SafeRelease(SamplerState);
    SafeRelease(ConstantBufferPerFrame);
}
