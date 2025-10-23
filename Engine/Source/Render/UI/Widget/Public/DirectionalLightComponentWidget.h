#pragma once

#include "Widget.h"

class UDirectionalLightComponent;

UCLASS()
class UDirectionalLightComponentWidget : public UWidget
{
    GENERATED_BODY()
    DECLARE_CLASS(UDirectionalLightComponentWidget, UWidget)
    
public:
    UDirectionalLightComponentWidget() = default;
    
    virtual ~UDirectionalLightComponentWidget() = default;
    
    /*-----------------------------------------------------------------------------
        UWidget Features
     -----------------------------------------------------------------------------*/
public:
    virtual void Initialize() override;
    virtual void Update() override;
    virtual void RenderWidget() override;

    /*-----------------------------------------------------------------------------
        UDirectionalLightComponentWidget Features
     -----------------------------------------------------------------------------*/
private:
    UDirectionalLightComponent* DirectionalLightComponent = nullptr;
};
