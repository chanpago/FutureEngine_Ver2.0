#pragma once

enum class EShaderType
{
	VS = 1 << 0,
	PS = 1 << 1,
	CS = 1 << 2,
};
inline EShaderType operator|(EShaderType a, EShaderType b)
{
	return static_cast<EShaderType>(static_cast<int>(a) | static_cast<int>(b));
}
inline bool ContainShaderType(EShaderType a, EShaderType b)
{
	return (static_cast<int>(a) & static_cast<int>(b)) > 0;
}


struct FPipelineInfo
{
	ID3D11InputLayout* InputLayout;
	ID3D11VertexShader* VertexShader;
	ID3D11RasterizerState* RasterizerState;
	ID3D11DepthStencilState* DepthStencilState;
	ID3D11PixelShader* PixelShader;
	ID3D11BlendState* BlendState;
	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};

class UPipeline
{
public:
	UPipeline(ID3D11DeviceContext* InDeviceContext);
	~UPipeline();

	void UpdatePipeline(FPipelineInfo Info);

	void SetIndexBuffer(ID3D11Buffer* indexBuffer, uint32 stride);

	void SetVertexBuffer(ID3D11Buffer* VertexBuffer, uint32 Stride);

	void SetConstantBuffer(uint32 Slot, EShaderType ShaderType, ID3D11Buffer* ConstantBuffer);

	void SetShaderResourceView(uint32 Slot, EShaderType ShaderType, ID3D11ShaderResourceView* Srv);

	void SetUnorderedAccessView(uint32 Slot, ID3D11UnorderedAccessView* UAV);

	void SetSamplerState(uint32 Slot, EShaderType ShaderType, ID3D11SamplerState* SamplerState);

	/** @todo This function is temporarily introduced for point light. */
	void SetRenderTargets(uint32 NumViews, ID3D11RenderTargetView* const *RenderTargetViews, ID3D11DepthStencilView* DepthStencilView);

	void Draw(uint32 VertexCount, uint32 StartLocation);

	void DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, int32 BaseVertexLocation);

	void DispatchCS(ID3D11ComputeShader* CS, uint32 x, uint32 y = 1, uint32 z = 1);

private:
	FPipelineInfo LastPipelineInfo{};
	ID3D11DeviceContext* DeviceContext;
};
