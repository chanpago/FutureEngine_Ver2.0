#pragma once
#include "Actor/Public/Actor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"

class UCubeComponent;

UCLASS()
class AStaticMeshActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(AStaticMeshActor, AActor)

public:
	AStaticMeshActor();
	
	virtual UClass* GetDefaultRootComponent() override;

private:
	UStaticMeshComponent* StaticMeshComponent = nullptr;
};
