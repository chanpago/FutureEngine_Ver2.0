#pragma once

#include "Widget.h"

class UClass;
class UPointLightComponent;

UCLASS()
class UPointLightComponentWidget : public UWidget
{
    GENERATED_BODY()
    DECLARE_CLASS(UPointLightComponentWidget, UWidget)
    
public:
    UPointLightComponentWidget() = default;
    
    virtual ~UPointLightComponentWidget() = default;
    
    /*-----------------------------------------------------------------------------
        UWidget Features
     -----------------------------------------------------------------------------*/
public:
    virtual void Initialize() override;
    virtual void Update() override;
    virtual void RenderWidget() override;

    /*-----------------------------------------------------------------------------
        UPointLightComponentWidget Features
     -----------------------------------------------------------------------------*/
private:
    UPointLightComponent* PointLightComponent = nullptr;
};
