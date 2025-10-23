#pragma once
#include "Core/Public/Object.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/public/Axis.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/BatchLines.h"
#include "editor/Public/Camera.h"

class UPrimitiveComponent;
class UUUIDTextComponent;
class FViewportClient;
class UCamera;
class ULevel;
class USplitterWidget;
struct FRay;

enum class EViewportLayoutState
{
	Multi,
	Single,
	Animating,
};

class UEditor : public UObject
{
	DECLARE_CLASS(UEditor, UObject)
public:
	UEditor();
	~UEditor();

	void Update();
	void RenderEditor();
	void RenderGizmo(UCamera* InCamera);

	void SetViewMode(EViewModeIndex InNewViewMode) { CurrentViewMode = InNewViewMode; }
	EViewModeIndex GetViewMode() const { return CurrentViewMode; }

	// 레이아웃 제어는 ViewportManager가 담당

	void SelectActor(AActor* InActor);
	AActor* GetSelectedActor() const { return SelectedActor; }
	void SelectComponent(UActorComponent* InComponent);
	UActorComponent* GetSelectedComponent() const { return SelectedComponent; }

// Getter
public:
	UBatchLines* GetBatchLines() { return &BatchLines; }
	UAxis* GetAxis() { return &Axis; }
	UGizmo* GetGizmo() { return &Gizmo; }

	
private:
	void UpdateBatchLines();
	void ProcessMouseInput();
	
	// 모든 기즈모 드래그 함수가 ActiveCamera를 받도록 통일
	FVector GetGizmoDragLocation(UCamera* InActiveCamera, FRay& WorldRay);
	FVector GetGizmoDragRotation(UCamera* InActiveCamera, FRay& WorldRay);
	FVector GetGizmoDragScale(UCamera* InActiveCamera, FRay& WorldRay);

	inline float Lerp(const float A, const float B, const float Alpha)
	{
		return A * (1 - Alpha) + B * Alpha;
	}

	UObjectPicker ObjectPicker;
	AActor* SelectedActor = nullptr; // 선택된 액터
	UActorComponent* SelectedComponent = nullptr; // 선택된 컴포넌트

	UCamera* Camera;
	UGizmo Gizmo;
	UAxis Axis;
	UBatchLines BatchLines;
	
	// InteractionViewport 제거: ViewportManager가 레이아웃 관리

	EViewModeIndex CurrentViewMode = EViewModeIndex::VMI_BlinnPhong;

	int32 ActiveViewportIndex = 0;

	const float MinScale = 0.01f;
};
