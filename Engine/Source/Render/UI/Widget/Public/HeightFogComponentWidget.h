#pragma once
#include "Widget.h"

class UHeightFogComponent;

UCLASS()
class UHeightFogComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UHeightFogComponentWidget, UWidget)
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	UHeightFogComponentWidget() = default;
	~UHeightFogComponentWidget() override = default;

private:
	UHeightFogComponent* FogComponent{};
};