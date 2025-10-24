#include "pch.h"
#include <algorithm>
#include "Component/Mesh/Public/StaticMesh.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/DecalComponent.h"
#include "Component/Public/HeightFogComponent.h"
#include "Component/Public/PointLightComponent.h"
#include "Component/Public/SpotLightComponent.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Component/Public/AmbientLightComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/UUIDTextComponent.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Editor.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Level/Public/Level.h"
#include "Manager/UI/Public/UIManager.h"
#include "Optimization/Public/OcclusionCuller.h"
#include "Render/RenderPass/Public/BillboardPass.h"
#include "Render/RenderPass/Public/DecalPass.h"
#include "Render/RenderPass/Public/FXAAPass.h"
#include "Render/RenderPass/Public/FogPass.h"
#include "Render/RenderPass/Public/PointLightPass.h"
#include "Render/RenderPass/Public/RenderPass.h"
#include "Render/RenderPass/Public/LightPass.h"
#include "Render/RenderPass/Public/StaticMeshPass.h"
#include "Render/RenderPass/Public/TextPass.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/RenderPass/Public/ClusteredRenderingGridPass.h"

#include "Render/RenderPass/Public/SceneDepthPass.h"
#include "Render/RenderPass/Public/UpdateLightBufferPass.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Global/Octree.h"

IMPLEMENT_SINGLETON_CLASS(URenderer, UObject)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;


void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext());
	ViewportClient = new FViewport();
	
	
	// 렌더링 상태 및 리소스 생성
	CreateDepthStencilState();
	CreateBlendState();
	CreateSamplerState();
	CreateDefaultShader();
	CreateTextureShader();
	CreateDecalShader();
	CreateFogShader();
	CreateConstantBuffers();
	CreateFXAAShader();
	CreateStaticMeshShader();
	CreateGizmoShader();
	CreateClusteredRenderingGrid();
	//CreateShadowMapShader();

	//ViewportClient->InitializeLayout(DeviceResources->GetViewportInfo());

	// Shadow Map 베이킹 Pass (가장 먼저 실행)
	//FUpdateLightBufferPass* UpdateLightBufferPass = new FUpdateLightBufferPass(Pipeline, ConstantBufferViewProj, ConstantBufferModels,
	//	ShadowMapVS, ShadowMapPS, ShadowMapInputLayout);
	//RenderPasses.push_back(UpdateLightBufferPass);

	LightPass = new FLightPass(Pipeline, ConstantBufferViewProj, GizmoInputLayout, GizmoVS, GizmoPS, DefaultDepthStencilState);
	RenderPasses.push_back(LightPass);

	FStaticMeshPass* StaticMeshPass = new FStaticMeshPass(Pipeline, ConstantBufferViewProj, ConstantBufferModels,
		UberLitVertexShader, UberLitPixelShader, UberLitInputLayout, DefaultDepthStencilState);
	RenderPasses.push_back(StaticMeshPass);

	FDecalPass* DecalPass = new FDecalPass(Pipeline, ConstantBufferViewProj,
		DecalVertexShader, DecalPixelShader, DecalInputLayout, DecalDepthStencilState, AlphaBlendState);
	RenderPasses.push_back(DecalPass);
	
	FBillboardPass* BillboardPass = new FBillboardPass(Pipeline, ConstantBufferViewProj, ConstantBufferModels,
		TextureVertexShader, TexturePixelShader, TextureInputLayout, DefaultDepthStencilState, AlphaBlendState);
	RenderPasses.push_back(BillboardPass);

	FTextPass* TextPass = new FTextPass(Pipeline, ConstantBufferViewProj, ConstantBufferModels);
	RenderPasses.push_back(TextPass);

	FFogPass* FogPass = new FFogPass(Pipeline, ConstantBufferViewProj,
		FogVertexShader, FogPixelShader, FogInputLayout, DefaultDepthStencilState, AlphaBlendState);
	RenderPasses.push_back(FogPass);

	ClusteredRenderingGridPass = new FClusteredRenderingGridPass(Pipeline, ConstantBufferViewProj,
		ClusteredRenderingGridVS, ClusteredRenderingGridPS, ClusteredRenderingGridInputLayout, DefaultDepthStencilState, AlphaBlendState);
	RenderPasses.push_back(ClusteredRenderingGridPass);

	FSceneDepthPass* SceneDepthPass = new FSceneDepthPass(Pipeline, ConstantBufferViewProj, DisabledDepthStencilState);
	RenderPasses.push_back(SceneDepthPass);

	// UPipeline* InPipeline, UDeviceResources* InDeviceResources, ID3D11VertexShader* InVS,
	// ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11SamplerState* InSampler
	FXAAPass = new FFXAAPass(Pipeline, DeviceResources, FXAAVertexShader, FXAAPixelShader, FXAAInputLayout, FXAASamplerState);
	//RenderPasses.push_back(FXAAPass);
	
}

void URenderer::Release()
{
	ReleaseConstantBuffers();
	ReleaseDefaultShader();
	ReleaseDepthStencilState();
	ReleaseBlendState();
	ReleaseSamplerState();
	FRenderResourceFactory::ReleaseRasterizerState();
	for (auto& RenderPass : RenderPasses)
	{
		RenderPass->Release();
		SafeDelete(RenderPass);
	}
	FXAAPass->Release();
	SafeDelete(FXAAPass);
	
	SafeDelete(ViewportClient);
	SafeDelete(Pipeline);
	SafeDelete(DeviceResources);
}

void URenderer::CreateDepthStencilState()
{
	// 3D Default Depth Stencil (Depth O, Stencil X)
	D3D11_DEPTH_STENCIL_DESC DefaultDescription = {};
	DefaultDescription.DepthEnable = TRUE;
	DefaultDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DefaultDescription.DepthFunc = D3D11_COMPARISON_LESS;
	DefaultDescription.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&DefaultDescription, &DefaultDepthStencilState);

	// Decal Depth Stencil (Depth Read, Stencil X)
	D3D11_DEPTH_STENCIL_DESC DecalDescription = {};
	DecalDescription.DepthEnable = TRUE;
	DecalDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DecalDescription.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	DecalDescription.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&DecalDescription, &DecalDepthStencilState);


	// Disabled Depth Stencil (Depth X, Stencil X)
	D3D11_DEPTH_STENCIL_DESC DisabledDescription = {};
	DisabledDescription.DepthEnable = FALSE;
	DisabledDescription.StencilEnable = FALSE;
	GetDevice()->CreateDepthStencilState(&DisabledDescription, &DisabledDepthStencilState);
}

void URenderer::CreateBlendState()
{
    // Alpha Blending
    D3D11_BLEND_DESC BlendDesc = {};
    BlendDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    GetDevice()->CreateBlendState(&BlendDesc, &AlphaBlendState);

    // Additive Blending
    D3D11_BLEND_DESC AdditiveBlendDesc = {};
    AdditiveBlendDesc.RenderTarget[0].BlendEnable = TRUE;
    AdditiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    AdditiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    AdditiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    AdditiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    AdditiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    AdditiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    AdditiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    GetDevice()->CreateBlendState(&AdditiveBlendDesc, &AdditiveBlendState);
}

void URenderer::CreateSamplerState()
{
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	GetDevice()->CreateSamplerState(&samplerDesc, &DefaultSampler);
}

void URenderer::RegisterShaderReloadCache(const std::filesystem::path& ShaderPath, ShaderUsage Usage)
{
	std::error_code ErrorCode; // 파일 API는 대개 문제가 생겼을 때 exception 발생을 피하고 ErrorCode에 문제 내용 저장

	// weakly_canonical: 경로의 일부가 실제로 없어도 존재하는 구간까지는 실제 파일시스템을 참고해 절대경로로 정규화 + 나머지는 문자열로 정규화
	std::filesystem::path NormalizedPath = std::filesystem::weakly_canonical(ShaderPath, ErrorCode);
	if (ErrorCode)
	{
		// 1차 FALLBACK: 기준 경로 오류 대비. 현재 작업 디렉터리를 기준으로 재시도.
		ErrorCode.clear();
		NormalizedPath = std::filesystem::weakly_canonical(std::filesystem::current_path() / ShaderPath, ErrorCode);
	}
	if (ErrorCode)
	{
		// 2차 FALLBACK: 그냥 원본 경로 사용.
		ErrorCode.clear();
		NormalizedPath = ShaderPath;
	}

	const std::wstring Key = NormalizedPath.generic_wstring(); // 경로 구분자를 '/'로 통일한 유니코드 문자열
	ShaderFileUsageMap[Key].insert(Usage);

	// 정규화된 경로(NormalizedPath)의 마지막 수정 시간 캐싱
	const auto LastWriteTime = std::filesystem::last_write_time(NormalizedPath, ErrorCode);
	if (!ErrorCode)
	{
		ShaderFileLastWriteTimeMap[Key] = LastWriteTime;
		return;
	}
}

void URenderer::CreateDefaultShader()
{
	const std::wstring ShaderFilePathString = L"Asset/Shader/SampleShader.hlsl";
	const std::filesystem::path ShaderPath(ShaderFilePathString);

	TArray<D3D11_INPUT_ELEMENT_DESC> DefaultLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(ShaderFilePathString, DefaultLayout, &DefaultVertexShader, &DefaultInputLayout);
	FRenderResourceFactory::CreatePixelShader(ShaderFilePathString, &DefaultPixelShader);
	Stride = sizeof(FNormalVertex);

	RegisterShaderReloadCache(ShaderPath, ShaderUsage::DEFAULT);
}

void URenderer::CreateTextureShader()
{
	const std::wstring VSFilePathString = L"Asset/Shader/TextureVS.hlsl";
	const std::filesystem::path VSPath(VSFilePathString);
	const std::wstring PSFilePathString = L"Asset/Shader/TexturePS.hlsl";
	const std::filesystem::path PSPath(PSFilePathString);

	TArray<D3D11_INPUT_ELEMENT_DESC> TextureLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(VSFilePathString, TextureLayout, &TextureVertexShader, &TextureInputLayout);
	FRenderResourceFactory::CreatePixelShader(PSFilePathString, &TexturePixelShader);

	RegisterShaderReloadCache(VSPath, ShaderUsage::TEXTURE);
	RegisterShaderReloadCache(PSPath, ShaderUsage::TEXTURE);
}

void URenderer::CreateDecalShader()
{
	const std::wstring ShaderFilePathString = L"Asset/Shader/DecalShader.hlsl";
	const std::filesystem::path ShaderPath(ShaderFilePathString);

	TArray<D3D11_INPUT_ELEMENT_DESC> DecalLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(ShaderFilePathString, DecalLayout, &DecalVertexShader, &DecalInputLayout);
	FRenderResourceFactory::CreatePixelShader(ShaderFilePathString, &DecalPixelShader);

	RegisterShaderReloadCache(ShaderPath, ShaderUsage::DECAL);
}

void URenderer::CreateFogShader()
{
	const std::wstring ShaderFilePathString = L"Asset/Shader/HeightFogShader.hlsl";
	const std::filesystem::path ShaderPath(ShaderFilePathString);

	TArray<D3D11_INPUT_ELEMENT_DESC> FogLayout =
	{
	};
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(ShaderFilePathString, FogLayout, &FogVertexShader, &FogInputLayout);
	FRenderResourceFactory::CreatePixelShader(ShaderFilePathString, &FogPixelShader);

	RegisterShaderReloadCache(ShaderPath, ShaderUsage::FOG);
}

void URenderer::CreateFXAAShader()
{
	const std::wstring ShaderFilePathString = L"Asset/Shader/FXAAShader.hlsl";
	const std::filesystem::path ShaderPath(ShaderFilePathString);

	TArray<D3D11_INPUT_ELEMENT_DESC> FXAALayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(ShaderFilePathString, FXAALayout, &FXAAVertexShader, &FXAAInputLayout);
	FRenderResourceFactory::CreatePixelShader(ShaderFilePathString, &FXAAPixelShader);
	
	FXAASamplerState = FRenderResourceFactory::CreateFXAASamplerState();

	RegisterShaderReloadCache(ShaderPath, ShaderUsage::FXAA);
}

void URenderer::CreateStaticMeshShader()
{
	const std::wstring ShaderFilePathString = L"Asset/Shader/UberLit.hlsl";
	const std::filesystem::path ShaderPath(ShaderFilePathString);

	TArray<D3D11_INPUT_ELEMENT_DESC> ShaderMeshLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Tangent),  D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	
	// Compile Lambert variant (default)
	TArray<D3D_SHADER_MACRO> LambertMacros = {
		{ "LIGHTING_MODEL_LAMBERT", "1" },
		{ nullptr, nullptr }
	};
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(ShaderFilePathString, ShaderMeshLayout, &UberLitVertexShader, &UberLitInputLayout, "Uber_VS", LambertMacros.data());
	FRenderResourceFactory::CreatePixelShader(ShaderFilePathString, &UberLitPixelShader, "Uber_PS", LambertMacros.data());
	
	// Compile Gouraud variant
	TArray<D3D_SHADER_MACRO> GouraudMacros = {
		{ "LIGHTING_MODEL_GOURAUD", "1" },
		{ nullptr, nullptr }
	};
	ID3D11InputLayout* GouraudInputLayout = nullptr;
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(ShaderFilePathString, ShaderMeshLayout, &UberLitVertexShaderGouraud, &GouraudInputLayout, "Uber_VS", GouraudMacros.data());
	SafeRelease(GouraudInputLayout);
	FRenderResourceFactory::CreatePixelShader(ShaderFilePathString, &UberLitPixelShaderGouraud, "Uber_PS", GouraudMacros.data());
	
	// Compile Phong (Blinn-Phong) variant
	TArray<D3D_SHADER_MACRO> PhongMacros = {
		{ "LIGHTING_MODEL_BLINNPHONG", "1" },
		{ nullptr, nullptr }
	};
	FRenderResourceFactory::CreatePixelShader(ShaderFilePathString, &UberLitPixelShaderBlinnPhong, "Uber_PS", PhongMacros.data());

	TArray<D3D_SHADER_MACRO> WorldNormalViewMacros = {
		{ "LIGHTING_MODEL_NORMAL", "1" },
		{ nullptr, nullptr }
	};
	FRenderResourceFactory::CreatePixelShader(ShaderFilePathString, &UberLitPixelShaderWorldNormal, "Uber_PS", WorldNormalViewMacros.data());

	RegisterShaderReloadCache(ShaderPath, ShaderUsage::STATICMESH);
}

void URenderer::CreateGizmoShader()
{
	const std::wstring ShaderFilePathString = L"Asset/Shader/GizmoLine.hlsl";
	const std::filesystem::path ShaderPath(ShaderFilePathString);

	// Create shaders
	TArray<D3D11_INPUT_ELEMENT_DESC> LayoutDesc =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	FRenderResourceFactory::CreateVertexShaderAndInputLayout(ShaderFilePathString, LayoutDesc, &GizmoVS, &GizmoInputLayout);
	FRenderResourceFactory::CreatePixelShader(ShaderFilePathString, &GizmoPS);

	RegisterShaderReloadCache(ShaderPath, ShaderUsage::GIZMO);
}

void URenderer::CreateShadowMapShader()
{
	const std::wstring ShaderFilePathString = L"Asset/Shader/ShadowMap.hlsl";
	const std::filesystem::path ShaderPath(ShaderFilePathString);

	// Shadow Map용 Input Layout 정의
	TArray<D3D11_INPUT_ELEMENT_DESC> ShadowMapLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// Vertex Shader 및 Input Layout 생성
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(
		ShaderFilePathString,
		ShadowMapLayout,
		&ShadowMapVS,
		&ShadowMapInputLayout,
		"mainVS",
		nullptr
	);

	// Pixel Shader 생성 (VSM용)
	FRenderResourceFactory::CreatePixelShader(
		ShaderFilePathString,
		&ShadowMapPS,
		"mainPS",
		nullptr
	);

	RegisterShaderReloadCache(ShaderPath, ShaderUsage::STATICMESH);
}

void URenderer::CreateClusteredRenderingGrid()
{
	const std::wstring ShaderFilePathString = L"Asset/Shader/ClusteredRenderingGrid.hlsl";
	const std::filesystem::path ShaderPath(ShaderFilePathString);

	TArray<D3D11_INPUT_ELEMENT_DESC> InputLayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	FRenderResourceFactory::CreateVertexShaderAndInputLayout(ShaderFilePathString, InputLayout, &ClusteredRenderingGridVS, &ClusteredRenderingGridInputLayout);
	FRenderResourceFactory::CreatePixelShader(ShaderFilePathString, &ClusteredRenderingGridPS);

	RegisterShaderReloadCache(ShaderPath, ShaderUsage::CLUSTERED_RENDERING_GRID);
}

TSet<ShaderUsage> URenderer::GatherHotReloadTargets()
{
	TSet<ShaderUsage> HotReloadTargets = {};
	for (const auto& ShaderFileUsagePair : ShaderFileUsageMap)
	{
		const std::wstring& ShaderFilePathString = ShaderFileUsagePair.first;
		const std::filesystem::path ShaderPath(ShaderFilePathString);

		// 해당 파일을 마지막으로 불러왔을 때 캐시해둔 최종 수정 시간 조회
		const auto CachedLastWriteTimeIter = ShaderFileLastWriteTimeMap.find(ShaderFilePathString);
		if (CachedLastWriteTimeIter == ShaderFileLastWriteTimeMap.end())
		{
			continue;
		}
		const auto& CachedLastWriteTime = CachedLastWriteTimeIter->second;

		// 파일의 현재 최종 수정 시간 조회
		std::error_code ErrorCode;
		const auto CurrentLastWriteTime = std::filesystem::last_write_time(ShaderPath, ErrorCode);
		if (ErrorCode)
		{
			continue;
		}
		
		// 두 시간 비교해서 다르면 핫 리로드 대상에 추가
		if (CurrentLastWriteTime != CachedLastWriteTime)
		{
			for (const ShaderUsage Usage : ShaderFileUsagePair.second)
			{
				HotReloadTargets.insert(Usage);
			}
		}
	}

	return HotReloadTargets;
}

void URenderer::HotReloadShaders()
{
	TSet<ShaderUsage> HotReloadTargets = GatherHotReloadTargets();

	for (TSet<ShaderUsage>::iterator Iter = HotReloadTargets.begin(); Iter != HotReloadTargets.end(); Iter++)
	{
		switch (*Iter)
		{
			case ShaderUsage::DEFAULT:
				SafeRelease(DefaultInputLayout);
				SafeRelease(DefaultVertexShader);
				SafeRelease(DefaultPixelShader);
				CreateDefaultShader();
				break;
			case ShaderUsage::TEXTURE:
				SafeRelease(TextureInputLayout);
				SafeRelease(TextureVertexShader);
				SafeRelease(TexturePixelShader);
				CreateTextureShader();
				for (FRenderPass* RenderPass : RenderPasses)
				{
					if (auto* BillboardPass = dynamic_cast<FBillboardPass*>(RenderPass))
					{
						BillboardPass->SetInputLayout(TextureInputLayout);
						BillboardPass->SetVertexShader(TextureVertexShader);
						BillboardPass->SetPixelShader(TexturePixelShader);
						break;
					}
				}
				break;
			case ShaderUsage::DECAL:
				SafeRelease(DecalInputLayout);
				SafeRelease(DecalVertexShader);
				SafeRelease(DecalPixelShader);
				CreateDecalShader();
				for (FRenderPass* RenderPass : RenderPasses)
				{
					if (auto* DecalPass = dynamic_cast<FDecalPass*>(RenderPass))
					{
						DecalPass->SetInputLayout(DecalInputLayout);
						DecalPass->SetVertexShader(DecalVertexShader);
						DecalPass->SetPixelShader(DecalPixelShader);
						break;
					}
				}
				break;
			case ShaderUsage::FOG:
				SafeRelease(FogInputLayout);
				SafeRelease(FogVertexShader);
				SafeRelease(FogPixelShader);
				CreateFogShader();
				for (FRenderPass* RenderPass : RenderPasses)
				{
					if (auto* FogPass = dynamic_cast<FFogPass*>(RenderPass))
					{
						FogPass->SetInputLayout(FogInputLayout);
						FogPass->SetVertexShader(FogVertexShader);
						FogPass->SetPixelShader(FogPixelShader);
						break;
					}
				}
				break;
			case ShaderUsage::FXAA:
				SafeRelease(FXAAInputLayout);
				SafeRelease(FXAAVertexShader);
				SafeRelease(FXAAPixelShader);
				SafeRelease(FXAASamplerState);
				CreateFXAAShader();
				if (FXAAPass)
				{
					FXAAPass->SetInputLayout(FXAAInputLayout);
					FXAAPass->SetVertexShader(FXAAVertexShader);
					FXAAPass->SetPixelShader(FXAAPixelShader);
					FXAAPass->SetSamplerState(FXAASamplerState);
				}
				break;
			case ShaderUsage::STATICMESH:
				SafeRelease(UberLitInputLayout);
				SafeRelease(UberLitVertexShader);
				SafeRelease(UberLitVertexShaderGouraud);
				SafeRelease(UberLitPixelShader);
				SafeRelease(UberLitPixelShaderGouraud);
				SafeRelease(UberLitPixelShaderBlinnPhong);
				SafeRelease(UberLitPixelShaderWorldNormal);
				CreateStaticMeshShader();
				for (FRenderPass* RenderPass : RenderPasses)
				{
					if (auto* StaticMeshPass = dynamic_cast<FStaticMeshPass*>(RenderPass))
					{
						StaticMeshPass->SetInputLayout(UberLitInputLayout);
						StaticMeshPass->SetVertexShader(UberLitVertexShader);
						StaticMeshPass->SetPixelShader(UberLitPixelShader);
						break;
					}
				}
				break;
			case ShaderUsage::GIZMO:
				SafeRelease(GizmoInputLayout);
				SafeRelease(GizmoVS);
				SafeRelease(GizmoPS);
				CreateGizmoShader();
				for (FRenderPass* RenderPass : RenderPasses)
				{
					if (auto* LightPass = dynamic_cast<FLightPass*>(RenderPass))
					{
						LightPass->SetInputLayout(GizmoInputLayout);
						LightPass->SetVertexShader(GizmoVS);
						LightPass->SetPixelShader(GizmoPS);
						break;
					}
				}
				break;
			case ShaderUsage::CLUSTERED_RENDERING_GRID:
				SafeRelease(ClusteredRenderingGridInputLayout);
				SafeRelease(ClusteredRenderingGridVS);
				SafeRelease(ClusteredRenderingGridPS);
				CreateClusteredRenderingGrid();
				if (ClusteredRenderingGridPass)
				{
					ClusteredRenderingGridPass->SetVertexShader(ClusteredRenderingGridVS);
					ClusteredRenderingGridPass->SetPixelShader(ClusteredRenderingGridPS);
					ClusteredRenderingGridPass->SetInputLayout(ClusteredRenderingGridInputLayout);
				}
				break;
		}
	}
}

void URenderer::ReleaseDefaultShader()
{
	SafeRelease(UberLitInputLayout);
	SafeRelease(UberLitPixelShader);
	SafeRelease(UberLitPixelShaderGouraud);
	SafeRelease(UberLitPixelShaderBlinnPhong);
	SafeRelease(UberLitPixelShaderWorldNormal);
	SafeRelease(UberLitVertexShader);
	SafeRelease(UberLitVertexShaderGouraud);
	
	SafeRelease(DefaultInputLayout);
	SafeRelease(DefaultPixelShader);
	SafeRelease(DefaultVertexShader);
	
	SafeRelease(TextureInputLayout);
	SafeRelease(TexturePixelShader);
	SafeRelease(TextureVertexShader);
	
	SafeRelease(DecalVertexShader);
	SafeRelease(DecalPixelShader);
	SafeRelease(DecalInputLayout);
	
	SafeRelease(FogVertexShader);
	SafeRelease(FogPixelShader);
	SafeRelease(FogInputLayout);
	
	SafeRelease(FXAAVertexShader);
	SafeRelease(FXAAPixelShader);
	SafeRelease(FXAAInputLayout);

	SafeRelease(GizmoVS);
	SafeRelease(GizmoPS);
	SafeRelease(GizmoInputLayout);

	SafeRelease(ClusteredRenderingGridInputLayout);
	SafeRelease(ClusteredRenderingGridVS);
	SafeRelease(ClusteredRenderingGridPS);
}

void URenderer::ReleaseDepthStencilState()
{
	SafeRelease(DefaultDepthStencilState);
	SafeRelease(DecalDepthStencilState);
	SafeRelease(DisabledDepthStencilState);
	if (GetDeviceContext())
	{
		GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}
}

void URenderer::ReleaseBlendState()
{
    SafeRelease(AlphaBlendState);
	SafeRelease(AdditiveBlendState);
}

void URenderer::ReleaseSamplerState()
{
	SafeRelease(FXAASamplerState);
	SafeRelease(DefaultSampler);
}

void URenderer::Update()
{
	// 토글에 따라서 FXAA bool값 세팅
    if (const ULevel* CurrentLevel = GWorld->GetLevel())
    {
        bFXAAEnabled = (CurrentLevel->GetShowFlags() & EEngineShowFlags::SF_FXAA) != 0;
    }
    else
    {
        bFXAAEnabled = true;
    }

    RenderBegin();

    for (FViewport* Viewport : UViewportManager::GetInstance().GetViewports())
    {
        if (Viewport->GetRect().Width < 1.0f || Viewport->GetRect().Height < 1.0f) { continue; }

    	FRect SingleWindowRect = Viewport->GetRect();
    	const int32 ViewportToolBarHeight = 32;
    	D3D11_VIEWPORT LocalViewport = { (float)SingleWindowRect.Left,(float)SingleWindowRect.Top + ViewportToolBarHeight, (float)SingleWindowRect.Width, (float)SingleWindowRect.Height - ViewportToolBarHeight, 0.0f, 1.0f };
    	GetDeviceContext()->RSSetViewports(1, &LocalViewport);
		Viewport->SetRenderRect(LocalViewport);
        UCamera* CurrentCamera = Viewport->GetViewportClient()->GetCamera();
    	
        CurrentCamera->Update(LocalViewport);
    	
        FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferViewProj, CurrentCamera->GetFViewProjConstants());
        Pipeline->SetConstantBuffer(1, EShaderType::VS, ConstantBufferViewProj);
        {
            TIME_PROFILE(RenderLevel)
            RenderLevel(Viewport);
        }
		{
			TIME_PROFILE(RenderEditor)
				GEditor->GetEditorModule()->RenderEditor();
		}
        // Gizmo는 최종적으로 렌더
        GEditor->GetEditorModule()->RenderGizmo(CurrentCamera);
    }

    // 모든 지오메트리 패스가 끝난 직후, UI/오버레이를 그리기 전 실행
	// FXAAPass->Execute의 RenderingContext는 쓰레기 값
	// TODO : 포스트 프로세스 패스를 따로 파야할지도
    if (bFXAAEnabled)
    {
        ID3D11RenderTargetView* nullRTV[] = { nullptr };
        GetDeviceContext()->OMSetRenderTargets(1, nullRTV, nullptr);

        FRenderingContext RenderingContext;
        FXAAPass->Execute(RenderingContext);
    }

    {
        TIME_PROFILE(UUIManager)
        UUIManager::GetInstance().Render();
    }
    {
        TIME_PROFILE(UStatOverlay)
        UStatOverlay::GetInstance().Render();
    }

    RenderEnd();
}

void URenderer::RenderBegin() const
{
	auto* RenderTargetView = DeviceResources->GetRenderTargetView();
	if (!RenderTargetView)
	{
		UE_LOG_ERROR("RenderBegin: RenderTargetView is null!");
		return;
	}
	GetDeviceContext()->ClearRenderTargetView(RenderTargetView, ClearColor);

	// @TODO: The clear color for the normal buffer should be a specific value (e.g., {0.5, 0.5, 1.0, 1.0})
	auto* NormalRenderTargetView = DeviceResources->GetNormalRenderTargetView();
	if (!NormalRenderTargetView)
	{
		UE_LOG_ERROR("RenderBegin: NormalRenderTargetView is null!");
		return;
	}
	GetDeviceContext()->ClearRenderTargetView(NormalRenderTargetView, ClearColor);

	auto* DepthStencilView = DeviceResources->GetDepthStencilView();
	if (!DepthStencilView)
	{
		UE_LOG_ERROR("RenderBegin: DepthStencilView is null!");
		return;
	}
	GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// FXAA bool값 변수에 따라서 RTV세팅
    if (bFXAAEnabled)
    {
        auto* SceneColorRenderTargetView = DeviceResources->GetSceneColorRenderTargetView();
        auto* NormalRenderTargetView = DeviceResources->GetNormalRenderTargetView();
        GetDeviceContext()->ClearRenderTargetView(SceneColorRenderTargetView, ClearColor);
        GetDeviceContext()->ClearRenderTargetView(NormalRenderTargetView, ClearColor);
        GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        ID3D11RenderTargetView* rtvs[] = { SceneColorRenderTargetView, NormalRenderTargetView };
        GetDeviceContext()->OMSetRenderTargets(2, rtvs, DepthStencilView);
    }
    else
    {
        auto* RenderTargetView = DeviceResources->GetRenderTargetView();
        auto* NormalRenderTargetView = DeviceResources->GetNormalRenderTargetView();
        GetDeviceContext()->ClearRenderTargetView(RenderTargetView, ClearColor);
        GetDeviceContext()->ClearRenderTargetView(NormalRenderTargetView, ClearColor);
        GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
        ID3D11RenderTargetView* rtvs[] = { RenderTargetView, NormalRenderTargetView };
        GetDeviceContext()->OMSetRenderTargets(2, rtvs, DepthStencilView);
    }

    DeviceResources->UpdateViewport();
}

void URenderer::RenderLevel(FViewport* InViewport)
{
	const ULevel* CurrentLevel = GWorld->GetLevel();
	if (!CurrentLevel) { return; }

	const FCameraConstants& ViewProj = InViewport->GetViewportClient()->GetCamera()->GetFViewProjConstants();
	static bool bCullingEnabled = false; // 임시 토글(초기값: 컬링 비활성)
	TArray<UPrimitiveComponent*> FinalVisiblePrims;
	if (!bCullingEnabled)
	{
		// 1) 옥트리(정적 프리미티브) 전부 수집
		if (FOctree* StaticOctree = GWorld->GetLevel()->GetStaticOctree())
		{
			TArray<UPrimitiveComponent*> AllStatics;
			StaticOctree->GetAllPrimitives(AllStatics);
			for (UPrimitiveComponent* Primitive : AllStatics)
			{
				if (Primitive && Primitive->IsVisible())
				{
					FinalVisiblePrims.push_back(Primitive);
				}
			}
		}
		// 2) 동적 프리미티브 전부 수집
		TArray<UPrimitiveComponent*>& DynamicPrimitives = GWorld->GetLevel()->GetDynamicPrimitives();
		for (UPrimitiveComponent* Primitive : DynamicPrimitives)
		{
			if (Primitive && Primitive->IsVisible())
			{
				FinalVisiblePrims.push_back(Primitive);
			}
		}
	}
	else
	{
		FinalVisiblePrims = InViewport->GetViewportClient()->GetCamera()->GetViewVolumeCuller().GetRenderableObjects();
	}
	FRenderingContext RenderingContext(

		&ViewProj,
		InViewport->GetViewportClient()->GetCamera(),
		InViewport->GetViewportClient()->GetViewMode(),
		CurrentLevel->GetShowFlags(),
		InViewport->GetRenderRect(),
		{DeviceResources->GetViewportInfo().Width, DeviceResources->GetViewportInfo().Height}
		);
	// 1. Sort visible primitive components
	RenderingContext.AllPrimitives = FinalVisiblePrims;
	for (auto& Prim : FinalVisiblePrims)
	{
		if (auto StaticMesh = Cast<UStaticMeshComponent>(Prim))
		{
			RenderingContext.StaticMeshes.push_back(StaticMesh);
		}
		else if (auto BillBoard = Cast<UBillBoardComponent>(Prim))
		{
			RenderingContext.BillBoards.push_back(BillBoard);
		}
		else if (auto Text = Cast<UTextComponent>(Prim))
		{
			if (!Text->IsExactly(UUUIDTextComponent::StaticClass())) { RenderingContext.Texts.push_back(Text); }
			else { RenderingContext.UUIDs.push_back(Cast<UUUIDTextComponent>(Text)); }
		}
		else if (auto Decal = Cast<UDecalComponent>(Prim))
		{
			RenderingContext.Decals.push_back(Decal);
		}
	}
	
	for (const auto& LightComponent : CurrentLevel->GetLightComponents())
	{
		if (auto PointLightComponent = Cast<UPointLightComponent>(LightComponent))
		{
			if (auto SpotLightComponent = Cast<USpotLightComponent>(LightComponent))
			{
				RenderingContext.SpotLights.push_back(SpotLightComponent);
			}
			else
			{
				RenderingContext.PointLights.push_back(PointLightComponent);
			}
		}
		if (auto DirectionalLightComponent = Cast<UDirectionalLightComponent>(LightComponent))
		{
			RenderingContext.DirectionalLights.push_back(DirectionalLightComponent);
		}
		
		if (auto AmbientLightComponent = Cast<UAmbientLightComponent>(LightComponent))
		{
			RenderingContext.AmbientLights.push_back(AmbientLightComponent);
		}
	}

	// 2. Collect HeightFogComponents from all actors in the level
	for (const auto& Actor : CurrentLevel->GetLevelActors())
	{
		for (const auto& Component : Actor->GetOwnedComponents())
		{
			if (auto Fog = Cast<UHeightFogComponent>(Component))
			{
				RenderingContext.Fogs.push_back(Fog);
			}
		}
	}

	for (auto RenderPass: RenderPasses)
	{
		RenderPass->Execute(RenderingContext);
	}
}

void URenderer::RenderEditorPrimitive(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState, uint32 InStride, uint32 InIndexBufferStride)
{
    // Use the global stride if InStride is 0
    const uint32 FinalStride = (InStride == 0) ? Stride : InStride;

    // Allow for custom shaders, fallback to default
    FPipelineInfo PipelineInfo = {
        InPrimitive.InputLayout ? InPrimitive.InputLayout : DefaultInputLayout,
        InPrimitive.VertexShader ? InPrimitive.VertexShader : DefaultVertexShader,
		FRenderResourceFactory::GetRasterizerState(InRenderState),
        InPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : DefaultDepthStencilState,
        InPrimitive.PixelShader ? InPrimitive.PixelShader : DefaultPixelShader,
        nullptr,
        InPrimitive.Topology
    };
    Pipeline->UpdatePipeline(PipelineInfo);

    // Update constant buffers
	FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModels,
		FMatrix::GetModelMatrix(InPrimitive.Location, InPrimitive.Rotation, InPrimitive.Scale));
	Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModels);
	Pipeline->SetConstantBuffer(1, EShaderType::VS, ConstantBufferViewProj);
	
	FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferColor, InPrimitive.Color);
	Pipeline->SetConstantBuffer(2, EShaderType::PS, ConstantBufferColor);
	Pipeline->SetConstantBuffer(2, EShaderType::VS, ConstantBufferColor);
	
    Pipeline->SetVertexBuffer(InPrimitive.VertexBuffer, FinalStride);

    // The core logic: check for an index buffer
    if (InPrimitive.IndexBuffer && InPrimitive.NumIndices > 0)
    {
        Pipeline->SetIndexBuffer(InPrimitive.IndexBuffer, InIndexBufferStride);
        Pipeline->DrawIndexed(InPrimitive.NumIndices, 0, 0);
    }
    else
    {
        Pipeline->Draw(InPrimitive.NumVertices, 0);
    }
}

void URenderer::RenderEnd() const
{
	TIME_PROFILE(DrawCall)
	GetSwapChain()->Present(0, 0);
	TIME_PROFILE_END(DrawCall)
}

void URenderer::OnResize(uint32 InWidth, uint32 InHeight) const
{
    if (!DeviceResources || !GetDeviceContext() || !GetSwapChain()) return;

    DeviceResources->ReleaseSceneColorTarget();
	DeviceResources->ReleaseFrameBuffer();
	DeviceResources->ReleaseDepthBuffer();
	DeviceResources->ReleaseNormalBuffer();
	GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);

    if (FAILED(GetSwapChain()->ResizeBuffers(2, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, 0)))
    {
        UE_LOG("OnResize Failed");
        return;
    }

	DeviceResources->UpdateViewport();
    DeviceResources->CreateSceneColorTarget();
	DeviceResources->CreateFrameBuffer();
	DeviceResources->CreateDepthBuffer();
	DeviceResources->CreateNormalBuffer();

    ID3D11RenderTargetView* targetView = bFXAAEnabled
        ? DeviceResources->GetSceneColorRenderTargetView()
        : DeviceResources->GetRenderTargetView();
    ID3D11RenderTargetView* targetViews[] = { targetView };
    GetDeviceContext()->OMSetRenderTargets(1, targetViews, DeviceResources->GetDepthStencilView());
}

ID3D11VertexShader* URenderer::GetVertexShader(EViewModeIndex ViewModeIndex) const
{
	if (ViewModeIndex == EViewModeIndex::VMI_Gouraud)
	{
		return UberLitVertexShaderGouraud;
	}
	else if (ViewModeIndex == EViewModeIndex::VMI_Lambert
		|| ViewModeIndex == EViewModeIndex::VMI_BlinnPhong
		|| ViewModeIndex == EViewModeIndex::VMI_WorldNormal)
	{
		return UberLitVertexShader;
	}
	else if (ViewModeIndex == EViewModeIndex::VMI_Unlit || ViewModeIndex == EViewModeIndex::VMI_SceneDepth)
	{
		return TextureVertexShader;
	}
	// 기본값 반환 (모든 조건에 해당하지 않을 경우)
	return UberLitVertexShader;
}

ID3D11PixelShader* URenderer::GetPixelShader(EViewModeIndex ViewModeIndex) const
{
	if (ViewModeIndex == EViewModeIndex::VMI_Gouraud)
	{
		return UberLitPixelShaderGouraud;
	}
	else if (ViewModeIndex == EViewModeIndex::VMI_Lambert)
	{
		return UberLitPixelShader;
	}
	else if (ViewModeIndex == EViewModeIndex::VMI_BlinnPhong)
	{
		return UberLitPixelShaderBlinnPhong;
	}
	else if (ViewModeIndex == EViewModeIndex::VMI_Unlit || ViewModeIndex == EViewModeIndex::VMI_SceneDepth)
	{
		return TexturePixelShader;
	}
	else if (ViewModeIndex == EViewModeIndex::VMI_WorldNormal)
	{
		return UberLitPixelShaderWorldNormal;
	}
	// 기본값 반환 (모든 조건에 해당하지 않을 경우)
	return UberLitPixelShader;
}

void URenderer::CreateConstantBuffers()
{
	ConstantBufferModels = FRenderResourceFactory::CreateConstantBuffer<FMatrix>();
	ConstantBufferColor = FRenderResourceFactory::CreateConstantBuffer<FVector4>();
	ConstantBufferViewProj = FRenderResourceFactory::CreateConstantBuffer<FCameraConstants>();
}

void URenderer::ReleaseConstantBuffers()
{
	SafeRelease(ConstantBufferModels);
	SafeRelease(ConstantBufferColor);
	SafeRelease(ConstantBufferViewProj);
}
