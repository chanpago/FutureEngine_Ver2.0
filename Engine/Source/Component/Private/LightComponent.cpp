#include "pch.h"

#include "Component/Public/LightComponent.h"
#include "Utility/Public/JsonSerializer.h"

#include "Component/Public/BillBoardComponent.h"
#include "Actor/Public/Actor.h"

#include <algorithm>

IMPLEMENT_ABSTRACT_CLASS(ULightComponent, ULightComponentBase)

void ULightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // Load shadow properties
        FJsonSerializer::ReadFloat(InOutHandle, "ShadowResolutionScale", ShadowResolutionScale, 1.0f);
        FJsonSerializer::ReadFloat(InOutHandle, "ShadowBias", ShadowBias, 0.001f);
        FJsonSerializer::ReadFloat(InOutHandle, "ShadowSlopeBias", ShadowSlopeBias, 1.0f);
        FJsonSerializer::ReadFloat(InOutHandle, "ShadowSharpen", ShadowSharpen, 0.0f);
    }
    else
    {
        // Save shadow properties
        InOutHandle["ShadowResolutionScale"] = ShadowResolutionScale;
        InOutHandle["ShadowBias"] = ShadowBias;
        InOutHandle["ShadowSlopeBias"] = ShadowSlopeBias;
        InOutHandle["ShadowSharpen"] = ShadowSharpen;
    }
}

UObject* ULightComponent::Duplicate()
{
	ULightComponent* LightComponent = Cast<ULightComponent>(Super::Duplicate());

	// Copy shadow properties
	LightComponent->ShadowResolutionScale = ShadowResolutionScale;
	LightComponent->ShadowBias = ShadowBias;
	LightComponent->ShadowSlopeBias = ShadowSlopeBias;
	LightComponent->ShadowSharpen = ShadowSharpen;

	return LightComponent;
}

void ULightComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
}


void ULightComponent::SetIntensity(float InIntensity)
{
    Super::SetIntensity(InIntensity);
    UpdateVisualizationBillboardTint();
}

void ULightComponent::SetLightColor(FVector InLightColor)
{
    Super::SetLightColor(InLightColor);
    UpdateVisualizationBillboardTint();
}

void ULightComponent::UpdateVisualizationBillboardTint()
{
    if (!VisualizationBillboard)
    {
        return;
    }

    FVector ClampedColor = GetLightColor();
    ClampedColor.X = std::clamp(ClampedColor.X, 0.0f, 1.0f);
    ClampedColor.Y = std::clamp(ClampedColor.Y, 0.0f, 1.0f);
    ClampedColor.Z = std::clamp(ClampedColor.Z, 0.0f, 1.0f);

    float NormalizedIntensity = std::clamp(GetIntensity(), 0.0f, 20.0f) / 20.0f;
    FVector4 Tint(ClampedColor.X, ClampedColor.Y, ClampedColor.Z, 1.0f);
    VisualizationBillboard->SetSpriteTint(Tint);
}

void ULightComponent::RefreshVisualizationBillboardBinding()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    UBillBoardComponent* BoundBillboard = VisualizationBillboard;
    const bool bNeedsLookup = !BoundBillboard || !BoundBillboard->IsVisualizationComponent() || BoundBillboard->GetAttachParent() != this;

    if (bNeedsLookup)
    {
        BoundBillboard = nullptr;
        for (UActorComponent* Component : OwnerActor->GetOwnedComponents())
        {
            if (UBillBoardComponent* Candidate = Cast<UBillBoardComponent>(Component))
            {
                if (!Candidate->IsVisualizationComponent())
                {
                    continue;
                }

                if (Candidate->GetAttachParent() != this)
                {
                    continue;
                }

                BoundBillboard = Candidate;
                break;
            }
        }

        VisualizationBillboard = BoundBillboard;
    }

    if (VisualizationBillboard)
    {
        if (VisualizationBillboard->GetAttachParent() != this)
        {
            VisualizationBillboard->AttachToComponent(this);
        }

        UpdateVisualizationBillboardTint();
    }
}
