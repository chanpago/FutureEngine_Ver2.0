#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/BillBoardActor.h"

IMPLEMENT_CLASS(ABillBoardActor, AActor)

ABillBoardActor::ABillBoardActor()
{
}

UClass* ABillBoardActor::GetDefaultRootComponent()
{
    return UBillBoardComponent::StaticClass();
}
