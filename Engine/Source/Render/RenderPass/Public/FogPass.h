#pragma once
#include "Render/RenderPass/Public/RenderPass.h"

// Matches the layout in DecalShader.hlsl

struct FFogConstants
{
    FVector4 FogColor;
    float FogDensity;
    float FogHeightFalloff;
    float StartDistance;
    float FogCutoffDistance;
    float FogMaxOpacity;
    float FogZ;
    float Padding[2];
};

struct FCameraInverseConstants
{
    FMatrix ViewInverse;
    FMatrix ProjectionInverse;
};

struct FViewportConstants
{
    FVector2 RenderTargetSize;
    float Padding[2];
};

class FFogPass : public FRenderPass
{
public:
	FFogPass(
        UPipeline* InPipeline,
        ID3D11Buffer* InConstantBufferViewProj,
        ID3D11VertexShader* InVS,
        ID3D11PixelShader* InPS,
        ID3D11InputLayout* InLayout,
        ID3D11DepthStencilState* InDS_Read,
        ID3D11BlendState* InBlendState);
    
    void Execute(FRenderingContext& Context) override;
    void Release() override;

	void SetVertexShader(ID3D11VertexShader* InVS) { VS = InVS; }
	void SetPixelShader(ID3D11PixelShader* InPS) { PS = InPS; }
	void SetInputLayout(ID3D11InputLayout* InLayout) { InputLayout = InLayout; }

private:
    ID3D11VertexShader* VS = nullptr;
    ID3D11PixelShader* PS = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11DepthStencilState* DS_Read = nullptr;
    ID3D11BlendState* BlendState = nullptr;

    ID3D11Buffer* ConstantBufferFog = nullptr;
    ID3D11Buffer* ConstantBufferCameraInverse = nullptr;
    ID3D11Buffer* ConstantBufferViewportInfo = nullptr;
};
