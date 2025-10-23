#pragma once
#include "Render/RenderPass/Public/RenderPass.h"
#include "Component/Public/BillBoardComponent.h"

class FBillboardPass : public FRenderPass
{
public:
	FBillboardPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel,
        ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11DepthStencilState* InDS, ID3D11BlendState* InBS);
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
    ID3D11Buffer* ConstantBufferMaterial = nullptr;
    FMaterialConstants BillboardMaterialConstants;
};
