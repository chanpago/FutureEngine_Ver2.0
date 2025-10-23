#pragma once

#include "Actor/Public/Actor.h"
#include "Component/Public/BillBoardComponent.h"

UCLASS()
class ABillBoardActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ABillBoardActor, AActor)

public:
	ABillBoardActor();

	virtual UClass* GetDefaultRootComponent() override;
private:
	UBillBoardComponent* BillBoardComponent = nullptr;
};
