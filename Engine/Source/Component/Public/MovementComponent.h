#pragma once
#include "ActorComponent.h"

class USceneComponent;
class UPrimitiveComponent;

class UMovementComponent : public UActorComponent
{
    DECLARE_CLASS(UMovementComponent, UActorComponent)
    
public:
    UMovementComponent();
    virtual ~UMovementComponent();

    virtual void BeginPlay() override;
    void SetUpdatedComponent(USceneComponent* NewUpdatedComponent);
    void MoveUpdatedComponent(const FVector& Delta, const FQuaternion& NewRotation);
    
protected:
    USceneComponent* UpdatedComponent = nullptr;
    UPrimitiveComponent* UpdatedPrimitive = nullptr;

// Velocity Section
public:
    const FVector& GetVelocity() const { return Velocity; }
    void SetVelocity(const FVector& InVelocity) { Velocity = InVelocity; }
    void StopMovementImmediately();

protected:
    FVector Velocity;

public:
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    UObject* Duplicate() override;
};
