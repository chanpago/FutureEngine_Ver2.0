#pragma once
#include "MovementComponent.h"

class UProjectileMovementComponent : public UMovementComponent
{
    DECLARE_CLASS(UProjectileMovementComponent, UMovementComponent)

public:
    UProjectileMovementComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime) override;

    float GetInitialSpeed() const { return InitialSpeed; }
    void SetInitialSpeed(float Speed) { InitialSpeed = Speed; }
    float GetMaxSpeed() const { return MaxSpeed; }
    void SetMaxSpeed(float Speed) { MaxSpeed = Speed; }
    float GetGravityScale() const { return GravityScale; }
    void SetGravityScale(float Scale) { GravityScale = Scale; }
    bool GetRotationFollowsVelocity() const { return bRotationFollowsVelocity; }
    void SetRotationFollowsVelocity(bool InRotationFollowsVelocity) { bRotationFollowsVelocity = InRotationFollowsVelocity; }
    
protected:
    float InitialSpeed = 0;
    // 0 Is No Limit
    float MaxSpeed = 0;
    float GravityScale = 0;
    bool bRotationFollowsVelocity = false;

public:
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    UObject* Duplicate() override;
    UClass* GetSpecificWidgetClass() const override;
};
