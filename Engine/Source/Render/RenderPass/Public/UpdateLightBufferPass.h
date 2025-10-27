#pragma once
#include "Render/RenderPass/Public/RenderPass.h"

class UDirectionalLightComponent;
class USpotLightComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * @brief Shadow Map을 베이킹하는 RenderPass
 * Light의 관점에서 장면을 렌더링하여 Shadow Map 텍스처를 생성합니다.
 */
class FUpdateLightBufferPass : public FRenderPass
{
public:
	FUpdateLightBufferPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel,
		ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout);
	virtual ~FUpdateLightBufferPass() override;

	virtual void Execute(FRenderingContext& Context) override;
	virtual void Release() override;

	FMatrix GetLightViewMatrix() const { return LightViewP; }
	FMatrix GetLightProjectionMatrix() const { return LightProjP; }
	FMatrix GetCachedEyeView() const { return CachedEyeView; }
	FMatrix GetCachedEyeProj() const { return CachedEyeProj; }

	FVector4 GetLightOrthoLTRB() const {return LightOrthoLTRB;}

	// Calculate Cascade split distance
	void CalculateCascadeSplits(FVector4& OutSplits, const UCamera* InCamera);
	const FShadowMapConstants& GetCascadedShadowMapConstants() const { return CascadedShadowMapConstants; }

private:
    void BakeShadowMap(FRenderingContext& Context);
    void RenderPrimitive(class UStaticMeshComponent* MeshComp);

    // Refactored helpers (directional)
    void RenderDirectionalCSM(FRenderingContext& Context);
    void RenderDirectionalShadowSimple(FRenderingContext& Context);

    // Begin/End helpers for shadow-bake pass state
    void BeginShadowBake(ID3D11DeviceContext* DeviceContext,
                         D3D11_VIEWPORT& OutOriginalViewport,
                         ID3D11RenderTargetView*& OutOriginalRTV,
                         ID3D11DepthStencilView*& OutOriginalDSV);
    void EndShadowBake(ID3D11DeviceContext* DeviceContext,
                       const D3D11_VIEWPORT& OriginalViewport);

    // Future extension points (stubs): spot/point
    // void RenderSpotShadows(FRenderingContext& Context);
    // void RenderPointShadows(FRenderingContext& Context);

    // Shared small utilities (primarily for directional now; reusable for spot/point later)
    void ComputeSceneWorldAABB(const FRenderingContext& Context, FVector& OutMin, FVector& OutMax) const;
    void BindDirectionalShadowTargetsAndClear(ID3D11DeviceContext* DeviceContext, ID3D11DepthStencilView*& OutDSV, ID3D11RenderTargetView*& OutRTV) const;
	
    FMatrix BuildDirectionalLightViewMatrix(const UDirectionalLightComponent* Light, const FVector& LightPosition) const;
	
    FMatrix OrthoRowLH(float l, float r, float b, float t, float zn, float zf) const;
    void ComputeDirectionalOrthoBounds(const FMatrix& LightView, const FVector& SceneMin, const FVector& SceneMax,
    	float& OutMinX, float& OutMaxX, float& OutMinY, float& OutMaxY, float& OutMinZ, float& OutMaxZ) const;
	
    void SnapDirectionalBoundsToTexelGrid(const FMatrix& LightView, const FVector& SceneMin, const FVector& SceneMax,
    	float& InOutMinX, float& InOutMaxX, float& InOutMinY, float& InOutMaxY, const D3D11_VIEWPORT& Viewport) const;
	
    void UpdateDirectionalLVPConstants(const UDirectionalLightComponent* Light, const FMatrix& LightView, const FMatrix& LightProj,
    	float l, float r, float b, float t, const D3D11_VIEWPORT& Viewport);
    void DrawAllStaticReceivers(const FRenderingContext& Context);

    // PSM helpers (reusable for other light types later)
    void ComputeReceiverNDCBox(const FRenderingContext& Context, const FMatrix& CameraView, const FMatrix& CameraProj,
    	FVector& OutNdcMin, FVector& OutNdcMax) const;
    FMatrix BuildLookAtRowLH(const FVector& Eye, const FVector& At, const FVector& UpHint) const;
    void FitLightToNDCBox(const FVector& LightDirWorld, const FVector& ndcMin, const FVector& ndcMax,
    	const FMatrix& CameraView, const FMatrix& CameraProj, FMatrix& OutV, FMatrix& OutP, FVector4& OutLTRB,
    	float& OutNear, float& OutFar) const;
    void ApplyTexelSnappingToLightView(float& InOutL, float& InOutR, float& InOutB, float& InOutT,
    	float shadowMapWidth, float shadowMapHeight) const;

    // Packs PSM constants for directional light (updates member cached matrices as well)
    void UpdateDirectionalPSMConstants(const FRenderingContext& Context,
                                       const UDirectionalLightComponent* Light,
                                       const D3D11_VIEWPORT& Viewport);

	// Shadow Map Shaders
	ID3D11VertexShader* ShadowMapVS = nullptr;
	ID3D11PixelShader* ShadowMapPS = nullptr;
	ID3D11InputLayout* ShadowMapInputLayout = nullptr;
	ID3D11Buffer* LightCameraConstantBuffer = nullptr;
	ID3D11Buffer* PSMConstantBuffer = nullptr;

	// Shadow Map Viewports
	D3D11_VIEWPORT DirectionalShadowViewport;
	D3D11_VIEWPORT SpotShadowViewport;
	D3D11_VIEWPORT PointShadowViewport;

	FMatrix LightViewP;
	FMatrix LightProjP;
	FMatrix CachedEyeView;  // PSM 베이킹 시 사용한 카메라 V_e
	FMatrix CachedEyeProj;  // PSM 베이킹 시 사용한 카메라 P_e
	
	FVector4 LightOrthoLTRB = FVector4(-1, 1, -1, 1);

	// For Cascade
	FShadowMapConstants CascadedShadowMapConstants;
};
