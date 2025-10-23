#include "pch.h"
#include "Actor/Public/Light.h"
#include "Actor/Public/SpotLight.h"

IMPLEMENT_CLASS(ASpotLight, ALight)

ASpotLight::ASpotLight()
{
}

UClass* ASpotLight::GetDefaultRootComponent()
{
    return USpotLightComponent::StaticClass();
}
