#include "pch.h"
#include "Render/RenderPass/Public/StaticMeshPass.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Editor/Public/Camera.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Texture/Public/Texture.h"
#include "Render/RenderPass/Public/UpdateLightBufferPass.h"

FStaticMeshPass::FStaticMeshPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel,
	ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout, ID3D11DepthStencilState* InDS)
	: FRenderPass(InPipeline, InConstantBufferCamera, InConstantBufferModel), VS(InVS), PS(InPS), InputLayout(InLayout), DS(InDS)
{
	ConstantBufferMaterial = FRenderResourceFactory::CreateConstantBuffer<FMaterialConstants>();
	ConstantBufferShadowMap = FRenderResourceFactory::CreateConstantBuffer<FShadowMapConstants>();
}

void FStaticMeshPass::Execute(FRenderingContext& Context)
{

	const auto& Renderer = URenderer::GetInstance();
	FRenderState RenderState = UStaticMeshComponent::GetClassDefaultRenderState();
	if (Context.ViewMode == EViewModeIndex::VMI_Wireframe)
	{
		RenderState.CullMode = ECullMode::None;
		RenderState.FillMode = EFillMode::WireFrame;
	}
	else
	{
		VS = Renderer.GetVertexShader(Context.ViewMode);
		PS = Renderer.GetPixelShader(Context.ViewMode);
	}

	// ✅ 널 가드 + 로그 + 안전 종료(또는 폴백)
	if (!VS) {
		UE_LOG_ERROR("StaticMeshPass: VS null for ViewMode=%d", (int)Context.ViewMode);
		return; // 혹은 VS = GetDefaultUberVS();
	}
	if (!PS) {
		UE_LOG_ERROR("StaticMeshPass: PS null for ViewMode=%d", (int)Context.ViewMode);
		return; // 혹은 PS = GetDefaultUberPS();
	}
	
	ID3D11RasterizerState* RS = FRenderResourceFactory::GetRasterizerState(RenderState);
	FPipelineInfo PipelineInfo = { InputLayout, VS, RS, DS, PS, nullptr, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	Pipeline->UpdatePipeline(PipelineInfo);

	// ✅ 정말로 바인드됐는지 최종 확인(디버그용)
	{
		ID3D11VertexShader* curVS=nullptr; UINT cnt=0;
		Renderer.GetDeviceContext()->VSGetShader(&curVS, nullptr, &cnt);
		if (!curVS) { UE_LOG_ERROR("StaticMeshPass: VS not bound after UpdatePipeline"); return; }
		curVS->Release();
	}
	
	// Set default samplers: s0=wrap, s1=shadow(point+clamp)
	Pipeline->SetSamplerState(0, EShaderType::PS, URenderer::GetInstance().GetDefaultSampler());
	Pipeline->SetSamplerState(1, EShaderType::PS, URenderer::GetInstance().GetShadowSampler());

	Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);
	Pipeline->SetConstantBuffer(1, EShaderType::VS, ConstantBufferCamera);
	Pipeline->SetConstantBuffer(1, EShaderType::PS, ConstantBufferCamera);

	// Shadow Map 상수 버퍼 및 SRV 바인딩 (b6, t10 슬롯)
	FUpdateLightBufferPass* UpdateLightBufferPass = dynamic_cast<FUpdateLightBufferPass*>(Renderer.GetRenderPasses()[0]);
	if (UpdateLightBufferPass)
	{
		ID3D11ShaderResourceView* ShadowMapSRV = Renderer.GetDeviceResources()->GetDirectionalShadowMapSRV();
		if (ShadowMapSRV)  // Shadow Map이 존재할 때만 바인딩
		{
			FShadowMapConstants ShadowMapConsts;
			// ★ PSM 베이킹 시 사용한 카메라 V/P를 그대로 사용 (shading과 베이킹의 카메라 일치 보장)
			ShadowMapConsts.EyeView = UpdateLightBufferPass->GetCachedEyeView();
			ShadowMapConsts.EyeProj = UpdateLightBufferPass->GetCachedEyeProj();
			ShadowMapConsts.LightViewP = UpdateLightBufferPass->GetLightViewMatrix();
			ShadowMapConsts.LightProjP = UpdateLightBufferPass->GetLightProjectionMatrix();
			ShadowMapConsts.ShadowParams = FVector4(0.0008f,0.0f,0.0f,0.0f);
			ShadowMapConsts.bInvertedLight = 0;
			FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferShadowMap, ShadowMapConsts);
			
			// 바인딩 (PS b6 / t10)
			Pipeline->SetConstantBuffer(6, EShaderType::PS, ConstantBufferShadowMap);
			Pipeline->SetShaderResourceView(10, EShaderType::PS, ShadowMapSRV);
		}
	}
	
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

	// --- RTVs Setup ---
	
	/**
	 * @todo Find a better way to reduce depdency upon Renderer class.
	 * @note How about introducing methods like BeginPass(), EndPass() to set up and release pass specific state?
	 */
	const auto& DeviceResources = Renderer.GetDeviceResources();
	ID3D11RenderTargetView* RTV = nullptr;
	if (Renderer.GetFXAA())
	{
		RTV = DeviceResources->GetSceneColorRenderTargetView();	
	}
	else
	{
		RTV = DeviceResources->GetRenderTargetView();	
	}
	ID3D11RenderTargetView* RTVs[2] = { RTV, DeviceResources->GetNormalRenderTargetView() };
	ID3D11DepthStencilView* DSV = DeviceResources->GetDepthStencilView();
	Pipeline->SetRenderTargets(2, RTVs, DSV);

	// --- RTVs Setup End ---

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
				if (Material->GetAlphaTexture())    { MaterialConstants.MaterialFlags |= HAS_ALPHA_MAP; }
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

	
	// --- RTVs Reset ---
	
	/**
	 * @todo Find a better way to reduce depdency upon Renderer class.
	 * @note How about introducing methods like BeginPass(), EndPass() to set up and release pass specific state?
	 */
	Pipeline->SetRenderTargets(2, RTVs, DSV);

	// --- RTVs Reset End ---
}

void FStaticMeshPass::Release()
{
	SafeRelease(ConstantBufferMaterial);
	SafeRelease(ConstantBufferShadowMap);
}
