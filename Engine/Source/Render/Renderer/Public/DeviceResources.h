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

    // Point Light Shadow Cube Getters
    ID3D11ShaderResourceView* GetPointShadowCubeSRV() const { return PointShadowCubeSRV; }
    ID3D11DepthStencilView* GetPointShadowCubeDSV(int SliceIndex) const { return (SliceIndex >= 0 && (UINT)SliceIndex < PointShadowCubeDSVsCount) ? PointShadowCubeDSVs[SliceIndex] : nullptr; }
    UINT GetMaxPointShadowLights() const { return MaxPointShadowLights; }
    ID3D11ShaderResourceView* GetPointShadow2DArraySRV() const { return PointShadow2DArraySRV; }
    // Create a per-face SRV to a single cube face for preview/debug
    bool CreatePointShadowFaceSRV(UINT CubeIndex, UINT FaceIndex, ID3D11ShaderResourceView** OutSRV) const;

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

	// Point Light Shadow Cube (TextureCubeArray depth)
	static const UINT MaxPointShadowLights = 16; // capacity for shadowed point lights
	ID3D11Texture2D* PointShadowCubeTexture = nullptr; // Array size = 6 * MaxPointShadowLights
	ID3D11ShaderResourceView* PointShadowCubeSRV = nullptr; // SRV as TextureCubeArray
	ID3D11ShaderResourceView* PointShadow2DArraySRV = nullptr; // SRV as Texture2DArray (for PCF)
	ID3D11DepthStencilView* PointShadowCubeDSVs[6 * MaxPointShadowLights] = { nullptr }; // DSV per face slice
	UINT PointShadowCubeDSVsCount = 0;
};
