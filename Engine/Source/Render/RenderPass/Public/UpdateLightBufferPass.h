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
    // Spot light shadow matrices (for single caster)
    FMatrix GetSpotLightViewMatrix() const { return SpotLightView; }
    FMatrix GetSpotLightProjectionMatrix() const { return SpotLightProj; }
    // Spot shadow atlas resources
    ID3D11ShaderResourceView* GetSpotShadowAtlasSRV() const { return SpotShadowAtlasSRV; }
    uint32 GetSpotAtlasCols() const { return SpotAtlasCols; }
    uint32 GetSpotAtlasRows() const { return SpotAtlasRows; }
    float  GetSpotTileWidth() const { return SpotShadowViewport.Width; }
    float  GetSpotTileHeight() const { return SpotShadowViewport.Height; }
	FMatrix GetCachedEyeView() const { return CachedEyeView; }
	FMatrix GetCachedEyeProj() const { return CachedEyeProj; }

	FVector4 GetLightOrthoLTRB() const {return LightOrthoLTRB;}

	// Calculate Cascade split distance
	void CalculateCascadeSplits(FVector4& OutSplits, const UCamera* InCamera);
	const FShadowMapConstants& GetCascadedShadowMapConstants() const { return CascadedShadowMapConstants; }

	struct FShadowCalculationData
	{
	    TArray<FMatrix> LightViews;
	    TArray<FMatrix> LightProjs;
	    FVector4        LightOrthoParams; // For EShadowProjectionType::None
		FVector4		CascadeSplits;	  // For EShadowProjectionType::CSM
	};

private:
	void NewBakeShadowMap(FRenderingContext& Context);
    void BakeSpotShadowMap(FRenderingContext& Context);
    void BakePointShadowMap(FRenderingContext& Context);
	
	void BakeShadowMap(FRenderingContext& Context);
	void RenderPrimitive(class UStaticMeshComponent* MeshComp);
	
	// PSM for SpotLight: transform light to post-perspective space and setup shadow
	void BuildSpotLightPSM(const FMatrix& EyeView, const FMatrix& EyeProj,
		const FVector& SpotLightPosWS, const FVector& SpotLightDirWS,
		float SpotOuterAngle, int ShadowMapWidth, int ShadowMapHeight,
		FMatrix& OutLightView, FMatrix& OutLightProj, FMatrix& OutPSMMatrix);

	// Helper Functions
	void CalculateShadowMatrices(EShadowProjectionType ProjType, FRenderingContext& Context, FShadowCalculationData& OutShadowData);
	void SetShadowRenderTarget(EShadowProjectionType ProjType, EShadowFilterType FilterType, int CascadeIndex);
	void UpdateShadowCasterConstants(EShadowProjectionType ProjType, const FShadowCalculationData& InShadowData, int CascadeIndex, FRenderingContext& Context);

	// Shadow Map Shaders
	ID3D11VertexShader* ShadowMapVS = nullptr;
	ID3D11PixelShader* ShadowMapPS = nullptr;
	ID3D11InputLayout* ShadowMapInputLayout = nullptr;
	ID3D11Buffer* LightCameraConstantBuffer = nullptr;
	ID3D11Buffer* PSMConstantBuffer = nullptr;
	// Spot shadow atlas structured buffer (per-spot view/proj + atlas transform)
    ID3D11Buffer* SpotShadowAtlasStructuredBuffer = nullptr;
    ID3D11ShaderResourceView* SpotShadowAtlasSRV = nullptr;
    // Point shadow: mapping from global point light index -> cube array index (or 0xFFFFFFFF if not shadowed)
    ID3D11Buffer* PointShadowCubeIndexStructuredBuffer = nullptr;
    ID3D11ShaderResourceView* PointShadowCubeIndexSRV = nullptr;
public:
    ID3D11ShaderResourceView* GetPointShadowCubeIndexSRV() const { return PointShadowCubeIndexSRV; }

	// Shadow Map Viewports
	D3D11_VIEWPORT DirectionalShadowViewport;
	D3D11_VIEWPORT SpotShadowViewport;
	D3D11_VIEWPORT PointShadowViewport;
	// Atlas layout for spot shadows
	uint32 SpotAtlasCols = 4;
	uint32 SpotAtlasRows = 4;

    FMatrix LightViewP;
    FMatrix LightProjP;
    FMatrix SpotLightView;
    FMatrix SpotLightProj;
	FMatrix CachedEyeView;  // PSM 베이킹 시 사용한 카메라 V_e
	FMatrix CachedEyeProj;  // PSM 베이킹 시 사용한 카메라 P_e
	
	FVector4 LightOrthoLTRB = FVector4(-1, 1, -1, 1);

	// For Cascade
	FShadowMapConstants CascadedShadowMapConstants;
};
