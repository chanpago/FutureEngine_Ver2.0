#include "pch.h"
#include "Actor/Public/TriangleActor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"

IMPLEMENT_CLASS(ATriangleActor, AActor)

ATriangleActor::ATriangleActor()
{
}

UClass* ATriangleActor::GetDefaultRootComponent()
{
    return UStaticMeshComponent::StaticClass();
}

void ATriangleActor::InitializeComponents()
{
    Super::InitializeComponents();
    Cast<UStaticMeshComponent>(GetRootComponent())->SetStaticMesh("Data/Shapes/Triangle.obj");
}
