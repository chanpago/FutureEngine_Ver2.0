#include "pch.h"
#include "Component/Public/RotatingMovementComponent.h"
#include "Render/UI/Widget/Public/RotatingMovementComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(URotatingMovementComponent, UMovementComponent)

void URotatingMovementComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    if (!UpdatedComponent) { return; }

    // Compute new rotation
    const FQuaternion OldRotation = UpdatedComponent->GetWorldRotationAsQuaternion();
    const FQuaternion DeltaRotation = FQuaternion::FromEuler(RotationRate * DeltaTime);
    const FQuaternion NewRotation = bRotationInLocalSpace ? (OldRotation * DeltaRotation) : (DeltaRotation * OldRotation);

    // Compute new location
    FVector DeltaLocation = FVector::ZeroVector();
    if (!PivotTranslation.IsZero())
    {
        const FVector OldPivot = OldRotation.RotateVector(PivotTranslation);
        const FVector NewPivot = NewRotation.RotateVector(PivotTranslation);
        DeltaLocation = (OldPivot - NewPivot);
    }

    MoveUpdatedComponent(DeltaLocation, NewRotation);
}

void URotatingMovementComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FJsonSerializer::ReadVector(InOutHandle, "RotationRate", RotationRate, FVector::ZeroVector());
		FJsonSerializer::ReadVector(InOutHandle, "PivotTranslation", PivotTranslation, FVector::ZeroVector());

		FString bRotationInLocalSpaceString;
		FJsonSerializer::ReadString(InOutHandle, "bRotationInLocalSpace", bRotationInLocalSpaceString, "false");
		bRotationInLocalSpace = bRotationInLocalSpaceString == "true" ? true : false;
	}
	else
	{
		InOutHandle["RotationRate"] = FJsonSerializer::VectorToJson(RotationRate);
		InOutHandle["PivotTranslation"] = FJsonSerializer::VectorToJson(PivotTranslation);
		InOutHandle["bRotationInLocalSpace"] = bRotationInLocalSpace ? "true" : "false";
	}
}

UObject* URotatingMovementComponent::Duplicate()
{
	URotatingMovementComponent* RotatingMovementComponent = Cast<URotatingMovementComponent>(Super::Duplicate());
	RotatingMovementComponent->bRotationInLocalSpace = bRotationInLocalSpace;
	RotatingMovementComponent->RotationRate = RotationRate;
	RotatingMovementComponent->PivotTranslation = PivotTranslation;

	return RotatingMovementComponent;
}

UClass* URotatingMovementComponent::GetSpecificWidgetClass() const
{
    return URotatingMovementComponentWidget::StaticClass();
}
