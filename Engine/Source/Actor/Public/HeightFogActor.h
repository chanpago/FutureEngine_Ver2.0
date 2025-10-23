#pragma once
#include "Actor/Public/Actor.h"

UCLASS()
class AHeightFogActor : public AActor
{
    GENERATED_BODY()
    DECLARE_CLASS(AHeightFogActor, AActor)
	
public:
    AHeightFogActor();

    virtual UClass* GetDefaultRootComponent() override;
    virtual void InitializeComponents() override;
};

