#pragma once
#include "Actor/Public/Light.h"
#include "Component/Public/AmbientLightComponent.h"


UCLASS()
class AAmbientLight : public ALight
{
    GENERATED_BODY()
    DECLARE_CLASS(AAmbientLight, ALight)

public:
    AAmbientLight();

public:
    virtual UClass* GetDefaultRootComponent() override;
    
private:
    UAmbientLightComponent* AmbientLightComponent = nullptr;
};




