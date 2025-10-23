#pragma once

#include "Actor/Public/Actor.h"

class USphereComponent;

UCLASS()
class ASphereActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ASphereActor, AActor)

public:
	ASphereActor();

	virtual UClass* GetDefaultRootComponent() override;
	virtual void InitializeComponents() override;
	
private:
	USphereComponent* SphereComponent = nullptr;
};
