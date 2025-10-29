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

	// Shadow Map Getters
	ID3D11DepthStencilView* GetDirectionalShadowMapDSV() const { return DirectionalShadowMapDSV; }
	ID3D11ShaderResourceView* GetDirectionalShadowMapSRV() const { return DirectionalShadowMapSRV; }
	ID3D11RenderTargetView* GetDirectionalShadowMapColorRTV() const { return DirectionalShadowMapColorRTV; }
	ID3D11ShaderResourceView* GetDirectionalShadowMapColorSRV() const {return DirectionalShadowMapColorSRV; }
	ID3D11Texture2D* GetDirectionalShadowMapColorTexture() const {return DirectionalShadowMapColorTexture; }

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

	ID3D11ShaderResourceView* GetCascadedShadowMapSRV() const { return CascadedShadowMapSRV; }
	ID3D11DepthStencilView* GetCascadedShadowMapDSV(int CascadeIndex) const;
	
	const D3D11_VIEWPORT& GetViewportInfo() const { return ViewportInfo; }
	uint32 GetWidth() const { return Width; }
	uint32 GetHeight() const { return Height; }
	void UpdateViewport(float InMenuBarHeight = 0.f);

	// Direct2D/DirectWrite factory getters
	IDWriteFactory* GetDWriteFactory() const { return DWriteFactory; }

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

	// Directional Light Shadow Map
	ID3D11Texture2D* DirectionalShadowMapTexture = nullptr;
	ID3D11DepthStencilView* DirectionalShadowMapDSV = nullptr;
	ID3D11ShaderResourceView* DirectionalShadowMapSRV = nullptr;
	ID3D11RenderTargetView* DirectionalShadowMapColorRTV = nullptr;
	ID3D11ShaderResourceView* DirectionalShadowMapColorSRV = nullptr;
	ID3D11Texture2D* DirectionalShadowMapColorTexture = nullptr;

    // Spot Light Shadow Map
    ID3D11Texture2D* SpotShadowMapTexture = nullptr;
    ID3D11DepthStencilView* SpotShadowMapDSV = nullptr;
    ID3D11ShaderResourceView* SpotShadowMapSRV = nullptr;
    // Spot Light Shadow Map (VSM color moments)
    ID3D11Texture2D* SpotShadowMapColorTexture = nullptr;
    ID3D11RenderTargetView* SpotShadowMapColorRTV = nullptr;
    ID3D11ShaderResourceView* SpotShadowMapColorSRV = nullptr;

	// CSM Resources
	ID3D11Texture2D* CascadedShadowMapTexture = nullptr;
	ID3D11ShaderResourceView* CascadedShadowMapSRV = nullptr;
	ID3D11DepthStencilView* CascadedShadowMapDSVs[MAX_CASCADES] = { nullptr };
	
	D3D11_VIEWPORT ViewportInfo = {};

	uint32 Width = 0;
	uint32 Height = 0;

	// Direct2D/DirectWrite factories
	ID2D1Factory* D2DFactory = nullptr;
	IDWriteFactory* DWriteFactory = nullptr;

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
