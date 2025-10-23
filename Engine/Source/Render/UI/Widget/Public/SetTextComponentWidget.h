#pragma once
#include "Widget.h"

class UClass;
class UTextComponent;

class USetTextComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(USetTextComponentWidget, UWidget)
public:
	USetTextComponentWidget() = default;
	~USetTextComponentWidget() override = default;
	
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

private:
	UTextComponent* SelectedTextComponent = nullptr;
};