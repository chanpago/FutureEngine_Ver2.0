#pragma once
#include "Actor/Public/Light.h"
#include "Component/Public/PointLightComponent.h"


UCLASS()
class APointLight : public ALight
{
    GENERATED_BODY()
    DECLARE_CLASS(APointLight, ALight)

public:
    APointLight();

public:
    virtual UClass* GetDefaultRootComponent() override;
    
private:
    UPointLightComponent* PointLightComponent = nullptr;
};




