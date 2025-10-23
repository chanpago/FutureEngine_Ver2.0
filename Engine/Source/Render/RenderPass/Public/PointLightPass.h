#pragma once

#include "RenderPass.h"

struct FPointLightPerFrame
{
    FMatrix InvView;
    FMatrix InvProjection;
    FVector CameraLocation;
    float Padding; // Aligns CameraLocation to 16 bytes
    FVector4 Viewport;
    FVector2 RenderTargetSize;
    FVector2 Padding2; // Aligns the whole struct to a multiple of 16 bytes
};

struct FPointLightData
{
    FVector LightLocation;
    float LightIntensity;
    FVector LightColor;
    float LightRadius;
    float DistanceFalloffExponent;
    float Padding[3];
};

class FPointLightPass : public FRenderPass
{
public:
    FPointLightPass(UPipeline* InPipeline,
        ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout,
        ID3D11DepthStencilState* InDS, ID3D11BlendState* InBS);

    void Execute(FRenderingContext& Context) override;
    void Release() override;

	void SetVertexShader(ID3D11VertexShader* InVS) { VS = InVS; }
	void SetPixelShader(ID3D11PixelShader* InPS) { PS = InPS; }
	void SetInputLayout(ID3D11InputLayout* InLayout) { InputLayout = InLayout; }
    
private:
    ID3D11VertexShader* VS = nullptr;
    ID3D11PixelShader* PS = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11DepthStencilState* DS = nullptr;
    ID3D11BlendState* BS = nullptr;

    ID3D11Buffer* VertexBuffer;
    ID3D11Buffer* ConstantBufferPerFrame = nullptr;
    ID3D11Buffer* ConstantBufferPointLightData = nullptr;

    ID3D11SamplerState* PointLightSampler;
};
