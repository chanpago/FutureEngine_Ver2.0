#pragma once
#include "Component/Public/ActorComponent.h"

namespace json { class JSON; }
using JSON = json::JSON;

UCLASS()
class USceneComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USceneComponent, UActorComponent)

public:
	USceneComponent();

	void BeginPlay() override;
	    void TickComponent(float DeltaTime) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	
	virtual void MarkAsDirty();

	void SetRelativeLocation(const FVector& Location);
	void SetRelativeRotation(const FQuaternion& Rotation);
	void SetRelativeScale3D(const FVector& Scale);
	void SetUniformScale(bool bIsUniform);

	bool IsUniformScale() const;
	
	const FVector& GetRelativeLocation() const { return RelativeLocation; }
	const FQuaternion& GetRelativeRotation() const { return RelativeRotation; }
	const FVector& GetRelativeScale3D() const { return RelativeScale3D; }

	const FMatrix& GetWorldTransformMatrix() const;
	const FMatrix& GetWorldTransformMatrixInverse() const;

	FVector GetWorldLocation() const;
    FVector GetWorldRotation() const;
    FQuaternion GetWorldRotationAsQuaternion() const;
    FVector GetWorldScale3D() const;

    void SetWorldLocation(const FVector& NewLocation);
    void SetWorldRotation(const FVector& NewRotation);
    void SetWorldRotation(const FQuaternion& NewRotation);
    void SetWorldScale3D(const FVector& NewScale);

private:
	mutable bool bIsTransformDirty = true;
	mutable bool bIsTransformDirtyInverse = true;
	mutable FMatrix WorldTransformMatrix;
	mutable FMatrix WorldTransformMatrixInverse;

	FVector RelativeLocation = FVector{ 0,0,0.f };
	FQuaternion RelativeRotation = FQuaternion::Identity();
	FVector RelativeScale3D = FVector{ 1.f,1.f,1.f };
	bool bIsUniformScale = false;

	// SceneComponent Hierarchy Section
public:
	USceneComponent* GetAttachParent() const { return AttachParent; }
	void AttachToComponent(USceneComponent* Parent, bool bRemainTransform = false);
	void DetachFromComponent();
	bool IsAttachedTo(const USceneComponent* Parent) const { return AttachParent == Parent; }
	const TArray<USceneComponent*>& GetChildren() const { return AttachChildren; }
	
protected:
	void DetachChild(USceneComponent* ChildToDetach);

private:
	USceneComponent* AttachParent = nullptr;
	TArray<USceneComponent*> AttachChildren;
	
public:
	virtual UObject* Duplicate() override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;
};