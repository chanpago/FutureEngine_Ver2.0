#include "pch.h"
#include "Component/Public/ProjectileMovementComponent.h"
#include "Render/UI/Widget/Public/ProjectileMovementComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(UProjectileMovementComponent, UMovementComponent)

UProjectileMovementComponent::UProjectileMovementComponent()
{
    Velocity = {1, 0, 0};
}


void UProjectileMovementComponent::BeginPlay()
{
    UMovementComponent::BeginPlay();
    
    Velocity.Normalize();
    Velocity = Velocity * InitialSpeed;
}

void UProjectileMovementComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    if (!UpdatedComponent) { return; }

    // Apply Gravity
    Velocity.Z -= GravityScale * DeltaTime;
    if (MaxSpeed > 0 && Velocity.Length() > MaxSpeed)
    {
        Velocity.Normalize();
        Velocity *= MaxSpeed;
    }

    // Rotation Calculate
    FQuaternion NewRotation = UpdatedComponent->GetWorldRotationAsQuaternion();
    if (bRotationFollowsVelocity && !Velocity.IsZero())
    {
        NewRotation = FQuaternion::MakeFromDirection(Velocity.GetNormalized());
    }

    const FVector Delta = Velocity * DeltaTime;

    MoveUpdatedComponent(Delta, NewRotation);
}

void UProjectileMovementComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    
    if (bInIsLoading)
    {
        FJsonSerializer::ReadFloat(InOutHandle, "InitialSpeed", InitialSpeed, 0);
        FJsonSerializer::ReadFloat(InOutHandle, "MaxSpeed", MaxSpeed, 0);
        FJsonSerializer::ReadFloat(InOutHandle, "GravityScale", GravityScale, 0);

        FString bRotationFollowsVelocityString;
        FJsonSerializer::ReadString(InOutHandle, "bRotationFollowsVelocity", bRotationFollowsVelocityString, "false");
        bRotationFollowsVelocity = bRotationFollowsVelocityString == "true" ? true : false;
    }
    else
    {
        InOutHandle["InitialSpeed"] = InitialSpeed;
        InOutHandle["MaxSpeed"] = MaxSpeed;
        InOutHandle["GravityScale"] = GravityScale;
        InOutHandle["bRotationFollowsVelocity"] = bRotationFollowsVelocity ? "true" : "false";
    }
}

UObject* UProjectileMovementComponent::Duplicate()
{
    UProjectileMovementComponent* ProjectileMovementComponent = Cast<UProjectileMovementComponent>(Super::Duplicate());
    ProjectileMovementComponent->InitialSpeed = InitialSpeed;
    ProjectileMovementComponent->MaxSpeed = MaxSpeed;
    ProjectileMovementComponent->GravityScale = GravityScale;
    ProjectileMovementComponent->bRotationFollowsVelocity = bRotationFollowsVelocity;
    return ProjectileMovementComponent;
}

UClass* UProjectileMovementComponent::GetSpecificWidgetClass() const
{
    return UProjectileMovementComponentWidget::StaticClass();
}
