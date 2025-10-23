#include "pch.h"
#include "Actor/Public/HeightFogActor.h"
#include "Component/Public/BillBoardComponent.h"
#include "Component/Public/HeightFogComponent.h"
#include "Manager/Asset/Public/AssetManager.h"

IMPLEMENT_CLASS(AHeightFogActor, AActor)

AHeightFogActor::AHeightFogActor()
{
}

UClass* AHeightFogActor::GetDefaultRootComponent()
{
    return UHeightFogComponent::StaticClass();
}

void AHeightFogActor::InitializeComponents()
{
    Super::InitializeComponents();
	
    UBillBoardComponent* Billboard = CreateDefaultSubobject<UBillBoardComponent>();
    Billboard->AttachToComponent(GetRootComponent());
    Billboard->SetIsVisualizationComponent(true);
    Billboard->SetSprite(UAssetManager::GetInstance().LoadTexture("Data/Icons/ExponentialHeightFog_64x.png"));
    Billboard->SetScreenSizeScaled(true);
}
