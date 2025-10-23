#include "pch.h"
#include "Render/Renderer/Public/Pipeline.h"


//if (ContainShaderType(ShaderType, EShaderType::VS))
//{
//	DeviceContext->VSSetConstantBuffers(Slot, 1, &ConstantBuffer);
//}
//if (ContainShaderType(ShaderType, EShaderType::PS))
//{
//	DeviceContext->VSSetConstantBuffers(Slot, 1, &ConstantBuffer);
//}
//if (ContainShaderType(ShaderType, EShaderType::CS))
//{
//	DeviceContext->VSSetConstantBuffers(Slot, 1, &ConstantBuffer);
//}
/// @brief 그래픽 파이프라인을 관리하는 클래스
UPipeline::UPipeline(ID3D11DeviceContext* InDeviceContext)
	: DeviceContext(InDeviceContext)
{
	LastPipelineInfo = {};
	//첫 프레임 메쉬 그릴때 기본으로 TriangleList topology 라서 Set안불리는 버그 수정용
	LastPipelineInfo.Topology = D3D11_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST; 
}

UPipeline::~UPipeline()
{
	// Device Context는 Device Resource에서 제거
}


/// @brief 파이프라인 상태를 업데이트
void UPipeline::UpdatePipeline(FPipelineInfo Info)
{
	if (LastPipelineInfo.Topology != Info.Topology) {
		DeviceContext->IASetPrimitiveTopology(Info.Topology);
		LastPipelineInfo.Topology = Info.Topology;
	}
	if (LastPipelineInfo.InputLayout != Info.InputLayout) {
		DeviceContext->IASetInputLayout(Info.InputLayout);
		LastPipelineInfo.InputLayout = Info.InputLayout;
	}
	if (LastPipelineInfo.VertexShader != Info.VertexShader) {
		DeviceContext->VSSetShader(Info.VertexShader, nullptr, 0);
		LastPipelineInfo.VertexShader = Info.VertexShader;
	}
	if (LastPipelineInfo.RasterizerState != Info.RasterizerState) {
		DeviceContext->RSSetState(Info.RasterizerState);
		LastPipelineInfo.RasterizerState = Info.RasterizerState;
	}
	if (Info.DepthStencilState) {
		DeviceContext->OMSetDepthStencilState(Info.DepthStencilState, 0);
	}
	if (LastPipelineInfo.PixelShader != Info.PixelShader) {
		DeviceContext->PSSetShader(Info.PixelShader, nullptr, 0);
		LastPipelineInfo.PixelShader = Info.PixelShader;
	}
	if (LastPipelineInfo.BlendState != Info.BlendState) {
		DeviceContext->OMSetBlendState(Info.BlendState, nullptr, 0xffffffff);
		LastPipelineInfo.BlendState = Info.BlendState;
	}
}

void UPipeline::SetIndexBuffer(ID3D11Buffer* indexBuffer, uint32 stride)
{
	DeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

/// @brief 정점 버퍼를 바인딩
void UPipeline::SetVertexBuffer(ID3D11Buffer* VertexBuffer, uint32 Stride)
{
	uint32 Offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
}

/// @brief 상수 버퍼를 설정
void UPipeline::SetConstantBuffer(uint32 Slot, EShaderType ShaderType, ID3D11Buffer* ConstantBuffer)
{
	if (ContainShaderType(ShaderType, EShaderType::VS))
	{
		DeviceContext->VSSetConstantBuffers(Slot, 1, &ConstantBuffer);
	}
	if (ContainShaderType(ShaderType, EShaderType::PS))
	{
		DeviceContext->PSSetConstantBuffers(Slot, 1, &ConstantBuffer);
	}
	if (ContainShaderType(ShaderType, EShaderType::CS))
	{
		DeviceContext->CSSetConstantBuffers(Slot, 1, &ConstantBuffer);
	}
}

/// @brief 텍스처를 설정
void UPipeline::SetShaderResourceView(uint32 Slot, EShaderType ShaderType, ID3D11ShaderResourceView* Srv)
{
	if (ContainShaderType(ShaderType, EShaderType::VS))
	{
		DeviceContext->VSSetShaderResources(Slot, 1, &Srv);
	}
	if (ContainShaderType(ShaderType, EShaderType::PS))
	{
		DeviceContext->PSSetShaderResources(Slot, 1, &Srv);
	}
	if (ContainShaderType(ShaderType, EShaderType::CS))
	{
		DeviceContext->CSSetShaderResources(Slot, 1, &Srv);
	}
}

//Only CS Possible
void UPipeline::SetUnorderedAccessView(uint32 Slot, ID3D11UnorderedAccessView* UAV)
{
	DeviceContext->CSSetUnorderedAccessViews(Slot, 1, &UAV, nullptr);
}

/// @brief 샘플러 상태를 설정
void UPipeline::SetSamplerState(uint32 Slot, EShaderType ShaderType, ID3D11SamplerState* SamplerState)
{
	if (ContainShaderType(ShaderType, EShaderType::VS))
	{
		DeviceContext->VSSetSamplers(Slot, 1, &SamplerState);
	}
	if (ContainShaderType(ShaderType, EShaderType::PS))
	{
		DeviceContext->PSSetSamplers(Slot, 1, &SamplerState);
	}
	if (ContainShaderType(ShaderType, EShaderType::CS))
	{
		DeviceContext->CSSetSamplers(Slot, 1, &SamplerState);
	}
}



void UPipeline::SetRenderTargets(uint32 NumViews, ID3D11RenderTargetView* const* RenderTargetViews,
                                 ID3D11DepthStencilView* DepthStencilView)
{
	DeviceContext->OMSetRenderTargets(NumViews, RenderTargetViews, DepthStencilView);
}

/// @brief 정점 개수를 기반으로 드로우 호출
void UPipeline::Draw(uint32 VertexCount, uint32 StartLocation)
{
	DeviceContext->Draw(VertexCount, StartLocation);
}

void UPipeline::DrawIndexed(uint32 IndexCount, uint32 StartIndexLocation, int32 BaseVertexLocation)
{
	DeviceContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}

void UPipeline::DispatchCS(ID3D11ComputeShader* CS, uint32 x, uint32 y, uint32 z)
{
	DeviceContext->CSSetShader(CS, nullptr, 0);
	DeviceContext->Dispatch(x, y, z);
}
