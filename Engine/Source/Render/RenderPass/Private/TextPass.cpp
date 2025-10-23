#include "pch.h"
#include "Render/RenderPass/Public/TextPass.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Component/Public/TextComponent.h"
#include "Component/Public/UUIDTextComponent.h"
#include "Editor/Public/Camera.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Editor/Public/Editor.h"
#include "Texture/Public/Texture.h"

FTextPass::FTextPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel)
    : FRenderPass(InPipeline, InConstantBufferCamera, InConstantBufferModel)
{
    // Create shaders
    TArray<D3D11_INPUT_ELEMENT_DESC> LayoutDesc = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FFontVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FFontVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 0, offsetof(FFontVertex, CharIndex), D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    FRenderResourceFactory::CreateVertexShaderAndInputLayout(L"Asset/Shader/ShaderFont.hlsl", LayoutDesc, &FontVertexShader, &FontInputLayout);
    FRenderResourceFactory::CreatePixelShader(L"Asset/Shader/ShaderFont.hlsl", &FontPixelShader);

    // Create dynamic vertex buffer
    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth = sizeof(FFontVertex) * MAX_FONT_VERTICES;
    BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    URenderer::GetInstance().GetDevice()->CreateBuffer(&BufferDesc, nullptr, &DynamicVertexBuffer);

    // Create constant buffer
    FontDataConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FFontConstantBuffer>();

    // Load font texture
    UAssetManager& ResourceManager = UAssetManager::GetInstance();
    FontTexture = ResourceManager.LoadTexture("Data/Texture/DejaVu Sans Mono.png");
}

void FTextPass::Execute(FRenderingContext& Context)
{
    // Set up pipeline
    FPipelineInfo PipelineInfo = {};
    PipelineInfo.InputLayout = FontInputLayout;
    PipelineInfo.VertexShader = FontVertexShader;
    PipelineInfo.PixelShader = FontPixelShader;
    PipelineInfo.RasterizerState = FRenderResourceFactory::GetRasterizerState({ ECullMode::None, EFillMode::Solid });
    PipelineInfo.BlendState = URenderer::GetInstance().GetAlphaBlendState();
    PipelineInfo.DepthStencilState = URenderer::GetInstance().GetDefaultDepthStencilState(); // Or DisabledDepthStencilState based on a flag
    Pipeline->UpdatePipeline(PipelineInfo);
    if (!(Context.ShowFlags & EEngineShowFlags::SF_Text)) { return; }

    // Set constant buffers
    Pipeline->SetConstantBuffer(1, EShaderType::VS, ConstantBufferCamera);
    FRenderResourceFactory::UpdateConstantBufferData(FontDataConstantBuffer, ConstantBufferData);
    Pipeline->SetConstantBuffer(2, EShaderType::VS, FontDataConstantBuffer);

    // Bind resources
    Pipeline->SetShaderResourceView(0, EShaderType::PS, FontTexture->GetTextureSRV());
    Pipeline->SetSamplerState(0, EShaderType::PS, FontTexture->GetTextureSampler());

    for (UTextComponent* Text : Context.Texts)
    {
        RenderTextInternal(Text->GetText(), Text->GetWorldTransformMatrix());
    }

    // Render UUID
    if (!(Context.ShowFlags & EEngineShowFlags::SF_Billboard)) { return; }

    for (UUUIDTextComponent* PickedBillboard : Context.UUIDs)
    {
        if (PickedBillboard->GetOwner() != GEditor->GetEditorModule()->GetSelectedActor()) { continue; }
        PickedBillboard->UpdateRotationMatrix(Context.CurrentCamera->GetForward());
        FString UUIDString = "UID: " + std::to_string(PickedBillboard->GetUUID());
        RenderTextInternal(UUIDString, PickedBillboard->GetRTMatrix());
    }
}

void FTextPass::RenderTextInternal(const FString& Text, const FMatrix& WorldMatrix)
{
    if (Text.empty()) return;

    ID3D11DeviceContext* DeviceContext = URenderer::GetInstance().GetDeviceContext();

    size_t TextLength = Text.length();
    uint32 VertexCount = static_cast<uint32>(TextLength * 6);

    if (VertexCount > MAX_FONT_VERTICES) { return; }

    // Update vertex buffer
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    if (SUCCEEDED(DeviceContext->Map(DynamicVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
    {
        FFontVertex* Vertices = static_cast<FFontVertex*>(MappedResource.pData);
        float CurrentY = 0.0f - (TextLength * 1.0f) / 2.0f; // Assuming char width of 1.0f

        for (size_t Idx = 0; Idx < TextLength; ++Idx)
        {
            const char Ch = Text[Idx];
            const uint32 AsciiCode = static_cast<uint32>(Ch);

            // Simplified vertex generation, assuming fixed size
            const float Y = CurrentY + Idx * 1.0f;

            const float CharHeight = 2.0f;
            const float HalfHeight = CharHeight / 2.0f;

            const float BottomZ = -HalfHeight;
            const float TopZ = HalfHeight;     // +1.0f

            const FVector P0(0.0f, Y,        TopZ);
            const FVector P1(0.0f, Y + 1.0f, TopZ);
            const FVector P2(0.0f, Y,        BottomZ);
            const FVector P3(0.0f, Y + 1.0f, BottomZ);

            Vertices[Idx * 6 + 0] = { P0, FVector2(0.0f, 0.0f), AsciiCode };
            Vertices[Idx * 6 + 1] = { P1, FVector2(1.0f, 0.0f), AsciiCode };
            Vertices[Idx * 6 + 2] = { P2, FVector2(0.0f, 1.0f), AsciiCode };
            Vertices[Idx * 6 + 3] = { P1, FVector2(1.0f, 0.0f), AsciiCode };
            Vertices[Idx * 6 + 4] = { P3, FVector2(1.0f, 1.0f), AsciiCode };
            Vertices[Idx * 6 + 5] = { P2, FVector2(0.0f, 1.0f), AsciiCode };
        }
        DeviceContext->Unmap(DynamicVertexBuffer, 0);
    }

    // Update model constant buffer
    FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, WorldMatrix);
    Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

    // Set vertex buffer
    Pipeline->SetVertexBuffer(DynamicVertexBuffer, sizeof(FFontVertex));

    // Draw
    Pipeline->Draw(VertexCount, 0);
}

void FTextPass::Release()
{
    SafeRelease(FontVertexShader);
    SafeRelease(FontPixelShader);
    SafeRelease(FontInputLayout);
    SafeRelease(DynamicVertexBuffer);
    SafeRelease(FontDataConstantBuffer);
}