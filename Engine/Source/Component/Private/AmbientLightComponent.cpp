#include "pch.h"
#include "Component/Public/AmbientLightComponent.h"
#include "Render/UI/Widget/Public/AmbientLightComponentWidget.h"

IMPLEMENT_CLASS(UAmbientLightComponent, ULightComponent)

void UAmbientLightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
}

UObject* UAmbientLightComponent::Duplicate()
{
	UAmbientLightComponent* AmbientLightComponent = Cast<UAmbientLightComponent>(Super::Duplicate());

	return AmbientLightComponent;
}

void  UAmbientLightComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}

UClass* UAmbientLightComponent::GetSpecificWidgetClass() const
{
	return UAmbientLightComponentWidget::StaticClass();
}

void UAmbientLightComponent::EnsureVisualizationBillboard()
{
	if (VisualizationBillboard)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (GWorld)
	{
		EWorldType WorldType = GWorld->GetWorldType();
		if (WorldType != EWorldType::Editor && WorldType != EWorldType::EditorPreview)
		{
			return;
		}
	}

	UBillBoardComponent* Billboard = OwnerActor->AddComponent<UBillBoardComponent>();
	if (!Billboard)
	{
		return;
	}
	Billboard->AttachToComponent(this);
	Billboard->SetIsVisualizationComponent(true);
	Billboard->SetSprite(UAssetManager::GetInstance().LoadTexture("Data/Icons/SkyLight.png"));
	Billboard->SetRelativeScale3D(FVector(2.f,2.f,2.f));
	Billboard->SetScreenSizeScaled(true);

	VisualizationBillboard = Billboard;
	UpdateVisualizationBillboardTint();
}

FAmbientLightInfo UAmbientLightComponent::GetAmbientLightInfo() const
{
	return FAmbientLightInfo{ FVector4(LightColor, 1), Intensity };
}