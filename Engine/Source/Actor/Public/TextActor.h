#pragma once

#include "Actor/Public/Actor.h"
#include "Component/Public/TextComponent.h"

UCLASS()
class ATextActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ATextActor, AActor)

public:
	ATextActor();
	
	virtual UClass* GetDefaultRootComponent() override;
private:
	UTextComponent* TextComponent = nullptr;
};
