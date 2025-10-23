#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/TextActor.h"

IMPLEMENT_CLASS(ATextActor, AActor)

ATextActor::ATextActor()
{
}

UClass* ATextActor::GetDefaultRootComponent()
{
    return UTextComponent::StaticClass();
}
