#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/StaticMeshActor.h"

IMPLEMENT_CLASS(AStaticMeshActor, AActor)

AStaticMeshActor::AStaticMeshActor()
{
}

UClass* AStaticMeshActor::GetDefaultRootComponent()
{
	return UStaticMeshComponent::StaticClass();
}
