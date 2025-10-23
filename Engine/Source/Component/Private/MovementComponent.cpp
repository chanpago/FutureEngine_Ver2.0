#include "pch.h"
#include "Component/Public/MovementComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_ABSTRACT_CLASS(UMovementComponent, UActorComponent)

UMovementComponent::UMovementComponent()
{
    
}

UMovementComponent::~UMovementComponent()
{
    
}

void UMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    AActor* Owner = GetOwner();
    if (Owner)
    {
        SetUpdatedComponent(Owner->GetRootComponent());
    }
}

void UMovementComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
    if (NewUpdatedComponent)
    {
        UpdatedComponent = NewUpdatedComponent;
        UpdatedPrimitive = Cast<UPrimitiveComponent>(UpdatedComponent);
        bCanEverTick = true;
    }
    else
    {
        UpdatedComponent = nullptr;
        UpdatedPrimitive = nullptr;
        bCanEverTick = false;
    }
}

void UMovementComponent::MoveUpdatedComponent(const FVector& NewDelta, const FQuaternion& NewRotation)
{
    if (UpdatedComponent)
    {
        UpdatedComponent->SetWorldLocation(UpdatedComponent->GetWorldLocation() + NewDelta);
        UpdatedComponent->SetWorldRotation(NewRotation);
    }
}

void UMovementComponent::StopMovementImmediately()
{
    Velocity = FVector::Zero();
}

void UMovementComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    
    if (bInIsLoading)
    {
        FJsonSerializer::ReadVector(InOutHandle, "Velocity", Velocity, FVector::Zero());
    }
    else
    {
        InOutHandle["Velocity"] = FJsonSerializer::VectorToJson(Velocity);
    }
}

UObject* UMovementComponent::Duplicate()
{
    UMovementComponent* MovementComponent = Cast<UMovementComponent>(Super::Duplicate());
    MovementComponent->Velocity = Velocity;
    return MovementComponent;
}
