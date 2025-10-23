#include "pch.h"
#include "Render/RenderPass/Public/ClusteredRenderingGridPass.h"

#include "Editor/Public/Camera.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/RenderPass/Public//FogPass.h"
FClusteredRenderingGridPass::FClusteredRenderingGridPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferViewProj,
    ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11DepthStencilState* InDS_Read,
    ID3D11BlendState* InBlendState)
    : FRenderPass(InPipeline, InConstantBufferViewProj, nullptr),
    VS(InVS), PS(InPS), InputLayout(InLayout), DS_Read(InDS_Read), BlendState(InBlendState)
{
    ConstantBufferViewportInfo = FRenderResourceFactory::CreateConstantBuffer<FViewportConstants>();

}

void FClusteredRenderingGridPass::Execute(FRenderingContext& Context)
{
    if (bClusteredRenderginGridRender == false)
    {
        return;
    }
    TIME_PROFILE(FogPass)

    //--- Get Renderer Singleton ---//
    URenderer& Renderer = URenderer::GetInstance();

    //--- Detatch DSV from GPU ---//
    ID3D11RenderTargetView* RTV;
    if (!(Context.ShowFlags & EEngineShowFlags::SF_FXAA))
    {
        RTV = Renderer.GetDeviceResources()->GetRenderTargetView();
    }
    else
    {
        RTV = Renderer.GetDeviceResources()->GetSceneColorRenderTargetView();
    }
    auto* DSV = Renderer.GetDeviceResources()->GetDepthStencilView();
    Renderer.GetDeviceContext()->OMSetRenderTargets(1, &RTV, nullptr);

    // --- Set Pipeline State --- //
    FPipelineInfo PipelineInfo = { InputLayout, VS, FRenderResourceFactory::GetRasterizerState({ ECullMode::Back, EFillMode::Solid }),
        DS_Read, PS, BlendState };
    Pipeline->UpdatePipeline(PipelineInfo);

    // Update ViewportInfo Constant Buffer (Slot 2)
    FViewportConstants ViewportConstants;
    ViewportConstants.RenderTargetSize = { Context.RenderTargetSize.X, Context.RenderTargetSize.Y };
    FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferViewportInfo, ViewportConstants);
    Pipeline->SetConstantBuffer(2, EShaderType::PS, ConstantBufferViewportInfo);

    Pipeline->Draw(3, 0);
    Pipeline->SetShaderResourceView(0, EShaderType::PS, nullptr);

    Renderer.GetDeviceContext()->OMSetRenderTargets(1, &RTV, DSV);
}

void FClusteredRenderingGridPass::Release()
{
    SafeRelease(ConstantBufferViewportInfo);
}
