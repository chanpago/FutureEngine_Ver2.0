#pragma once
#include "UIWindow.h"

/**
 * @brief 현재 단계에서 필요하지 않은 기능들을 모아둔 Window
 */
class UExperimentalFeatureWindow : public UUIWindow
{
	DECLARE_CLASS(UExperimentalFeatureWindow, UUIWindow)
public:
	UExperimentalFeatureWindow();
	virtual ~UExperimentalFeatureWindow() override {}

	void Initialize() override;
};
