#pragma once
#include "Actor/Public/Actor.h"

class UCubeComponent;

UCLASS()
class ACubeActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ACubeActor, AActor)

public:
	ACubeActor();
	
	virtual UClass* GetDefaultRootComponent() override;
	virtual void InitializeComponents() override;
	
private:
	UCubeComponent* CubeComponent = nullptr;
};
