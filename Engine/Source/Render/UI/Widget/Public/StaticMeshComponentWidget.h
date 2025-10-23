#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class UStaticMeshComponent;
class UMaterial;

UCLASS()
class UStaticMeshComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMeshComponentWidget, UWidget)

public:
	void Initialize() override {}
	void Update() override {}
	void RenderWidget() override;

private:
	UStaticMeshComponent* StaticMeshComponent{};

	// Helper functions for rendering different sections
	void RenderStaticMeshSelector();
	void RenderMaterialSections();
	void RenderAvailableMaterials(int32 TargetSlotIndex);
	void RenderOptions();

	// Material utility functions
	FString GetMaterialDisplayName(UMaterial* Material) const;
	UTexture* GetPreviewTextureForMaterial(UMaterial* Material) const;
};
