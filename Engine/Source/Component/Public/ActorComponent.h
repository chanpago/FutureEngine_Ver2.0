#pragma once
#include "Core/Public/Object.h"

class AActor;
class UWidget;

UCLASS()
class UActorComponent : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UActorComponent, UObject)

public:
	UActorComponent();
	~UActorComponent() override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	/*virtual void Render(const URenderer& Renderer) const
	{

	}*/

	virtual void BeginPlay();
	virtual void TickComponent(float DeltaTime);
	virtual void EndPlay();

	virtual void OnSelected();
	virtual void OnDeselected();

	/**
	 * @brief 특정 컴포넌트 전용 Widget이 필요할 경우 재정의 필요
	 */
	virtual UClass* GetSpecificWidgetClass() const { return nullptr; }

	void SetOwner(AActor* InOwner) { Owner = InOwner; }
	AActor* GetOwner() const { return Owner; }

	bool CanEverTick() const { return bCanEverTick; }
	void SetCanEverTick(bool InbCanEverTick) { bCanEverTick = InbCanEverTick; }

protected:
	bool bCanEverTick = false;

private:
	AActor* Owner;
	
public:
	virtual UObject* Duplicate() override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

// Editor Debug Flag
public:
	bool IsEditorOnly() const { return bIsEditorOnly; }
	bool IsVisualizationComponent() const { return bIsVisualizationComponent; }
	void SetIsEditorOnly(bool bInIsEditorOnly);
	void SetIsVisualizationComponent(bool bIsInVisualizationComponent);
	
private:
	/**
	 * @brief 이 컴포넌트가 에디터 전용인지 여부를 나타냄
	 * true일 경우, 게임을 빌드할 때 이 컴포넌트는 완전히 제거
	 * PIE 월드 복제 시에도 컴포넌트 복제 X
	 */
	bool bIsEditorOnly = false;
	/**
	 * @brief 이 컴포넌트가 시각화용 도우미인지 여부를 나타냄
	 * true일 경우, 에디터의 컴포넌트 계층 구조에 표시 X
	 * true일 경우, 추가로 bIsEditorOnly도 true, PIE 월드에 복제 X
	 */
	bool bIsVisualizationComponent = false;
};
