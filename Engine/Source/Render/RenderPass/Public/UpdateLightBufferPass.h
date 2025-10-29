#pragma once
#include "Render/RenderPass/Public/RenderPass.h"
#include <unordered_map>

class UDirectionalLightComponent;
class USpotLightComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * @brief Point Light Shadow Tier Mapping Structure
 * Maps global point light index to tier and tier-local cube index
 */
struct FPointShadowTierMapping
{
    uint32 Tier;           // 0=Low(512), 1=Mid(1024), 2=High(2048), 0xFFFFFFFF=no shadow
    uint32 TierLocalIndex; // 0-7 within the tier's cube array
};

/**
 * @brief Spot Light Shadow Atlas Layout (3-Tier System)
 * Multi-resolution atlas supporting 24 total spot lights
 */
struct FSpotShadowAtlasLayout
{
    // High Tier: 2048x2048, 8 lights max, Scale 1.51~4.0
    static constexpr UINT HighTileSize = 2048;
    static constexpr UINT HighTilesX = 4;
    static constexpr UINT HighTilesY = 2;
    static constexpr UINT HighOffsetY = 0;
    static constexpr UINT MaxHighLights = 8;

    // Mid Tier: 1024x1024, 8 lights max, Scale 0.76~1.5
    static constexpr UINT MidTileSize = 1024;
    static constexpr UINT MidTilesX = 8;
    static constexpr UINT MidTilesY = 1;
    static constexpr UINT MidOffsetY = 4096;
    static constexpr UINT MaxMidLights = 8;

    // Low Tier: 512x512, 8 lights max, Scale 0.25~0.75
    static constexpr UINT LowTileSize = 512;
    static constexpr UINT LowTilesX = 8;
    static constexpr UINT LowTilesY = 1;
    static constexpr UINT LowOffsetY = 5120;
    static constexpr UINT MaxLowLights = 8;

    static constexpr UINT AtlasWidth = 8192;
    static constexpr UINT AtlasHeight = 8192;

    // Tier classification thresholds (same as Point Light system)
    static constexpr float LowThreshold = 0.75f;
    static constexpr float MidThreshold = 1.5f;
};

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
	FMatrix GetCachedLisPSMMatrix() const { return CachedLisPSMMatrix; }  // EyeSpace -> LightClip for LiSPSM

	FVector4 GetLightOrthoLTRB() const {return LightOrthoLTRB;}

	// Calculate Cascade split distance
    void CalculateCascadeSplits(FRenderingContext& Context, float* OutSplits, const UCamera* InCamera);
	const FShadowMapConstants& GetCascadedShadowMapConstants() const { return CascadedShadowMapConstants; }

	// LiSPSM for DirectionalLight: Light-Space Perspective Shadow Mapping (PUBLIC for StaticMeshPass)
	void BuildDirectionalLightLiSPSM(const FMatrix& EyeView, const FMatrix& EyeProj,
		const FVector& LightDirWS, int ShadowMapWidth, int ShadowMapHeight,
		FMatrix& OutLightView, FMatrix& OutLightProj, FMatrix& OutPSMMatrix);

	struct FShadowCalculationData
	{
	    TArray<FMatrix> LightViews;
	    TArray<FMatrix> LightProjs;
	    FVector4        LightOrthoParams; // For EShadowProjectionType::None
		float CascadeSplits[MAX_CASCADES];    // For EShadowProjectionType::CSM
FMatrix			LisPSM;	
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

    // CPU-side mapping for UI and debug
    bool GetPointCubeIndexCPU(const UPointLightComponent* Comp, uint32& OutCubeIdx) const;

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
	FMatrix CachedLisPSMMatrix;  // LiSPSM: EyeSpace -> LightClip transform
	
	FVector4 LightOrthoLTRB = FVector4(-1, 1, -1, 1);

	// For Cascade
    FShadowMapConstants CascadedShadowMapConstants;

    // CPU-side: map component pointer to cube index
    std::unordered_map<const UPointLightComponent*, uint32> PointCubeIndexCPU;
};
