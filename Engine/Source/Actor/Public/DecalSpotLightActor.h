#pragma once
#include "Actor/Public/Actor.h"

UCLASS()
class ADecalSpotLightActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ADecalSpotLightActor, AActor)
	
public:
	ADecalSpotLightActor();

	virtual UClass* GetDefaultRootComponent() override;
	virtual void InitializeComponents() override;
};

