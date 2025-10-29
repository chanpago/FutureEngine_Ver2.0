#pragma once

class UDeviceResources
{
public:
	UDeviceResources(HWND InWindowHandle);
	~UDeviceResources();

	void Create(HWND InWindowHandle);
	void Release();

	void CreateDeviceAndSwapChain(HWND InWindowHandle);
	void ReleaseDeviceAndSwapChain();
	void CreateFrameBuffer();
	void ReleaseFrameBuffer();
	void CreateNormalBuffer();
	void ReleaseNormalBuffer();
	void CreateDepthBuffer();
	void ReleaseDepthBuffer();
	void CreateGizmoDepthBuffer();
	void ReleaseGizmoDepthBuffer();

	// Scene Color Texture, rtv, srv
	void CreateSceneColorTarget();
	void ReleaseSceneColorTarget();

	// Shadow Map Resources
	void CreateShadowMapResources();
	void ReleaseShadowMapResources();
	void CreateSpotShadowMapResources();
	void ReleaseSpotShadowMapResources();
	void CreatePointShadowCubeResources();
	void ReleasePointShadowCubeResources();

private:
	// Helper for creating a single shadow tier
	bool CreatePointShadowTier(
		UINT Resolution,
		ID3D11Texture2D** OutDepthTexture,
		ID3D11ShaderResourceView** OutDepthSRV,
		ID3D11ShaderResourceView** OutDepth2DArraySRV,
		ID3D11DepthStencilView** OutDSVs,
		ID3D11Texture2D** OutColorTexture,
		ID3D11RenderTargetView** OutRTVs,
		ID3D11ShaderResourceView** OutColorSRV);

public:

	// CSM Resources
	void CreateCascadedShadowMap();
	void ReleaseCascadedShadowMap();
	
	// Direct2D/DirectWrite
	void CreateFactories();
	void ReleaseFactories();

	ID3D11Device* GetDevice() const { return Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }
	IDXGISwapChain* GetSwapChain() const { return SwapChain; }
	ID3D11RenderTargetView* GetRenderTargetView() const { return FrameBufferRTV; }
	ID3D11RenderTargetView* GetNormalRenderTargetView() const { return NormalBufferRTV; }
	ID3D11DepthStencilView* GetDepthStencilView() const { return DepthStencilView; }

	ID3D11ShaderResourceView* GetSceneColorSRV() const { return FrameBufferSRV; }
	ID3D11ShaderResourceView* GetNormalSRV() const { return NormalBufferSRV; }
	ID3D11ShaderResourceView* GetDepthSRV() const { return DepthBufferSRV; }
	ID3D11ShaderResourceView* GetDepthStencilSRV() const { return DepthStencilSRV; }

	// Shadow Map Getters (returns resources from currently active tier)
	ID3D11DepthStencilView* GetDirectionalShadowMapDSV() const { return DirectionalShadowMapDSVs[DirectionalShadowActiveTier]; }
	ID3D11ShaderResourceView* GetDirectionalShadowMapSRV() const { return DirectionalShadowMapSRVs[DirectionalShadowActiveTier]; }
	ID3D11RenderTargetView* GetDirectionalShadowMapColorRTV() const { return DirectionalShadowMapColorRTVs[DirectionalShadowActiveTier]; }
	ID3D11ShaderResourceView* GetDirectionalShadowMapColorSRV() const { return DirectionalShadowMapColorSRVs[DirectionalShadowActiveTier]; }
	ID3D11Texture2D* GetDirectionalShadowMapColorTexture() const { return DirectionalShadowMapColorTextures[DirectionalShadowActiveTier]; }

	// Set active tier based on shadow resolution scale (0=Low:1024, 1=Mid:2048, 2=High:4096)
	void SetDirectionalShadowTier(float ShadowResolutionScale)
	{
		if (ShadowResolutionScale <= 0.75f)
			DirectionalShadowActiveTier = 0; // Low
		else if (ShadowResolutionScale <= 1.5f)
			DirectionalShadowActiveTier = 1; // Mid
		else
			DirectionalShadowActiveTier = 2; // High
	}
	uint32 GetDirectionalShadowResolution() const
	{
		static const uint32 TierResolutions[3] = { 1024, 2048, 4096 };
		return TierResolutions[DirectionalShadowActiveTier];
	}

    // Spot Light Shadow Map Getters
    ID3D11DepthStencilView* GetSpotShadowMapDSV() const { return SpotShadowMapDSV; }
    ID3D11ShaderResourceView* GetSpotShadowMapSRV() const { return SpotShadowMapSRV; }
    ID3D11RenderTargetView* GetSpotShadowMapColorRTV() const { return SpotShadowMapColorRTV; }
    ID3D11ShaderResourceView* GetSpotShadowMapColorSRV() const { return SpotShadowMapColorSRV; }

    // Point Light Shadow Cube Getters (3-Tier System)
    // Low Tier (512x512)
    ID3D11Texture2D* GetPointShadowLowTierTexture() const { return PointShadowLowTierTexture; }
    ID3D11ShaderResourceView* GetPointShadowLowTierSRV() const { return PointShadowLowTierSRV; }
    ID3D11ShaderResourceView* GetPointShadowLowTier2DArraySRV() const { return PointShadowLowTier2DArraySRV; }
    ID3D11ShaderResourceView* GetPointShadowLowTierColorSRV() const { return PointShadowLowTierColorSRV; }
    ID3D11DepthStencilView* GetPointShadowLowTierDSV(int SliceIndex) const { return (SliceIndex >= 0 && (UINT)SliceIndex < MaxLightsPerTier * 6) ? PointShadowLowTierDSVs[SliceIndex] : nullptr; }
    ID3D11RenderTargetView* GetPointShadowLowTierRTV(int SliceIndex) const { return (SliceIndex >= 0 && (UINT)SliceIndex < MaxLightsPerTier * 6) ? PointShadowLowTierRTVs[SliceIndex] : nullptr; }

    // Mid Tier (1024x1024)
    ID3D11Texture2D* GetPointShadowMidTierTexture() const { return PointShadowMidTierTexture; }
    ID3D11ShaderResourceView* GetPointShadowMidTierSRV() const { return PointShadowMidTierSRV; }
    ID3D11ShaderResourceView* GetPointShadowMidTier2DArraySRV() const { return PointShadowMidTier2DArraySRV; }
    ID3D11ShaderResourceView* GetPointShadowMidTierColorSRV() const { return PointShadowMidTierColorSRV; }
    ID3D11DepthStencilView* GetPointShadowMidTierDSV(int SliceIndex) const { return (SliceIndex >= 0 && (UINT)SliceIndex < MaxLightsPerTier * 6) ? PointShadowMidTierDSVs[SliceIndex] : nullptr; }
    ID3D11RenderTargetView* GetPointShadowMidTierRTV(int SliceIndex) const { return (SliceIndex >= 0 && (UINT)SliceIndex < MaxLightsPerTier * 6) ? PointShadowMidTierRTVs[SliceIndex] : nullptr; }

    // High Tier (2048x2048)
    ID3D11Texture2D* GetPointShadowHighTierTexture() const { return PointShadowHighTierTexture; }
    ID3D11ShaderResourceView* GetPointShadowHighTierSRV() const { return PointShadowHighTierSRV; }
    ID3D11ShaderResourceView* GetPointShadowHighTier2DArraySRV() const { return PointShadowHighTier2DArraySRV; }
    ID3D11ShaderResourceView* GetPointShadowHighTierColorSRV() const { return PointShadowHighTierColorSRV; }
    ID3D11DepthStencilView* GetPointShadowHighTierDSV(int SliceIndex) const { return (SliceIndex >= 0 && (UINT)SliceIndex < MaxLightsPerTier * 6) ? PointShadowHighTierDSVs[SliceIndex] : nullptr; }
    ID3D11RenderTargetView* GetPointShadowHighTierRTV(int SliceIndex) const { return (SliceIndex >= 0 && (UINT)SliceIndex < MaxLightsPerTier * 6) ? PointShadowHighTierRTVs[SliceIndex] : nullptr; }

    UINT GetMaxLightsPerTier() const { return MaxLightsPerTier; }

	ID3D11RenderTargetView* GetSceneColorRenderTargetView() const {return SceneColorTextureRTV; }
	ID3D11ShaderResourceView* GetSceneColorShaderResourceView() const{return SceneColorTextureSRV; }
	ID3D11Texture2D* GetSceneColorTexture() const {return SceneColorTexture; }

	// CSM Getters (returns resources from currently active tier, same as Directional)
	ID3D11ShaderResourceView* GetCascadedShadowMapSRV() const { return CascadedShadowMapSRVs[DirectionalShadowActiveTier]; }
	ID3D11ShaderResourceView* GetCascadedShadowMapColorSRV() const { return CascadedShadowMapColorSRVs[DirectionalShadowActiveTier]; }
	ID3D11ShaderResourceView* GetCascadedShadowMapSliceSRV(int CascadeIndex) const;
	ID3D11DepthStencilView* GetCascadedShadowMapDSV(int CascadeIndex) const;
	ID3D11RenderTargetView* GetCascadedShadowMapColorRTV(int CascadeIndex) const;
	
	ID3D11Texture2D* GetGizmoDepthTexture() const { return GizmoDepthTexture; }
	ID3D11DepthStencilView* GetGizmoDSV() const { return GizmoDSV; }

	const D3D11_VIEWPORT& GetViewportInfo() const { return ViewportInfo; }
	uint32 GetWidth() const { return Width; }
	uint32 GetHeight() const { return Height; }
	void UpdateViewport(float InMenuBarHeight = 0.f);

	// Direct2D/DirectWrite factory getters
	IDWriteFactory* GetDWriteFactory() const { return DWriteFactory; }

	void GetShadowMapMemoryUsage(float& OutDirectional, float& OutCSM, float& OutPoint, float& OutSpot) const;

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;
	ID3D11ShaderResourceView* FrameBufferSRV = nullptr;
	
	/** 
	 * This is introduced to support post-process point light effects
	 * without modifying the existing forward rendering pipeline.
	 * 
	 * @note This variable is temporary and intended to be removed 
	 * once a full deferred lighting system is implemented.
	 */
	ID3D11Texture2D* NormalBuffer = nullptr;
	ID3D11RenderTargetView* NormalBufferRTV = nullptr;
	ID3D11ShaderResourceView* NormalBufferSRV = nullptr;

	ID3D11Texture2D* DepthBuffer = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	ID3D11ShaderResourceView* DepthBufferSRV = nullptr;
	ID3D11ShaderResourceView* DepthStencilSRV = nullptr;

	ID3D11Texture2D* SceneColorTexture = nullptr;
	ID3D11RenderTargetView* SceneColorTextureRTV = nullptr;
	ID3D11ShaderResourceView* SceneColorTextureSRV = nullptr;

	// Directional Light Shadow Map (3-Tier System)
	// Tier 0: Low (1024x1024), Tier 1: Mid (2048x2048), Tier 2: High (4096x4096)
	ID3D11Texture2D* DirectionalShadowMapTextures[3] = { nullptr, nullptr, nullptr };
	ID3D11DepthStencilView* DirectionalShadowMapDSVs[3] = { nullptr, nullptr, nullptr };
	ID3D11ShaderResourceView* DirectionalShadowMapSRVs[3] = { nullptr, nullptr, nullptr };
	ID3D11RenderTargetView* DirectionalShadowMapColorRTVs[3] = { nullptr, nullptr, nullptr };
	ID3D11ShaderResourceView* DirectionalShadowMapColorSRVs[3] = { nullptr, nullptr, nullptr };
	ID3D11Texture2D* DirectionalShadowMapColorTextures[3] = { nullptr, nullptr, nullptr };
	uint32 DirectionalShadowActiveTier = 1; // Default: Mid tier (2048x2048)

    // Spot Light Shadow Map
    ID3D11Texture2D* SpotShadowMapTexture = nullptr;
    ID3D11DepthStencilView* SpotShadowMapDSV = nullptr;
    ID3D11ShaderResourceView* SpotShadowMapSRV = nullptr;
    // Spot Light Shadow Map (VSM color moments)
    ID3D11Texture2D* SpotShadowMapColorTexture = nullptr;
    ID3D11RenderTargetView* SpotShadowMapColorRTV = nullptr;
    ID3D11ShaderResourceView* SpotShadowMapColorSRV = nullptr;

	// CSM Resources (3-Tier System)
	// Tier 0: Low (1024), Tier 1: Mid (2048), Tier 2: High (4096)
	ID3D11Texture2D* CascadedShadowMapTextures[3] = { nullptr, nullptr, nullptr };
	ID3D11Texture2D* CascadedShadowMapColorTextures[3] = { nullptr, nullptr, nullptr };
	ID3D11ShaderResourceView* CascadedShadowMapSRVs[3] = { nullptr, nullptr, nullptr };
	ID3D11ShaderResourceView* CascadedShadowMapColorSRVs[3] = { nullptr, nullptr, nullptr };
	ID3D11ShaderResourceView* CascadedShadowMapSliceSRVs[3][MAX_CASCADES] = { { nullptr } };
	ID3D11DepthStencilView* CascadedShadowMapDSVs[3][MAX_CASCADES] = { { nullptr } };
	ID3D11RenderTargetView* CascadedShadowMapColorRTVs[3][MAX_CASCADES] = { { nullptr } };
	// Note: CSM uses same tier as Directional shadow (DirectionalShadowActiveTier)
	
	D3D11_VIEWPORT ViewportInfo = {};

	uint32 Width = 0;
	uint32 Height = 0;

	// Direct2D/DirectWrite factories
	ID2D1Factory* D2DFactory = nullptr;
	IDWriteFactory* DWriteFactory = nullptr;

	// For Axis Gizmo mesh depth-test
	ID3D11Texture2D* GizmoDepthTexture = nullptr;
	ID3D11DepthStencilView* GizmoDSV = nullptr;

	// Point Light Shadow 3-Tier System
	static const UINT MaxLightsPerTier = 8; // Max lights per resolution tier

	// Low Tier (512x512) - Scale 0.25~0.75
	ID3D11Texture2D* PointShadowLowTierTexture = nullptr;
	ID3D11ShaderResourceView* PointShadowLowTierSRV = nullptr;  // TextureCubeArray
	ID3D11ShaderResourceView* PointShadowLowTier2DArraySRV = nullptr;  // Texture2DArray for PCF
	ID3D11DepthStencilView* PointShadowLowTierDSVs[6 * MaxLightsPerTier] = { nullptr };
	ID3D11Texture2D* PointShadowLowTierColorTexture = nullptr;  // VSM
	ID3D11RenderTargetView* PointShadowLowTierRTVs[6 * MaxLightsPerTier] = { nullptr };
	ID3D11ShaderResourceView* PointShadowLowTierColorSRV = nullptr;

	// Mid Tier (1024x1024) - Scale 0.76~1.5
	ID3D11Texture2D* PointShadowMidTierTexture = nullptr;
	ID3D11ShaderResourceView* PointShadowMidTierSRV = nullptr;
	ID3D11ShaderResourceView* PointShadowMidTier2DArraySRV = nullptr;
	ID3D11DepthStencilView* PointShadowMidTierDSVs[6 * MaxLightsPerTier] = { nullptr };
	ID3D11Texture2D* PointShadowMidTierColorTexture = nullptr;
	ID3D11RenderTargetView* PointShadowMidTierRTVs[6 * MaxLightsPerTier] = { nullptr };
	ID3D11ShaderResourceView* PointShadowMidTierColorSRV = nullptr;

	// High Tier (2048x2048) - Scale 1.51~4.0
	ID3D11Texture2D* PointShadowHighTierTexture = nullptr;
	ID3D11ShaderResourceView* PointShadowHighTierSRV = nullptr;
	ID3D11ShaderResourceView* PointShadowHighTier2DArraySRV = nullptr;
	ID3D11DepthStencilView* PointShadowHighTierDSVs[6 * MaxLightsPerTier] = { nullptr };
	ID3D11Texture2D* PointShadowHighTierColorTexture = nullptr;
	ID3D11RenderTargetView* PointShadowHighTierRTVs[6 * MaxLightsPerTier] = { nullptr };
	ID3D11ShaderResourceView* PointShadowHighTierColorSRV = nullptr;
};
