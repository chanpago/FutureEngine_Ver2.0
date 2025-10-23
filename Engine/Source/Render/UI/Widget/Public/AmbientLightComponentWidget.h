#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class UAmbientLightComponent;

UCLASS()
class UAmbientLightComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UAmbientLightComponentWidget, UWidget)
public:
	void Initialize() override {}
	void Update() override;
	void RenderWidget() override;

private:
	UAmbientLightComponent* AmbientLightComponent = nullptr;
};