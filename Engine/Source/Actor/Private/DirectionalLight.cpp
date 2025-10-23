#include "pch.h"
#include "Actor/Public/Light.h"
#include "Actor/Public/DirectionalLight.h"

IMPLEMENT_CLASS(ADirectionalLight, ALight)

ADirectionalLight::ADirectionalLight()
{
}

UClass* ADirectionalLight::GetDefaultRootComponent()
{
    return UDirectionalLightComponent::StaticClass();
}
