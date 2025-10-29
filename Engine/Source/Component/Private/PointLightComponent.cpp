#include "pch.h"

#include "Component/Public/PointLightComponent.h"
#include "Render/UI/Widget/Public/PointLightComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(UPointLightComponent, ULightComponent)

void UPointLightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);
	if (bInIsLoading)
	{
		FJsonSerializer::ReadFloat(InOutHandle, "DistanceFalloffExponent", DistanceFalloffExponent);
		SetDistanceFalloffExponent(DistanceFalloffExponent); // clamping을 위해 Setter 사용
		FJsonSerializer::ReadFloat(InOutHandle, "AttenuationRadius", AttenuationRadius);
	}
	else
	{
		InOutHandle["DistanceFalloffExponent"] = DistanceFalloffExponent;
		InOutHandle["AttenuationRadius"] = AttenuationRadius;
	}
}

UObject* UPointLightComponent::Duplicate()
{
	UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(Super::Duplicate());
	PointLightComponent->DistanceFalloffExponent = DistanceFalloffExponent;
	PointLightComponent->AttenuationRadius = AttenuationRadius;

	return PointLightComponent;
}

void UPointLightComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}

UClass* UPointLightComponent::GetSpecificWidgetClass() const
{
    return UPointLightComponentWidget::StaticClass();
}

void UPointLightComponent::EnsureVisualizationBillboard()
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
	Billboard->SetSprite(UAssetManager::GetInstance().LoadTexture("Data/Icons/S_LightPoint.png"));
	Billboard->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));
	Billboard->SetScreenSizeScaled(true);

	VisualizationBillboard = Billboard;
	UpdateVisualizationBillboardTint();
}
FPointLightInfo UPointLightComponent::GetPointlightInfo() const
{
	return FPointLightInfo{
		FVector4(LightColor, 1.0f),
		GetWorldLocation(),
		Intensity,
		AttenuationRadius,
		DistanceFalloffExponent,
		ResolutionScale,
		Bias,
		SlopeBias,
		Sharpen };
}
