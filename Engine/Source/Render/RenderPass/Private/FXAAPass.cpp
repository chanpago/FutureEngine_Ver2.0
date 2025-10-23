#include "pch.h"
#include "Render/RenderPass/Public/FXAAPass.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/Renderer/Public/DeviceResources.h"

struct FFullscreenVertex
{
    FVector2 Position;
    FVector2 UV;
};

FFXAAPass::FFXAAPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources, ID3D11VertexShader* InVS,
    ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11SamplerState* InSampler)
    :FRenderPass(InPipeline,nullptr,nullptr)
    , DeviceResources(InDeviceResources)
    , VertexShader(InVS)
    , PixelShader(InPS)
    , InputLayout(InLayout)
    , SamplerState(InSampler)
{
    InitializeFullscreenQuad();
    FXAAConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FFXAAConstants>();
}

FFXAAPass::~FFXAAPass()
{
    Release();
}

void FFXAAPass::Execute(FRenderingContext& Context)
{
    ID3D11ShaderResourceView* SceneSRV = DeviceResources->GetSceneColorShaderResourceView(); // 오프스크린 컬러입력
    if (!SceneSRV)
    {
        return;
    }

    UpdateConstants();
    SetRenderTargets();

    FPipelineInfo PipelineInfo = {};
    PipelineInfo.InputLayout = InputLayout;
    PipelineInfo.VertexShader = VertexShader;
    PipelineInfo.PixelShader = PixelShader;
    PipelineInfo.DepthStencilState = nullptr;
    PipelineInfo.BlendState = nullptr;
    PipelineInfo.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    Pipeline->UpdatePipeline(PipelineInfo);

    UINT offset = 0;
    Pipeline->SetVertexBuffer(FullscreenVB, FullscreenStride);
    Pipeline->SetIndexBuffer(FullscreenIB, 0);

    Pipeline->SetConstantBuffer(0, EShaderType::PS, FXAAConstantBuffer);
    Pipeline->SetShaderResourceView(0, EShaderType::PS, SceneSRV);
    Pipeline->SetSamplerState(0, EShaderType::PS, SamplerState);

    Pipeline->DrawIndexed(FullscreenIndexCount, 0, 0);

    // 정리
    Pipeline->SetShaderResourceView(0, EShaderType::PS, nullptr);
}

void FFXAAPass::Release()
{
    SafeRelease(FullscreenVB);
    SafeRelease(FullscreenIB);
    SafeRelease(FXAAConstantBuffer);
}

void FFXAAPass::InitializeFullscreenQuad()
{
    static const FFullscreenVertex Vertices[] =
    {
        {{-1.f,  1.f}, {0.f, 0.f}},
        {{ 1.f,  1.f}, {1.f, 0.f}},
        {{ 1.f, -1.f}, {1.f, 1.f}},
        {{-1.f, -1.f}, {0.f, 1.f}},
    };

    static const uint32 Indices[] = { 0, 1, 2, 0, 2, 3 };

    FullscreenStride = sizeof(FFullscreenVertex);
    FullscreenIndexCount = static_cast<UINT>(sizeof(Indices) / sizeof(Indices[0]));

    D3D11_BUFFER_DESC VBDesc = {};
    VBDesc.ByteWidth = sizeof(Vertices);
    VBDesc.Usage = D3D11_USAGE_IMMUTABLE;
    VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA VBData = {};
    VBData.pSysMem = Vertices;

    DeviceResources->GetDevice()->CreateBuffer(&VBDesc, &VBData, &FullscreenVB);

    D3D11_BUFFER_DESC IBDesc = {};
    IBDesc.ByteWidth = sizeof(Indices);
    IBDesc.Usage = D3D11_USAGE_IMMUTABLE;
    IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA IBData = {};
    IBData.pSysMem = Indices;

    DeviceResources->GetDevice()->CreateBuffer(&IBDesc, &IBData, &FullscreenIB);
}

void FFXAAPass::UpdateConstants()
{
    const D3D11_VIEWPORT& VP = DeviceResources->GetViewportInfo();
    FXAAParams.InvResolution = FVector2(1.0f / VP.Width, 1.0f / VP.Height);

    // FXAA 품질 설정값을 명시적으로 업데이트                                           
    FXAAParams.FXAASpanMax = 8.0f;                                        
    FXAAParams.FXAAReduceMul = 1.0f / 8.0f;                               
    FXAAParams.FXAAReduceMin = 1.0f / 128.0f;
    
    FRenderResourceFactory::UpdateConstantBufferData(FXAAConstantBuffer, FXAAParams);
}

void FFXAAPass::SetRenderTargets()
{
    ID3D11RenderTargetView* RTV = DeviceResources->GetRenderTargetView(); // 스왑체인 RTV
    DeviceResources->GetDeviceContext()->OMSetRenderTargets(1, &RTV, nullptr);

    const D3D11_VIEWPORT& VP = DeviceResources->GetViewportInfo();
    DeviceResources->GetDeviceContext()->RSSetViewports(1, &VP);
}
