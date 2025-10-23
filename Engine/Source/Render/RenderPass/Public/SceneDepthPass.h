#pragma once
#include "RenderPass.h"

struct FSceneDepthConstants
{
    FVector2 RenderTarget;
    int32 IsOrthographic;
};

class FSceneDepthPass : public FRenderPass
{
public:
    FSceneDepthPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11DepthStencilState* InDS);

    void Execute(FRenderingContext& Context) override;
    void Release() override;

private:
    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;

    ID3D11DepthStencilState* DS = nullptr;
    ID3D11SamplerState* SamplerState = nullptr;
    ID3D11Buffer* ConstantBufferPerFrame = nullptr;
};
