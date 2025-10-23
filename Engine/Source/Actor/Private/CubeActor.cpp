#include "pch.h"
#include "Actor/Public/CubeActor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"

IMPLEMENT_CLASS(ACubeActor, AActor)

ACubeActor::ACubeActor()
{
}

UClass* ACubeActor::GetDefaultRootComponent()
{
    return UStaticMeshComponent::StaticClass();
}

void ACubeActor::InitializeComponents()
{
    Super::InitializeComponents();
    Cast<UStaticMeshComponent>(GetRootComponent())->SetStaticMesh("Data/Shapes/Cube.obj");
}
