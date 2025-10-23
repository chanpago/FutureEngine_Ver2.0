#pragma once
#include "Actor/Public/CubeActor.h"

class AMovingCubeActor : public ACubeActor
{
    DECLARE_CLASS(AMovingCubeActor, ACubeActor)
public:
    AMovingCubeActor();
    virtual void Tick(float DeltaTime) override;

private:
    float MovingTime = 3.0f;
    float MovingTimer = 0.0f;
    FVector Direction = FVector(0.0f, 0.0f, 3.0f);
};
