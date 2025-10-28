#pragma once
#include "Editor/Public/EditorPrimitive.h"
#include "Core/Public/Object.h"
#include "Actor/Public/Actor.h"

class UCamera;

enum class EGizmoMode
{
	Translate,
	Rotate,
	Scale
};

enum class EGizmoDirection
{
	Right,
	Up,
	Forward,
	None
};

struct FGizmoTranslationCollisionConfig
{
	FGizmoTranslationCollisionConfig()
		: Radius(0.04f), Height(0.9f), Scale(2.f) {
	}

	float Radius = {};
	float Height = {};
	float Scale = {};
};

struct FGizmoRotateCollisionConfig
{
	FGizmoRotateCollisionConfig()
		: OuterRadius(0.58f), InnerRadius(0.475f), Scale(1.f) {
	}

	float OuterRadius = {0.58f};  // 링 큰 반지름
	float InnerRadius = {0.475f};  // 링 굵기 r
	float Scale = {1.0f};
};

class UGizmo : public UObject
{
public:
	UGizmo();
	~UGizmo() override;
	void UpdateScale(UCamera* InCamera);
	void RenderGizmo(UCamera* InCamera);
	void ChangeGizmoMode();

	/* *
	* @brief Setter
	*/
	void SetLocation(const FVector& Location);
	void SetGizmoDirection(EGizmoDirection Direction) { GizmoDirection = Direction; }
	void SetComponentRotation(const FVector& Rotation) { TargetComponent->SetWorldRotation(Rotation); }
	void SetComponentScale(const FVector& Scale) { TargetComponent->SetWorldScale3D(Scale); }

	void SetWorld() { bIsWorld = true; }
	void SetLocal() { bIsWorld = false; }
	bool IsWorldMode() { return bIsWorld; }

	/* *
	* @brief Getter
	*/
	const float GetTranslateScale() const { return TranslateCollisionConfig.Scale; }
	const float GetRotateScale() const { return RotateCollisionConfig.Scale; }
	const EGizmoDirection GetGizmoDirection() { return GizmoDirection; }
	const FVector& GetGizmoLocation() { return Primitives[(int)GizmoMode].Location; }
	const FVector& GetComponentRotation() { return TargetComponent->GetWorldRotation(); }
	const FVector& GetComponentScale() { return TargetComponent->GetWorldScale3D(); }
	const FVector& GetDragStartMouseLocation() { return DragStartMouseLocation; }
	const FVector& GetDragStartActorLocation() { return DragStartActorLocation; }
	const FVector& GetDragStartActorRotation() { return DragStartActorRotation; }
	const FVector& GetDragStartActorScale() { return DragStartActorScale; }
	const EGizmoMode GetGizmoMode() { return GizmoMode; }
	const FVector GetGizmoAxis() {
		FVector Axis[3]{ {1,0,0},{0,1,0},{0,0,1} }; return Axis[AxisIndex(GizmoDirection)];
	}

	float GetTranslateRadius() const { return TranslateCollisionConfig.Radius * TranslateCollisionConfig.Scale; }
	float GetTranslateHeight() const { return TranslateCollisionConfig.Height * TranslateCollisionConfig.Scale; }
	float GetRotateOuterRadius() const { return RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale; }
	float GetRotateInnerRadius() const { return RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale; }
	float GetRotateThickness()   const { return std::max(0.001f, RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale); }
	USceneComponent* GetTargetComponent() {return TargetComponent;}
	void SetSelectedComponent(USceneComponent* InComponent) { TargetComponent = InComponent; }
	USceneComponent* GetSelectedComponent() const { return TargetComponent; }
	bool IsInRadius(float Radius);
	bool HasComponent() const { return TargetComponent; }

	/* *
	* @brief 마우스 관련
	*/
	void EndDrag() { bIsDragging = false; }
	bool IsDragging() const { return bIsDragging; }
	void OnMouseHovering() {}
	void OnMouseDragStart(FVector& CollisionPoint);
	void OnMouseRelease(EGizmoDirection DirectionReleased) {}

private:
	static inline int AxisIndex(EGizmoDirection InDirection)
	{
		switch (InDirection)
		{
		case EGizmoDirection::Forward:	return 0;
		case EGizmoDirection::Right:	return 1;
		case EGizmoDirection::Up:		return 2;
		default:						return 2;
		}
	}

	// 렌더 시 하이라이트 색상 계산(상태 오염 방지)
	FVector4 ColorFor(EGizmoDirection InAxis) const;

	UEditor* Editor = nullptr;

	TArray<FEditorPrimitive> Primitives;
	USceneComponent* TargetComponent = nullptr;

	TArray<FVector4> GizmoColor;
	FVector DragStartActorLocation;
	FVector DragStartMouseLocation;
	FVector DragStartActorRotation;
	FVector DragStartActorScale;

	FGizmoTranslationCollisionConfig TranslateCollisionConfig;
	FGizmoRotateCollisionConfig RotateCollisionConfig;
	float HoveringFactor = 0.8f;
	const float ScaleFactor = 0.3f;
	const float MinScaleFactor = 7.0f;
	const float OrthoScaleFactor = 7.0f;
	bool bIsDragging = false;
	bool bIsWorld = true;	// Gizmo coordinate mode (true; world)

	FRenderState RenderState;

	EGizmoDirection GizmoDirection = EGizmoDirection::None;
	EGizmoMode      GizmoMode = EGizmoMode::Translate;
};
