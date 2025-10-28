#include "pch.h"
#include "Render/Renderer/Public/DeviceResources.h"

UDeviceResources::UDeviceResources(HWND InWindowHandle)
{
	Create(InWindowHandle);
}

UDeviceResources::~UDeviceResources()
{
	Release();
}

void UDeviceResources::Create(HWND InWindowHandle)
{
	RECT ClientRect;
	GetClientRect(InWindowHandle, &ClientRect);
	Width = ClientRect.right - ClientRect.left;
	Height = ClientRect.bottom - ClientRect.top;

	CreateDeviceAndSwapChain(InWindowHandle);
	CreateFrameBuffer();
	CreateNormalBuffer();
	CreateDepthBuffer();
	CreateSceneColorTarget();
    CreateShadowMapResources();  // TODO: 임시 비활성화
    CreateSpotShadowMapResources();
    CreatePointShadowCubeResources();
    CreateCascadedShadowMap();
	CreateGizmoDepthBuffer();
    CreateFactories();
}

void UDeviceResources::Release()
{
	ReleaseFactories();
    ReleaseCascadedShadowMap();
    ReleaseShadowMapResources();
    ReleaseSpotShadowMapResources();
    ReleasePointShadowCubeResources();
    ReleaseSceneColorTarget();
    ReleaseFrameBuffer();
    ReleaseNormalBuffer();
	ReleaseDepthBuffer();
	ReleaseGizmoDepthBuffer();
	ReleaseDeviceAndSwapChain();
}

/**
 * @brief Direct3D 장치 및 스왑 체인을 생성하는 함수
 * @param InWindowHandle
 */
void UDeviceResources::CreateDeviceAndSwapChain(HWND InWindowHandle)
{
	// 지원하는 Direct3D 기능 레벨을 정의
	D3D_FEATURE_LEVEL featurelevels[] = {D3D_FEATURE_LEVEL_11_0};

	// 스왑 체인 설정 구조체 초기화
	DXGI_SWAP_CHAIN_DESC SwapChainDescription = {};
	SwapChainDescription.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
	SwapChainDescription.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
	SwapChainDescription.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
	SwapChainDescription.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
	SwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT; // 렌더 타겟으로 사용
	SwapChainDescription.BufferCount = 2; // 더블 버퍼링
	SwapChainDescription.OutputWindow = InWindowHandle; // 렌더링할 창 핸들
	SwapChainDescription.Windowed = TRUE; // 창 모드
	SwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

	// Direct3D 장치와 스왑 체인을 생성
	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
	                                           D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG ,
	                                           featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
	                                           &SwapChainDescription, &SwapChain, &Device, nullptr, &DeviceContext);

	if (FAILED(hr))
	{
		assert(!"Failed To Create SwapChain");
	}

	// 생성된 스왑 체인의 정보 가져오기
	SwapChain->GetDesc(&SwapChainDescription);

	// Viewport Info 업데이트
	ViewportInfo = {
		0.0f, 0.0f, static_cast<float>(SwapChainDescription.BufferDesc.Width),
		static_cast<float>(SwapChainDescription.BufferDesc.Height), 0.0f, 1.0f
	};
}

/**
 * @brief Direct3D 장치 및 스왑 체인을 해제하는 함수
 */
void UDeviceResources::ReleaseDeviceAndSwapChain()
{
	if (DeviceContext)
	{
		DeviceContext->ClearState();
		// 남아있는 GPU 명령 실행
		DeviceContext->Flush();
	}

	if (SwapChain)
	{
		SwapChain->Release();
		SwapChain = nullptr;
	}

	if (DeviceContext)
	{
		DeviceContext->Release();
		DeviceContext = nullptr;
	}

	// DX 메모리 Leak 디버깅용 함수
	// DXGI 누출 로그 출력 시 주석 해제로 타입 확인 가능
	// ID3D11Debug* DebugPointer = nullptr;
	// HRESULT Result = GetDevice()->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&DebugPointer));
	// if (SUCCEEDED(Result))
	// {
	// 	DebugPointer->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	// 	DebugPointer->Release();
	// }
	
	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}
}

/**
 * @brief FrameBuffer 생성 함수
 */
void UDeviceResources::CreateFrameBuffer()
{
	UE_LOG("CreateFrameBuffer: Starting...");
	// 스왩 체인으로부터 백 버퍼 텍스처 가져오기
	HRESULT hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);


	// 렌더 타겟 뷰 생성
	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
	framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

	Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);


	// 셰이더 리소스 뷰 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	Device->CreateShaderResourceView(FrameBuffer, &srvDesc, &FrameBufferSRV);
}

/**
 * @brief 프레임 버퍼를 해제하는 함수
 */
void UDeviceResources::ReleaseFrameBuffer()
{
	if (FrameBuffer)
	{
		FrameBuffer->Release();
		FrameBuffer = nullptr;
	}

	if (FrameBufferRTV)
	{
		FrameBufferRTV->Release();
		FrameBufferRTV = nullptr;
	}

	if (FrameBufferSRV)
	{
		FrameBufferSRV->Release();
		FrameBufferSRV = nullptr;
	}
}

void UDeviceResources::CreateNormalBuffer()
{
	D3D11_TEXTURE2D_DESC Texture2DDesc = {};
	Texture2DDesc.Width = Width;
	Texture2DDesc.Height = Height;
	Texture2DDesc.MipLevels = 1;
	Texture2DDesc.ArraySize = 1;
	Texture2DDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	Texture2DDesc.SampleDesc.Count = 1;
	Texture2DDesc.SampleDesc.Quality = 0;
	Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
	Texture2DDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	Texture2DDesc.CPUAccessFlags = 0;
	Texture2DDesc.MiscFlags = 0;

	Device->CreateTexture2D(&Texture2DDesc, nullptr, &NormalBuffer);

	D3D11_RENDER_TARGET_VIEW_DESC RenderTargetViewDesc = {};
	RenderTargetViewDesc.Format = Texture2DDesc.Format;
	RenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RenderTargetViewDesc.Texture2D.MipSlice = 0;
	
	Device->CreateRenderTargetView(NormalBuffer, &RenderTargetViewDesc, &NormalBufferRTV);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = Texture2DDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	Device->CreateShaderResourceView(NormalBuffer, &srvDesc, &NormalBufferSRV);
}

void UDeviceResources::ReleaseNormalBuffer()
{
	if (NormalBuffer)
	{
		NormalBuffer->Release();
		NormalBuffer = nullptr;
	}

	if (NormalBufferRTV)
	{
		NormalBufferRTV->Release();
		NormalBufferRTV = nullptr;
	}

	if (NormalBufferSRV)
	{
		NormalBufferSRV->Release();
		NormalBufferSRV = nullptr;
	}
}


/**
 * @brief Scene Color Texture, SRV, RTV 생성 함수
 */
void UDeviceResources::CreateSceneColorTarget()
{
	ReleaseSceneColorTarget();

	if (!Device || Width == 0 || Height == 0)
	{
		return;
	}

	D3D11_TEXTURE2D_DESC SceneDesc = {};
	SceneDesc.Width = Width;
	SceneDesc.Height = Height;
	SceneDesc.MipLevels = 1;
	SceneDesc.ArraySize = 1;
	SceneDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	SceneDesc.SampleDesc.Count = 1;
	SceneDesc.SampleDesc.Quality = 0;
	SceneDesc.Usage = D3D11_USAGE_DEFAULT;
	SceneDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	SceneDesc.CPUAccessFlags = 0;
	SceneDesc.MiscFlags = 0;


	HRESULT Result = Device->CreateTexture2D(&SceneDesc, nullptr, &SceneColorTexture);
	if (FAILED(Result))
	{
		UE_LOG_ERROR("DeviceResources: SceneColor Texture 생성 실패");
		ReleaseSceneColorTarget();
		return;
	}

	Result = Device->CreateRenderTargetView(SceneColorTexture, nullptr, &SceneColorTextureRTV);
	if (FAILED(Result))
	{
		UE_LOG_ERROR("DeviceResources: SceneColor RTV 생성 실패");
		ReleaseSceneColorTarget();
		return;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = SceneDesc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;

	Result = Device->CreateShaderResourceView(SceneColorTexture, &SRVDesc, &SceneColorTextureSRV);
	if (FAILED(Result))
	{
		UE_LOG_ERROR("DeviceResources: SceneColor SRV 생성 실패");
		ReleaseSceneColorTarget();
	}
}

/**
 * @brief Scene Color Texture, SRV, RTV를 해제하는 함수
 */
void UDeviceResources::ReleaseSceneColorTarget()
{
	SafeRelease(SceneColorTextureSRV);
	SafeRelease(SceneColorTextureRTV);
	SafeRelease(SceneColorTexture);
}


void UDeviceResources::CreateDepthBuffer()
{
	D3D11_TEXTURE2D_DESC dsDesc = {};
	dsDesc.Width = Width;
	dsDesc.Height = Height;
	dsDesc.MipLevels = 1;
	dsDesc.ArraySize = 1;
	dsDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	dsDesc.SampleDesc.Count = 1;
	dsDesc.SampleDesc.Quality = 0;
	dsDesc.Usage = D3D11_USAGE_DEFAULT;
	dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	dsDesc.CPUAccessFlags = 0;
	dsDesc.MiscFlags = 0;

	Device->CreateTexture2D(&dsDesc, nullptr, &DepthBuffer);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	Device->CreateDepthStencilView(DepthBuffer, &dsvDesc, &DepthStencilView);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	Device->CreateShaderResourceView(DepthBuffer, &srvDesc, &DepthBufferSRV);
	Device->CreateShaderResourceView(DepthBuffer, &srvDesc, &DepthStencilSRV);
}

void UDeviceResources::ReleaseDepthBuffer()
{
	if (DepthStencilView)
	{
		DepthStencilView->Release();
		DepthStencilView = nullptr;
	}
	if (DepthStencilSRV)
	{
		DepthStencilSRV->Release();
		DepthStencilSRV = nullptr;
	}
	if (DepthBuffer)
	{
		DepthBuffer->Release();
		DepthBuffer = nullptr;
	}
	if (DepthBufferSRV)
	{
		DepthBufferSRV->Release();
		DepthBufferSRV = nullptr;
	}
}

void UDeviceResources::CreateGizmoDepthBuffer()
{
	ReleaseGizmoDepthBuffer(); // Ensure previous resources are released

	D3D11_TEXTURE2D_DESC depthTexDesc = {};
	depthTexDesc.Width = Width;
	depthTexDesc.Height = Height;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.ArraySize = 1;
	depthTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthTexDesc.SampleDesc.Count = 1;
	depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	HRESULT hr = Device->CreateTexture2D(&depthTexDesc, nullptr, &GizmoDepthTexture);
	assert(SUCCEEDED(hr) && "Failed to create Gizmo Depth Texture!");

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	hr = Device->CreateDepthStencilView(GizmoDepthTexture, &dsvDesc, &GizmoDSV);
	assert(SUCCEEDED(hr) && "Failed to create Gizmo DSV!");
}

void UDeviceResources::ReleaseGizmoDepthBuffer()
{
	SafeRelease(GizmoDSV);
	SafeRelease(GizmoDepthTexture);
}

ID3D11DepthStencilView* UDeviceResources::GetCascadedShadowMapDSV(int CascadeIndex) const
{
	if (CascadeIndex >= 0 && CascadeIndex < MAX_CASCADES)
	{
		return CascadedShadowMapDSVs[CascadeIndex];
	}
	return nullptr;
}

ID3D11RenderTargetView* UDeviceResources::GetCascadedShadowMapColorRTV(int CascadeIndex) const
{
	if (CascadeIndex >= 0 && CascadeIndex < MAX_CASCADES)
	{
		return CascadedShadowMapColorRTVs[CascadeIndex];
	}
	return nullptr;
}

void UDeviceResources::UpdateViewport(float InMenuBarHeight)
{
	DXGI_SWAP_CHAIN_DESC SwapChainDescription = {};
	SwapChain->GetDesc(&SwapChainDescription);

	// 전체 화면 크기
	float FullWidth = static_cast<float>(SwapChainDescription.BufferDesc.Width);
	float FullHeight = static_cast<float>(SwapChainDescription.BufferDesc.Height);

	// 메뉴바 아래에 위치하도록 뷰포트 조정
	ViewportInfo = {
		0.f,
		0.f,
		FullWidth,
		FullHeight,
		0.0f,
		1.0f
	};

	Width = SwapChainDescription.BufferDesc.Width;
	Height = SwapChainDescription.BufferDesc.Height;
}

void UDeviceResources::CreateCascadedShadowMap()
{
	// Create Texture2DArray
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = 2048;
	texDesc.Height = 2048;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = MAX_CASCADES;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	HRESULT hr = Device->CreateTexture2D(&texDesc, nullptr, &CascadedShadowMapTexture);
	if (FAILED(hr)) { UE_LOG_ERROR("Failed to create CSM Texture2DArray"); return; }

	// Create Shader Resource View
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = 1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = MAX_CASCADES;

	hr = Device->CreateShaderResourceView(CascadedShadowMapTexture, &srvDesc, &CascadedShadowMapSRV);
	if (FAILED(hr)) { UE_LOG_ERROR("Failed to create CSM SRV"); return; }

	// Create Depth Stencil View
	for (int i = 0; i < MAX_CASCADES; i++)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = i;
		dsvDesc.Texture2DArray.ArraySize = 1;

		hr = Device->CreateDepthStencilView(CascadedShadowMapTexture, &dsvDesc, &CascadedShadowMapDSVs[i]);
		if (FAILED(hr)) { UE_LOG_ERROR("Failed to create CSM DSV for slice %d", i); }
	}

	// Color Texture2DArray for VSM (moments)
	D3D11_TEXTURE2D_DESC ColorDesc = {};
	ColorDesc.Width = 2048;
	ColorDesc.Height = 2048;
	ColorDesc.MipLevels = 0; // generate full mip chain
	ColorDesc.ArraySize = MAX_CASCADES;
	ColorDesc.Format = DXGI_FORMAT_R32G32_FLOAT;	// moments (m1, m2)
	ColorDesc.SampleDesc.Count = 1;
	ColorDesc.Usage = D3D11_USAGE_DEFAULT;
	ColorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	ColorDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	hr = Device->CreateTexture2D(&ColorDesc, nullptr, &CascadedShadowMapColorTexture);
	if (FAILED(hr)) { UE_LOG_ERROR("Failed to create CSM ColorArray"); return; }

	// RTVs per slice
	for (int i = 0; i < MAX_CASCADES; i++)
	{
		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
		RTVDesc.Format = ColorDesc.Format;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		RTVDesc.Texture2DArray.MipSlice = 0;
		RTVDesc.Texture2DArray.FirstArraySlice = i;
		RTVDesc.Texture2DArray.ArraySize = 1;

		hr = Device->CreateRenderTargetView(CascadedShadowMapColorTexture, &RTVDesc, &CascadedShadowMapColorRTVs[i]);
		if (FAILED(hr)) { UE_LOG_ERROR("Failed to create CSM Color RTV for slice %d", i); }
	}

	// SRV (total moments array)
	D3D11_SHADER_RESOURCE_VIEW_DESC ColorSRVDesc = {};
	ColorSRVDesc.Format = ColorDesc.Format;
	ColorSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	ColorSRVDesc.Texture2DArray.MostDetailedMip = 0;
	ColorSRVDesc.Texture2DArray.MipLevels = -1;
	ColorSRVDesc.Texture2DArray.FirstArraySlice = 0;
	ColorSRVDesc.Texture2DArray.ArraySize = MAX_CASCADES;

	hr = Device->CreateShaderResourceView(CascadedShadowMapColorTexture, &ColorSRVDesc, &CascadedShadowMapColorSRV);
	if (FAILED(hr)) { UE_LOG_ERROR("Failed to create CSM Color SRV"); return; }
}

void UDeviceResources::ReleaseCascadedShadowMap()
{
	for (int i = 0; i < MAX_CASCADES; i++)
	{
		SafeRelease(CascadedShadowMapDSVs[i]);
	}
	for (int i = 0; i < MAX_CASCADES; i++)
	{
		SafeRelease(CascadedShadowMapColorRTVs[i]);
	}
	SafeRelease(CascadedShadowMapSRV);
	SafeRelease(CascadedShadowMapColorSRV);
	SafeRelease(CascadedShadowMapTexture);
	SafeRelease(CascadedShadowMapColorTexture);
}

void UDeviceResources::CreateFactories()
{
	// DirectWrite 팩토리 생성
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
	                    reinterpret_cast<IUnknown**>(&DWriteFactory));
}

void UDeviceResources::ReleaseFactories()
{
	if (DWriteFactory)
	{
		DWriteFactory->Release();
		DWriteFactory = nullptr;
	}
}

/**
 * @brief Directional Light Shadow Map 리소스 생성
 */
void UDeviceResources::CreateShadowMapResources()
{
	// TODO: 임시 비활성화 - 문제 분리용
	
	if (!Device)
	{
		UE_LOG_ERROR("CreateShadowMapResources: Device is null!");
		return;
	}

	// Shadow Map 크기 (2048x2048 고품질)
	const UINT ShadowMapSize = 2048;

	// Depth Texture 생성
	D3D11_TEXTURE2D_DESC TexDesc = {};
	TexDesc.Width = ShadowMapSize;
	TexDesc.Height = ShadowMapSize;
	TexDesc.MipLevels = 1;
	TexDesc.ArraySize = 1;
	TexDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;  // Depth 24bit + Stencil 8bit
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	TexDesc.Usage = D3D11_USAGE_DEFAULT;
	TexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	TexDesc.CPUAccessFlags = 0;
	TexDesc.MiscFlags = 0;

	HRESULT hr = Device->CreateTexture2D(&TexDesc, nullptr, &DirectionalShadowMapTexture);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("Failed to create Directional Shadow Map Texture");
		return;
	}

	// Depth Stencil View 생성
	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
	DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSVDesc.Texture2D.MipSlice = 0;

	hr = Device->CreateDepthStencilView(DirectionalShadowMapTexture, &DSVDesc, &DirectionalShadowMapDSV);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("Failed to create Directional Shadow Map DSV");
		ReleaseShadowMapResources();
		return;
	}

	// Shader Resource View 생성 (Depth를 텔스처로 사용)
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;

	hr = Device->CreateShaderResourceView(DirectionalShadowMapTexture, &SRVDesc, &DirectionalShadowMapSRV);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("Failed to create Directional Shadow Map SRV");
		ReleaseShadowMapResources();
		return;
	}

	UE_LOG("Directional Shadow Map Resources Created Successfully (Size: %dx%d)", ShadowMapSize, ShadowMapSize);

	D3D11_TEXTURE2D_DESC ColorDesc = {};
	ColorDesc.Width = ShadowMapSize;
	ColorDesc.Height = ShadowMapSize;
	ColorDesc.MipLevels = 0; // 전체 밉맵 체인을 생성하도록 함.
	ColorDesc.ArraySize = 1;
	ColorDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
	ColorDesc.SampleDesc.Count = 1;
	ColorDesc.Usage = D3D11_USAGE_DEFAULT;
	ColorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	ColorDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS; // 밉맵 자동 생성을 허용하는 플래그

	hr = Device->CreateTexture2D(&ColorDesc, nullptr, &DirectionalShadowMapColorTexture);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("Failed to create Directional Shadow Map Color Texture");
		ReleaseShadowMapResources();
		return;
	}

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.Format = ColorDesc.Format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = 0;

	hr = Device->CreateRenderTargetView(DirectionalShadowMapColorTexture, &RTVDesc, &DirectionalShadowMapColorRTV);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("Failed to create Directional Shadow Map Color RTV");
		ReleaseShadowMapResources();
		return;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC ColorSRVDesc = {};
	ColorSRVDesc.Format = ColorDesc.Format;
	ColorSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	ColorSRVDesc.Texture2D.MostDetailedMip = 0;
	ColorSRVDesc.Texture2D.MipLevels = 1;

	hr = Device->CreateShaderResourceView(DirectionalShadowMapColorTexture, &ColorSRVDesc, &DirectionalShadowMapColorSRV);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("Failed to create Directional Shadow Map Color SRV");
		ReleaseShadowMapResources();
		return;
	}
}

/**
 * @brief Spot Light Shadow Map 리소스 생성 (1024x1024)
 */
void UDeviceResources::CreateSpotShadowMapResources()
{
    if (!Device)
    {
        UE_LOG_ERROR("CreateSpotShadowMapResources: Device is null!");
        return;
    }

    // Configure a fixed 4x4 atlas of 1024x1024 tiles for spotlights
    const UINT TileSize = 1024;
    const UINT AtlasCols = 4;
    const UINT AtlasRows = 4;
    const UINT ShadowMapWidth = TileSize * AtlasCols;
    const UINT ShadowMapHeight = TileSize * AtlasRows;

    // Depth Texture for Spot Shadow
    D3D11_TEXTURE2D_DESC DepthDesc = {};
    DepthDesc.Width = ShadowMapWidth;
    DepthDesc.Height = ShadowMapHeight;
    DepthDesc.MipLevels = 1;
    DepthDesc.ArraySize = 1;
    DepthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    DepthDesc.SampleDesc.Count = 1;
    DepthDesc.SampleDesc.Quality = 0;
    DepthDesc.Usage = D3D11_USAGE_DEFAULT;
    DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = Device->CreateTexture2D(&DepthDesc, nullptr, &SpotShadowMapTexture);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("Failed to create Spot Shadow Map Texture");
        return;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
    DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    DSVDesc.Texture2D.MipSlice = 0;
    hr = Device->CreateDepthStencilView(SpotShadowMapTexture, &DSVDesc, &SpotShadowMapDSV);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("Failed to create Spot Shadow Map DSV");
        ReleaseSpotShadowMapResources();
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = 1;
    hr = Device->CreateShaderResourceView(SpotShadowMapTexture, &SRVDesc, &SpotShadowMapSRV);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("Failed to create Spot Shadow Map SRV");
        ReleaseSpotShadowMapResources();
        return;
    }

    // Create VSM color moments atlas texture (R32G32_FLOAT), same atlas layout
    {
        D3D11_TEXTURE2D_DESC ColorDesc = {};
        ColorDesc.Width = ShadowMapWidth;
        ColorDesc.Height = ShadowMapHeight;
        ColorDesc.MipLevels = 0; // allow full mip chain
        ColorDesc.ArraySize = 1;
        ColorDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        ColorDesc.SampleDesc.Count = 1;
        ColorDesc.Usage = D3D11_USAGE_DEFAULT;
        ColorDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        ColorDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        hr = Device->CreateTexture2D(&ColorDesc, nullptr, &SpotShadowMapColorTexture);
        if (FAILED(hr))
        {
            UE_LOG_ERROR("Failed to create Spot Shadow Map Color Texture");
            ReleaseSpotShadowMapResources();
            return;
        }

        D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
        RTVDesc.Format = ColorDesc.Format;
        RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        RTVDesc.Texture2D.MipSlice = 0;
        hr = Device->CreateRenderTargetView(SpotShadowMapColorTexture, &RTVDesc, &SpotShadowMapColorRTV);
        if (FAILED(hr))
        {
            UE_LOG_ERROR("Failed to create Spot Shadow Map Color RTV");
            ReleaseSpotShadowMapResources();
            return;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC ColorSRVDesc = {};
        ColorSRVDesc.Format = ColorDesc.Format;
        ColorSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        ColorSRVDesc.Texture2D.MostDetailedMip = 0;
        ColorSRVDesc.Texture2D.MipLevels = 1; // expose base level; mips optional
        hr = Device->CreateShaderResourceView(SpotShadowMapColorTexture, &ColorSRVDesc, &SpotShadowMapColorSRV);
        if (FAILED(hr))
        {
            UE_LOG_ERROR("Failed to create Spot Shadow Map Color SRV");
            ReleaseSpotShadowMapResources();
            return;
        }
    }
}

void UDeviceResources::ReleaseSpotShadowMapResources()
{
    SafeRelease(SpotShadowMapSRV);
    SafeRelease(SpotShadowMapDSV);
    SafeRelease(SpotShadowMapTexture);
    SafeRelease(SpotShadowMapColorSRV);
    SafeRelease(SpotShadowMapColorRTV);
    SafeRelease(SpotShadowMapColorTexture);
}

void UDeviceResources::CreatePointShadowCubeResources()
{
    // Create a Texture2D array with 6 slices per cube (TextureCubeArray semantics)
    SafeRelease(PointShadowCubeSRV);
    for (UINT i = 0; i < PointShadowCubeDSVsCount; ++i)
    {
        SafeRelease(PointShadowCubeDSVs[i]);
    }
    PointShadowCubeDSVsCount = 0;
    SafeRelease(PointShadowCubeTexture);

    if (!Device)
    {
        UE_LOG_ERROR("CreatePointShadowCubeResources: Device is null!");
        return;
    }

    const UINT FaceSize = 1024; // match PointShadowViewport; can be adjusted
    const UINT NumCubes = MaxPointShadowLights;
    const UINT ArraySize = 6 * NumCubes;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = FaceSize;
    texDesc.Height = FaceSize;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = ArraySize;
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    HRESULT hr = Device->CreateTexture2D(&texDesc, nullptr, &PointShadowCubeTexture);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("Failed to create Point Shadow Cube Texture");
        ReleasePointShadowCubeResources();
        return;
    }

    // SRV as TextureCubeArray
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
    srvDesc.TextureCubeArray.MostDetailedMip = 0;
    srvDesc.TextureCubeArray.MipLevels = 1;
    srvDesc.TextureCubeArray.First2DArrayFace = 0;
    srvDesc.TextureCubeArray.NumCubes = NumCubes;
    hr = Device->CreateShaderResourceView(PointShadowCubeTexture, &srvDesc, &PointShadowCubeSRV);
    if (FAILED(hr))
    {
        UE_LOG_ERROR("Failed to create Point Shadow Cube SRV");
        ReleasePointShadowCubeResources();
        return;
    }

    // Create a DSV for each face slice
    for (UINT slice = 0; slice < ArraySize; ++slice)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice = 0;
        dsvDesc.Texture2DArray.FirstArraySlice = slice;
        dsvDesc.Texture2DArray.ArraySize = 1;
        hr = Device->CreateDepthStencilView(PointShadowCubeTexture, &dsvDesc, &PointShadowCubeDSVs[slice]);
        if (FAILED(hr))
        {
            UE_LOG_ERROR("Failed to create Point Shadow Cube DSV");
            ReleasePointShadowCubeResources();
            return;
        }
        ++PointShadowCubeDSVsCount;
    }
}

void UDeviceResources::ReleasePointShadowCubeResources()
{
    SafeRelease(PointShadowCubeSRV);
    for (UINT i = 0; i < PointShadowCubeDSVsCount; ++i)
    {
        SafeRelease(PointShadowCubeDSVs[i]);
        PointShadowCubeDSVs[i] = nullptr;
    }
    PointShadowCubeDSVsCount = 0;
    SafeRelease(PointShadowCubeTexture);
}

/**
 * @brief Shadow Map 리소스 해제
 */
void UDeviceResources::ReleaseShadowMapResources()
{
    SafeRelease(DirectionalShadowMapColorSRV);
    SafeRelease(DirectionalShadowMapColorRTV);
    SafeRelease(DirectionalShadowMapColorTexture);
    SafeRelease(DirectionalShadowMapSRV);
    SafeRelease(DirectionalShadowMapDSV);
    SafeRelease(DirectionalShadowMapTexture);
}
