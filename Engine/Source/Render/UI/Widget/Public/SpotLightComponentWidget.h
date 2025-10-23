#pragma once

#include "Widget.h"

class USpotLightComponent;

UCLASS()
class USpotLightComponentWidget : public UWidget
{
    GENERATED_BODY()
    DECLARE_CLASS(USpotLightComponentWidget, UWidget)
    
public:
    USpotLightComponentWidget() = default;
    
    virtual ~USpotLightComponentWidget() = default;
    
    /*-----------------------------------------------------------------------------
        UWidget Features
     -----------------------------------------------------------------------------*/
public:
    virtual void Initialize() override;
    virtual void Update() override;
    virtual void RenderWidget() override;

    /*-----------------------------------------------------------------------------
        USpotLightComponentWidget Features
     -----------------------------------------------------------------------------*/
private:
    USpotLightComponent* SpotLightComponent = nullptr;
};
