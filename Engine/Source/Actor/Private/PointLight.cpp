#include "pch.h"
#include "Actor/Public/Light.h"
#include "Actor/Public/PointLight.h"

IMPLEMENT_CLASS(APointLight, ALight)

APointLight::APointLight()
{
}

UClass* APointLight::GetDefaultRootComponent()
{
    return UPointLightComponent::StaticClass();
}
