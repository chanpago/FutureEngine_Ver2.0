#pragma once
#include "Component/Public/DecalComponent.h"
struct FSpotLightOBB;

UCLASS()
class UDecalSpotLightComponent : public UDecalComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UDecalSpotLightComponent, UDecalComponent)

public:
	UDecalSpotLightComponent();
	~UDecalSpotLightComponent();

public:
	void TickComponent(float DeltaTime) override;
	void UpdateProjectionMatrix() override;
	virtual UObject* Duplicate() override;
	
	virtual const IBoundingVolume* GetBoundingBox() override;
	FSpotLightOBB* GetSpotLightBoundingBox() { return SpotLightBoundingBox; }

private:
	FSpotLightOBB* SpotLightBoundingBox = nullptr;
};

