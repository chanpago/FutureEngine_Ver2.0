#pragma once
#include "UIWindow.h"
#include "Render/UI/Widget/Public/MainBarWidget.h"

class UMainBarWidget;

/**
 * @brief 메인 메뉴바를 관리하는 UI 윈도우 클래스
 * 일반적인 윈도우와 달리 상단에 고정되며 뷰포트의 사이즈에 영향이 있음
 */
UCLASS()
class UMainMenuWindow :	public UUIWindow
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UMainMenuWindow, UUIWindow)

public:
	void Initialize() override;
	void Cleanup() override;

	float GetMenuBarHeight() const { return MainBarWidget ? MainBarWidget->GetMenuBarHeight() : 0.0f; }
	UMainBarWidget* GetMainBarWidget() const { return MainBarWidget; }
	bool IsSingleton() override { return true; }

private:
	UMainBarWidget* MainBarWidget = nullptr;

	void SetupMainMenuConfig();
};
