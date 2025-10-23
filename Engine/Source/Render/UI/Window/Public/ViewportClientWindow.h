#pragma once
#include "Render/UI/Window/Public/UIWindow.h"

/**
 * @brief 다중 뷰포트의 속성을 제어할 수 있는 UI를 담당하는 Window
 * FutureEngine 철학: ViewportControlWidget은 LevelTabBarWindow에서 관리
 */
class UViewportClientWindow : public UUIWindow
{
	DECLARE_CLASS(UViewportClientWindow, UUIWindow)
public:
	UViewportClientWindow();
	virtual ~UViewportClientWindow() override = default;

	void Initialize() override;

private:
	void SetupConfig();
};

