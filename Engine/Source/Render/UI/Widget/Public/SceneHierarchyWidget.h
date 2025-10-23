#pragma once
#include "Widget.h"

class AActor;
class ULevel;
class UCamera;
class UTexture;

/**
 * @brief 현재 Level의 모든 Actor들을 트리 형태로 표시하는 Widget
 * Actor를 클릭하면 Level에서 선택되도록 하는 기능 포함
 */
class USceneHierarchyWidget : public UWidget
{	
	DECLARE_CLASS(USceneHierarchyWidget, UWidget)
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Special Member Function
	USceneHierarchyWidget();
	~USceneHierarchyWidget() override;

private:
	// UI 상태
	bool bShowDetails = true;

	// 검색 기능
	char SearchBuffer[256] = "";
	FString SearchFilter;
	TArray<int32> FilteredIndices; // 필터링된 Actor 인덱스 캐시
	bool bNeedsFilterUpdate = true; // 필터 업데이트 필요 여부

	// 이름 변경 기능
	AActor* RenamingActor = nullptr;
	char RenameBuffer[256] = "";
	double LastClickTime = 0.0f;
	AActor* LastClickedActor = nullptr;
	static constexpr float RENAME_CLICK_DELAY = 0.5f; // 두 번째 클릭 간격

	// 아이콘 시스템
	TMap<FString, UTexture*> IconTextureMap; // 클래스 이름 -> 아이콘 텍스처 매핑
	void LoadActorIcons();
	UTexture* GetIconForActor(AActor* InActor);

	// Camera focus animation
	bool bIsCameraAnimating = false;
	float CameraAnimationTime = 0.0f;
	TArray<FVector> CameraStartLocation;
	TArray<FVector> CameraStartRotation;
	TArray<FVector> CameraTargetLocation;
	TArray<FVector> CameraTargetRotation;

	// Heuristic constant
	static constexpr float CAMERA_ANIMATION_DURATION = 0.8f;
	static constexpr float FOCUS_DISTANCE = 10.0f;

	// Camera movement
	void RenderActorInfo(AActor* InActor, int32 InIndex);
	void SelectActor(AActor* InActor, bool bInFocusCamera = false);
	void FocusOnActor(AActor* InActor);
	void UpdateCameraAnimation();

	// 검색 기능
	void RenderSearchBar();
	void UpdateFilteredActors(const TArray<AActor*>& InLevelActors);
	static bool IsActorMatchingSearch(const FString& InActorName, const FString& InSearchTerm);

	// 이름 변경 기능
	void StartRenaming(AActor* InActor);
	void FinishRenaming(bool bInConfirm);
	bool IsRenaming() const { return RenamingActor != nullptr; }
};
