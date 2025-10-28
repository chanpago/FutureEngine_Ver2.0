#include "pch.h"
#include "Render/RenderPass/Public/StaticMeshPass.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Editor/Public/Camera.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Texture/Public/Texture.h"
#include "Render/RenderPass/Public/UpdateLightBufferPass.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Component/Public/SpotLightComponent.h"

FStaticMeshPass::FStaticMeshPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel,
	ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11DepthStencilState* InDS)
	: FRenderPass(InPipeline, InConstantBufferCamera, InConstantBufferModel), VS(InVS), PS(InPS), InputLayout(InLayout), DS(InDS)
{
	ConstantBufferMaterial = FRenderResourceFactory::CreateConstantBuffer<FMaterialConstants>();
	ConstantBufferShadowMap = FRenderResourceFactory::CreateConstantBuffer<FShadowMapConstants>();
	ConstantBufferSpotShadow = FRenderResourceFactory::CreateConstantBuffer<FSpotShadowConstants>();
}

void FStaticMeshPass::Execute(FRenderingContext& Context)
{
	const auto& Renderer = URenderer::GetInstance();
	FRenderState RenderState = UStaticMeshComponent::GetClassDefaultRenderState();

	// +-+-+ SET UP THE PIPELINE (SHADERS, RASTERIZER STAGE) +-+-+
	if (Context.ViewMode == EViewModeIndex::VMI_Wireframe)
	{
		RenderState.CullMode = ECullMode::None;
		RenderState.FillMode = EFillMode::WireFrame;
	}
	else {
		VS = Renderer.GetVertexShader(Context.ViewMode);
		PS = Renderer.GetPixelShader(Context.ViewMode);
	}
	if (!VS || !PS) { UE_LOG_ERROR("StaticMeshPass: missing shaders"); return; }

	ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState(RenderState);
	FPipelineInfo PipelineInfo = { InputLayout, VS, RS, DS, PS, nullptr, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	Pipeline->UpdatePipeline(PipelineInfo);
	Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);
	Pipeline->SetConstantBuffer(1, EShaderType::VS, ConstantBufferCamera);
	Pipeline->SetConstantBuffer(1, EShaderType::PS, ConstantBufferCamera);

	// +-+-+ BIND SHADOW MAP CONSTANT BUFFER AND SRV +-+-+
	FUpdateLightBufferPass* LightBufferPass = dynamic_cast<FUpdateLightBufferPass*>(Renderer.GetRenderPasses()[0]);
	UDirectionalLightComponent* Light = Context.DirectionalLights.empty() ? nullptr : Context.DirectionalLights[0];
	if (LightBufferPass && Light && (Context.ShowFlags & EEngineShowFlags::SF_Shadow))
	{
		const EShadowProjectionType ProjectionType = Context.ShadowProjectionType;
		const EShadowFilterType FilterType = Context.ShadowFilterType;

		ID3D11ShaderResourceView* ShadowMapSRV = nullptr;
		FShadowMapConstants ShadowConsts = {};

		ShadowConsts.bUsePCF = (FilterType == EShadowFilterType::PCF) ? 1.0f : 0.0f;
		ShadowConsts.bUseVSM = (FilterType == EShadowFilterType::VSM) ? 1.0f : 0.0f;
		ShadowConsts.bUsePSM = (ProjectionType == EShadowProjectionType::PSM) ? 1.0f : 0.0f;
		ShadowConsts.bUseCSM = (ProjectionType == EShadowProjectionType::CSM) ? 1.0f : 0.0f;
		ShadowConsts.ShadowParams.X = (FilterType == EShadowFilterType::VSM) ? 0.0015f : 0.005f;

		switch (ProjectionType)
		{
		case EShadowProjectionType::Default:
		case EShadowProjectionType::PSM:
		{
			// ★ PSM 베이킹 시 사용한 카메라 V/P를 그대로 사용 (shading과 베이킹의 카메라 일치 보장)
			ShadowConsts.EyeView = LightBufferPass->GetCachedEyeView();
			ShadowConsts.EyeProj = LightBufferPass->GetCachedEyeProj();
			ShadowConsts.EyeViewProjInv = (ShadowConsts.EyeView * ShadowConsts.EyeProj).Inverse();

			ShadowConsts.LightViewP[0] = LightBufferPass->GetLightViewMatrix();
			ShadowConsts.LightProjP[0] = LightBufferPass->GetLightProjectionMatrix();
			ShadowConsts.LightViewPInv[0] = ShadowConsts.LightViewP[0].Inverse();

			ShadowConsts.ShadowParams = FVector4(0.0008f, 0.0f, 0.0f, 0.0f);
			FVector LdirWS = (-Context.DirectionalLights[0]->GetForwardVector()).GetNormalized();
			ShadowConsts.LightDirWS = LdirWS;
			ShadowConsts.bInvertedLight = 0;

			ShadowConsts.LightOrthoParams = LightBufferPass->GetLightOrthoLTRB(); // (l,r,b,t)

			ShadowConsts.ShadowMapSize = FVector2(2048.0f, 2048.0f);
			ShadowConsts.bUsePSM = Context.DirectionalLights[0]->GetCastShadows();
			break;
		}
		case EShadowProjectionType::CSM:
		{
			ShadowConsts = LightBufferPass->GetCascadedShadowMapConstants();
			ShadowConsts.bUseCSM = 1.0f;
			break;
		}
		default:
			break;
		}

		if (ProjectionType == EShadowProjectionType::CSM)
		{
			ShadowMapSRV = ShadowConsts.bUseVSM
				? nullptr /* Renderer.GetDeviceResources()->GetCascadedShadowMapColorSRV() */
				: Renderer.GetDeviceResources()->GetCascadedShadowMapSRV();
		}
		else  // LVP or PSM
		{
			ShadowMapSRV = ShadowConsts.bUseVSM && !(ShadowConsts.bUsePCF)
				? Renderer.GetDeviceResources()->GetDirectionalShadowMapColorSRV()
				: Renderer.GetDeviceResources()->GetDirectionalShadowMapSRV();
		}

		//if (bUseCSM)
		//{
		//	//// Get pre-computed CSM constants from LightBufferPass
		//	//ShadowConsts = LightBufferPass->GetCascadedShadowMapConstants();
		//	//// Specify the use of CSM
		//	//ShadowConsts.bUseCSM = 1.0f;

		//	//if (bUseVSM)
		//	//{
		//	//	// CSM + VSM
		//	//	ShadowConsts.bUseVSM = 1.0f;
		//	//	ShadowConsts.ShadowParams.X = 0.0015f;  // VSM needs less bias
		//	//	// TODO: needs Texture2DArray resource of VSM format (color)
		//	//	// bShouldBindShadows = false;        // Temporarily disabled
		//	//}
		//	//else
		//	//{
		//	//	// Only CSM
		//	//	ShadowConsts.bUseVSM = 0.0f;
		//	//	ShadowConsts.ShadowParams.X = 0.005f;  // VSM needs less bias
		//	//	ShadowMapSRV = Renderer.GetDeviceResources()->GetCascadedShadowMapSRV();
		//	//}
		//}

		if (ShadowMapSRV)  // Shadow Map이 존재할 때만 바인딩
		{
			FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferShadowMap, ShadowConsts);
			Pipeline->SetConstantBuffer(6, EShaderType::PS, ConstantBufferShadowMap);

			// +-+-+ BIND CONSTANT & RESOURCES +-+-+
			if (ShadowConsts.bUseCSM)
			{
				Pipeline->SetShaderResourceView(11, EShaderType::PS, ShadowMapSRV);
			}
			else   // LVP or PSM
			{
				Pipeline->SetShaderResourceView(10, EShaderType::PS, ShadowMapSRV);
			}

			// +-+-+ SET SAMPLER +-+-+
			Pipeline->SetSamplerState(0, EShaderType::PS, Renderer.GetDefaultSampler());
			if (FilterType == EShadowFilterType::None)
			{
				Pipeline->SetSamplerState(2, EShaderType::PS, Renderer.GetShadowSampler());
			}
			else if (FilterType == EShadowFilterType::PCF)
			{
				Pipeline->SetSamplerState(10, EShaderType::PS, Renderer.GetShadowMapPCFSampler());
			}
			else if (FilterType == EShadowFilterType::VSM)
			{
				Pipeline->SetSamplerState(1, EShaderType::PS, Renderer.GetShadowMapClampSampler());
			}
		}
		else
		{
			Pipeline->SetShaderResourceView(10, EShaderType::PS, nullptr);
			Pipeline->SetShaderResourceView(11, EShaderType::PS, nullptr);
		}

		// Bind spotlight shadow (single caster) alongside directional
		if (!Context.SpotLights.empty())
		{
			USpotLightComponent* SpotCaster = nullptr;
			for (auto* SL : Context.SpotLights)
			{
				// if (SL && SL->GetCastShadows())
				// {
				SpotCaster = SL;
				// }
				if (SpotCaster)
				{
					FSpotShadowConstants SpotConsts = {};
					SpotConsts.LightView = LightBufferPass->GetSpotLightViewMatrix();
					SpotConsts.LightProj = LightBufferPass->GetSpotLightProjectionMatrix();
					SpotConsts.SpotPosition = SpotCaster->GetWorldLocation();
					SpotConsts.SpotRange = SpotCaster->GetAttenuationRadius();
					SpotConsts.SpotDirection = SpotCaster->GetForwardVector().GetNormalized();
					SpotConsts.OuterCone = SpotCaster->GetOuterConeAngle();
					SpotConsts.InnerCone = SpotCaster->GetInnerConeAngle();
					SpotConsts.ShadowMapSize = FVector2(1024.0f, 1024.0f);
					SpotConsts.ShadowBias = 0.005f;
					SpotConsts.bUseVSM = (FilterType == EShadowFilterType::VSM) ? 1 : 0;
					SpotConsts.bUsePCF = (FilterType == EShadowFilterType::PCF) ? 1 : 0;

					// Atlas info (must match DeviceResources atlas configuration)
					const float tileW = LightBufferPass->GetSpotTileWidth();
					const float tileH = LightBufferPass->GetSpotTileHeight();
					const uint32 cols = LightBufferPass->GetSpotAtlasCols();
					const uint32 rows = LightBufferPass->GetSpotAtlasRows();
					SpotConsts.SpotAtlasTextureSize = FVector2(tileW * cols, tileH * rows);
					SpotConsts.SpotTileSize = FVector2(tileW, tileH);
					SpotConsts.SpotAtlasCols = cols;
					SpotConsts.SpotAtlasRows = rows;

					FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferSpotShadow, SpotConsts);
					Pipeline->SetConstantBuffer(7, EShaderType::PS, ConstantBufferSpotShadow);

					ID3D11ShaderResourceView* SpotSRV = Renderer.GetDeviceResources()->GetSpotShadowMapSRV();
					Pipeline->SetShaderResourceView(12, EShaderType::PS, SpotSRV);

					// Bind atlas entries buffer (t13)
					Pipeline->SetShaderResourceView(13, EShaderType::PS, LightBufferPass->GetSpotShadowAtlasSRV());

					// Sampler (reuse same policy)
					if (FilterType == EShadowFilterType::None)
					{
						Pipeline->SetSamplerState(2, EShaderType::PS, Renderer.GetShadowSampler());
					}
					else if (FilterType == EShadowFilterType::PCF)
					{
						Pipeline->SetSamplerState(10, EShaderType::PS, Renderer.GetShadowMapPCFSampler());
					}
					else if (FilterType == EShadowFilterType::VSM)
					{
						Pipeline->SetSamplerState(1, EShaderType::PS, Renderer.GetShadowMapClampSampler());
					}
				}
			}
		}
        else
        {
            // No spot shadow caster: clear bindings to avoid sampling invalid SRV/CB
            Pipeline->SetShaderResourceView(12, EShaderType::PS, nullptr);
            Pipeline->SetShaderResourceView(13, EShaderType::PS, nullptr);
            Pipeline->SetConstantBuffer(7, EShaderType::PS, nullptr);
        }
	}

	/**
	* @todo Find a better way to reduce depdency upon Renderer class.
	* @note How about introducing methods like BeginPass(), EndPass() to set up and release pass specific state?
	*/

	// +-+ RTVS SETUP +-+
	const auto& DeviceResources = Renderer.GetDeviceResources();
	ID3D11RenderTargetView* RTV = nullptr;
	RTV = Renderer.GetFXAA() 
		? DeviceResources->GetSceneColorRenderTargetView()
		: DeviceResources->GetRenderTargetView();
	ID3D11RenderTargetView* RTVs[2] = { RTV, DeviceResources->GetNormalRenderTargetView() };
	ID3D11DepthStencilView* DSV = DeviceResources->GetDepthStencilView();
	Pipeline->SetRenderTargets(2, RTVs, DSV);

	// +-+ MESH RENDERING +-+
	RenderStaticMeshes(Context);
}

void FStaticMeshPass::RenderStaticMeshes(FRenderingContext& Context)
{
	if (!(Context.ShowFlags & EEngineShowFlags::SF_StaticMesh)) { return; }

	TArray<UStaticMeshComponent*>& MeshComponents = Context.StaticMeshes;
	sort(MeshComponents.begin(), MeshComponents.end(),
		[](UStaticMeshComponent* A, UStaticMeshComponent* B) {
			int32 MeshA = A->GetStaticMesh() ? A->GetStaticMesh()->GetAssetPathFileName().GetComparisonIndex() : 0;
			int32 MeshB = B->GetStaticMesh() ? B->GetStaticMesh()->GetAssetPathFileName().GetComparisonIndex() : 0;
			return MeshA < MeshB;
		});

	FStaticMesh* CurrentMeshAsset = nullptr;
	UMaterial* CurrentMaterial = nullptr;

	for (UStaticMeshComponent* MeshComp : MeshComponents)
	{
		if (!MeshComp->IsVisible()) { continue; }
		if (!MeshComp->GetStaticMesh()) { continue; }
		FStaticMesh* MeshAsset = MeshComp->GetStaticMesh()->GetStaticMeshAsset();
		if (!MeshAsset) { continue; }

		if (CurrentMeshAsset != MeshAsset)
		{
			Pipeline->SetVertexBuffer(MeshComp->GetVertexBuffer(), sizeof(FNormalVertex));
			Pipeline->SetIndexBuffer(MeshComp->GetIndexBuffer(), 0);
			CurrentMeshAsset = MeshAsset;
		}

		FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, MeshComp->GetWorldTransformMatrix());
		Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

		if (MeshAsset->MaterialInfo.empty() || MeshComp->GetStaticMesh()->GetNumMaterials() == 0)
		{
			Pipeline->DrawIndexed(MeshAsset->Indices.size(), 0, 0);
			continue;
		}

		if (MeshComp->IsScrollEnabled())
		{
			MeshComp->SetElapsedTime(MeshComp->GetElapsedTime() + UTimeManager::GetInstance().GetDeltaTime());
		}

		for (const FMeshSection& Section : MeshAsset->Sections)
		{
			UMaterial* Material = MeshComp->GetMaterial(Section.MaterialSlot);
			if (CurrentMaterial != Material) {
				FMaterialConstants MaterialConstants = {};
				FVector AmbientColor = Material->GetAmbientColor(); MaterialConstants.Ka = FVector4(AmbientColor.X, AmbientColor.Y, AmbientColor.Z, 1.0f);
				FVector DiffuseColor = Material->GetDiffuseColor(); MaterialConstants.Kd = FVector4(DiffuseColor.X, DiffuseColor.Y, DiffuseColor.Z, 1.0f);
				FVector SpecularColor = Material->GetSpecularColor(); MaterialConstants.Ks = FVector4(SpecularColor.X, SpecularColor.Y, SpecularColor.Z, 1.0f);
				MaterialConstants.Ns = Material->GetSpecularExponent();
				MaterialConstants.Ni = Material->GetRefractionIndex();
				MaterialConstants.D = Material->GetDissolveFactor();
				MaterialConstants.MaterialFlags = 0;
				if (Material->GetDiffuseTexture())  { MaterialConstants.MaterialFlags |= HAS_DIFFUSE_MAP; }
				if (Material->GetAmbientTexture())  { MaterialConstants.MaterialFlags |= HAS_AMBIENT_MAP; }
				if (Material->GetSpecularTexture()) { MaterialConstants.MaterialFlags |= HAS_SPECULAR_MAP; }
				if (Material->GetNormalTexture())   { MaterialConstants.MaterialFlags |= HAS_NORMAL_MAP; }
				if (!MeshComp->IsNormalMapEnabled())
				{
					MaterialConstants.MaterialFlags &= ~HAS_NORMAL_MAP;
				}
				if (Material->GetAlphaTexture())	{ MaterialConstants.MaterialFlags |= HAS_ALPHA_MAP; }
				if (Material->GetBumpTexture())     { MaterialConstants.MaterialFlags |= HAS_BUMP_MAP; }
				MaterialConstants.Time = MeshComp->GetElapsedTime();

				FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferMaterial, MaterialConstants);
				Pipeline->SetConstantBuffer(2, EShaderType::VS | EShaderType::PS, ConstantBufferMaterial);

				if (UTexture* DiffuseTexture = Material->GetDiffuseTexture())
				{
					Pipeline->SetShaderResourceView(0, EShaderType::PS, DiffuseTexture->GetTextureSRV());
					Pipeline->SetSamplerState(0, EShaderType::PS, DiffuseTexture->GetTextureSampler());
				}
				if (UTexture* AmbientTexture = Material->GetAmbientTexture())
				{
					Pipeline->SetShaderResourceView(1, EShaderType::PS, AmbientTexture->GetTextureSRV());
				}
				if (UTexture* SpecularTexture = Material->GetSpecularTexture())
				{
					Pipeline->SetShaderResourceView(2, EShaderType::PS, SpecularTexture->GetTextureSRV());
				}
				if (Material->GetNormalTexture() && MeshComp->IsNormalMapEnabled())
				{
					Pipeline->SetShaderResourceView(3, EShaderType::PS, Material->GetNormalTexture()->GetTextureSRV());
				}
				if (UTexture* AlphaTexture = Material->GetAlphaTexture())
				{
					Pipeline->SetShaderResourceView(4, EShaderType::PS, AlphaTexture->GetTextureSRV());
				}
				if (UTexture* BumpTexture = Material->GetBumpTexture())
				{ // 범프 텍스처 추가 그러나 범프 텍스처 사용하지 않아서 없을 것임. 무시 ㄱㄱ
					Pipeline->SetShaderResourceView(5, EShaderType::PS, BumpTexture->GetTextureSRV());
					// 필요한 경우 샘플러 지정
					// Pipeline->SetSamplerState(5, false, BumpTexture->GetTextureSampler());
				}
				CurrentMaterial = Material;
			}
			Pipeline->DrawIndexed(Section.IndexCount, Section.StartIndex, 0);
		}
	}
	Pipeline->SetConstantBuffer(2, EShaderType::PS, nullptr);
}

void FStaticMeshPass::Release()
{
	SafeRelease(ConstantBufferMaterial);
	SafeRelease(ConstantBufferShadowMap);
	SafeRelease(ConstantBufferSpotShadow);
}
