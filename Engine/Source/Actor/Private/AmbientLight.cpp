#include "pch.h"
#include "Actor/Public/Light.h"
#include "Actor/Public/AmbientLight.h"

IMPLEMENT_CLASS(AAmbientLight, ALight)

AAmbientLight::AAmbientLight()
{
}

UClass* AAmbientLight::GetDefaultRootComponent()
{
    return UAmbientLightComponent::StaticClass();
}
