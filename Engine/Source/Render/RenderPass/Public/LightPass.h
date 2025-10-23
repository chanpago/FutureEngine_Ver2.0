#pragma once
#include "Global/Vector.h"
#include "Render/RenderPass/Public/RenderPass.h"

struct FViewClusterInfo 
{
	FMatrix ProjectionInv;
	FMatrix ViewInv;
	FMatrix ViewMat;
	float ZNear;
	float ZFar;
	float Aspect;
	float fov;
};
struct FClusterSliceInfo
{
	uint32 ClusterSliceNumX;
	uint32 ClusterSliceNumY;
	uint32 ClusterSliceNumZ;
	uint32 LightMaxCountPerCluster;
	uint32 SpotLightIntersectOption;
	uint32 Orthographic; //0 = Perspective, 1 = Orthographic
	FVector2 padding;
};
struct FLightCountInfo
{
	uint32 PointLightCount;
	uint32 SpotLightCount;
	FVector2 padding;
};
struct FGizmoVertex
{
	FVector Pos;
	FVector4 Color;
};

class FLightPass : public FRenderPass
{
public:
	FLightPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, 
		ID3D11InputLayout* InGizmoInputLayout, ID3D11VertexShader* InGizmoVS, ID3D11PixelShader* InGizmoPS,
		ID3D11DepthStencilState* InGizmoDSS);
	void Execute(FRenderingContext& Context) override;
	void Release() override;

	void ClusterGizmoUpdate()
	{
		bClusterGizmoSet = false;
	}

	bool GetClusterGizmoRender() const { return bRenderClusterGizmo; }
	void SetClusterGizmoRender(bool b)
	{
		bRenderClusterGizmo = b;
	}
	bool GetSpotIntersectType() const { return bSpotIntersectOpti; }
	void SetSpotIntersectType(bool b)
	{
		bSpotIntersectOpti = b;
	}

	void SetVertexShader(ID3D11VertexShader* InGizmoVS) { GizmoVS = InGizmoVS; }
	void SetPixelShader(ID3D11PixelShader* InGizmoPS) { GizmoPS = InGizmoPS; }
	void SetInputLayout(ID3D11InputLayout* InGizmoInputLayout) { GizmoInputLayout = InGizmoInputLayout; }

	uint32 GetClusterSliceNumX() const { return ClusterSliceNumX; }
	uint32 GetClusterSliceNumY() const { return ClusterSliceNumY; }
	uint32 GetClusterSliceNumZ() const { return ClusterSliceNumZ; }
	void SetClusterSliceNumX(uint32 SliceNum) { ClusterSliceNumX = SliceNum; }
	void SetClusterSliceNumY(uint32 SliceNum) { ClusterSliceNumY = SliceNum; }
	void SetClusterSliceNumZ(uint32 SliceNum) { ClusterSliceNumZ = SliceNum; }
private:
	uint32 GetClusterCount() const { return ClusterSliceNumX * ClusterSliceNumY * ClusterSliceNumZ; }
	bool IsChangeOption();
	void CreateOptionBuffers();
	void ReleaseOptionBuffers();
public:
private:
	ID3D11ComputeShader* ViewClusterCS = nullptr;
	ID3D11ComputeShader* ClusteredLightCullingCS = nullptr;
	ID3D11ComputeShader* ClusterGizmoSetCS = nullptr;

	ID3D11Buffer* ViewClusterInfoConstantBuffer = nullptr;
	ID3D11Buffer* ClusterSliceInfoConstantBuffer = nullptr;
	ID3D11Buffer* LightCountInfoConstantBuffer = nullptr;
	ID3D11Buffer* GlobalLightConstantBuffer = nullptr;
	ID3D11Buffer* PointLightStructuredBuffer = nullptr;
	ID3D11Buffer* SpotLightStructuredBuffer = nullptr;
	ID3D11Buffer* ClusterAABBRWStructuredBuffer = nullptr;
	ID3D11Buffer* PointLightIndicesRWStructuredBuffer = nullptr;
	ID3D11Buffer* SpotLightIndicesRWStructuredBuffer = nullptr;

	ID3D11ShaderResourceView* PointLightStructuredBufferSRV = nullptr;
	ID3D11ShaderResourceView* SpotLightStructuredBufferSRV = nullptr;
	ID3D11ShaderResourceView* ClusterAABBRWStructuredBufferSRV = nullptr;
	ID3D11UnorderedAccessView* ClusterAABBRWStructuredBufferUAV = nullptr;
	ID3D11ShaderResourceView* PointLightIndicesRWStructuredBufferSRV = nullptr;
	ID3D11UnorderedAccessView* PointLightIndicesRWStructuredBufferUAV = nullptr;
	ID3D11ShaderResourceView* SpotLightIndicesRWStructuredBufferSRV = nullptr;
	ID3D11UnorderedAccessView* SpotLightIndicesRWStructuredBufferUAV = nullptr;
	
	uint32 PointLightBufferCount = 256;
	uint32 SpotLightBufferCount = 256;
	uint32 ClusterSliceNumX = 24;
	uint32 ClusterSliceNumY = 16;
	uint32 ClusterSliceNumZ = 32;
	uint32 LightMaxCountPerCluster = 32;


	ID3D11Buffer* ClusterGizmoVertexRWStructuredBuffer = nullptr;
	ID3D11UnorderedAccessView* ClusterGizmoVertexRWStructuredBufferUAV = nullptr;
	ID3D11ShaderResourceView* ClusterGizmoVertexRWStructuredBufferSRV = nullptr;

	//Renderer에서 참조해오고 Renderer에서 해제되므로 Release함수에서 해제x
	ID3D11Buffer* CameraConstantBuffer = nullptr;
	ID3D11InputLayout* GizmoInputLayout = nullptr;
	ID3D11VertexShader* GizmoVS = nullptr;
	ID3D11PixelShader* GizmoPS = nullptr;
	ID3D11DepthStencilState* GizmoDSS = nullptr;

	bool bRenderClusterGizmo = false;
	bool bClusterGizmoSet = false;
	bool bSpotIntersectOpti = true;





};