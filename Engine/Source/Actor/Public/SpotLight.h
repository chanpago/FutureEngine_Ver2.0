#pragma once
#include "Actor/Public/Light.h"
#include "Component/Public/SpotLightComponent.h"


UCLASS()
class ASpotLight : public ALight
{
    GENERATED_BODY()
    DECLARE_CLASS(ASpotLight, ALight)

public:
    ASpotLight();

public:
    virtual UClass* GetDefaultRootComponent() override;
    
private:
    USpotLightComponent* SpotLightComponent = nullptr;
};




