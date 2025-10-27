#pragma once
#include "Render/RenderPass/Public/RenderPass.h"

class FStaticMeshPass : public FRenderPass
{
public:
    FStaticMeshPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferViewProj, ID3D11Buffer* InConstantBufferModel,
        ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11DepthStencilState* InDS);
    void Execute(FRenderingContext& Context) override;
    void RenderStaticMeshes(FRenderingContext& Context);
    void Release() override;

    // hot reload용 setter methods
	void SetVertexShader(ID3D11VertexShader* InVS) { VS = InVS; } 
	void SetPixelShader(ID3D11PixelShader* InPS) { PS = InPS; }
	void SetInputLayout(ID3D11InputLayout* InLayout) { InputLayout = InLayout; }

private:
    ID3D11VertexShader* VS = nullptr;
    ID3D11PixelShader* PS = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11DepthStencilState* DS = nullptr;
    
    ID3D11Buffer* ConstantBufferMaterial = nullptr;
    ID3D11Buffer* ConstantBufferShadowMap = nullptr;
    ID3D11Buffer* ConstantBufferSpotShadow = nullptr;
    ID3D11Buffer* ConstantBufferPointShadow = nullptr;

};
