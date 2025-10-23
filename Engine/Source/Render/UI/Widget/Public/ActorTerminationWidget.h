#pragma once
#include "Widget.h"
#include "Render/UI/Widget/Public/ActorDetailWidget.h"

/**
 * @brief 해당 Widget은 ImGui 기능을 제공하지 않지만 Termination 유틸을 제공한다
 */
class UActorTerminationWidget : public UWidget
{
	DECLARE_CLASS(UActorTerminationWidget, UWidget)
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void DeleteSelectedActor(AActor* InSelectedActor);
	void DeleteSelectedComponent(AActor* InSelectedActor, UActorComponent* InSelectedComponent);

	UActorTerminationWidget() = default;
	~UActorTerminationWidget() override = default;

	void SetActorDetailWidget(UActorDetailWidget* InActorDetailWidget) { ActorDetailWidget = InActorDetailWidget; }

private:
	UActorDetailWidget* ActorDetailWidget = nullptr;
};
