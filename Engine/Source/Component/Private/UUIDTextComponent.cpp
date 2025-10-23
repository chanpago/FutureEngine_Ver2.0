#include "pch.h"
#include "Component/Public/UUIDTextComponent.h"
#include "Editor/Public/Editor.h"
#include "Actor/Public/Actor.h"

IMPLEMENT_CLASS(UUUIDTextComponent, UTextComponent)

/**
 * @brief Level에서 각 Actor마다 가지고 있는 UUID를 출력해주기 위한 빌보드 클래스
 * Actor has a UBillBoardComponent
 */

UUUIDTextComponent::UUUIDTextComponent() : ZOffset(5.0f)
{
	SetIsEditorOnly(true);
	SetCanPick(false);

	bReceivesDecals = false;
};

UUUIDTextComponent::~UUUIDTextComponent()
{
}

void UUUIDTextComponent::OnSelected()
{
	Super::OnSelected();
}

void UUUIDTextComponent::OnDeselected()
{
	Super::OnDeselected();
}

void UUUIDTextComponent::UpdateRotationMatrix(const FVector& InCameraForward)
{	
	FVector Forward = InCameraForward;
	FVector Right = FVector::UpVector().Cross(Forward); Right.Normalize();
	FVector Up = Forward.Cross(Right); Up.Normalize();
    
	// Construct the rotation matrix from the basis vectors
	RTMatrix = FMatrix(Forward, Right, Up);
	
	const FVector& OwnerActorLocation = GetOwner()->GetActorLocation();
	const FVector Translation = OwnerActorLocation + FVector(0.0f, 0.0f, ZOffset);
	RTMatrix *= FMatrix::TranslationMatrix(Translation);
}

void UUUIDTextComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UTextComponent::Serialize(bInIsLoading, InOutHandle);
	if (bInIsLoading)
	{
		SetOffset(5);
	}
}

UClass* UUUIDTextComponent::GetSpecificWidgetClass() const
{
	return nullptr;
}
