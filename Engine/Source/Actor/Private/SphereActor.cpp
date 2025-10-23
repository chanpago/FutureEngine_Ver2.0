#include "pch.h"
#include "Actor/Public/SphereActor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"

IMPLEMENT_CLASS(ASphereActor, AActor)

ASphereActor::ASphereActor()
{
}

UClass* ASphereActor::GetDefaultRootComponent()
{
	return UStaticMeshComponent::StaticClass();
}

void ASphereActor::InitializeComponents()
{
	Super::InitializeComponents();
	Cast<UStaticMeshComponent>(GetRootComponent())->SetStaticMesh("Data/Shapes/Sphere.obj");
}
