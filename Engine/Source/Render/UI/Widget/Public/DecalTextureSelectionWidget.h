#pragma once
#include "Widget.h"

class UClass;
class UDecalComponent;

UCLASS()
class UDecalTextureSelectionWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UDecalTextureSelectionWidget, UWidget)
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Special Member Function
	UDecalTextureSelectionWidget();
	~UDecalTextureSelectionWidget() override;

	void SetDecalComponent(UDecalComponent* InComponent) { DecalComponent = InComponent; }

private:
	UDecalComponent* DecalComponent{};

	// Helper functions for rendering different sections
	void RenderMaterialSections();
	void RenderAvailableMaterials(int32 TargetSlotIndex);
	void RenderOptions();


	// Material utility functions
	FString GetMaterialDisplayName(UMaterial* Material) const;
};

