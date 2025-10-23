#include "pch.h"
#include "Actor/Public/MovingCubeActor.h"

IMPLEMENT_CLASS(AMovingCubeActor, ACubeActor)
AMovingCubeActor::AMovingCubeActor()
{
    bCanEverTick = true;
}

void AMovingCubeActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    MovingTimer += DeltaTime;
    if (MovingTimer >= MovingTime)
    {
        MovingTimer = 0.0f;
        Direction *= -1.0f;
    }

    SetActorLocation(GetActorLocation() + Direction * DeltaTime);
}
