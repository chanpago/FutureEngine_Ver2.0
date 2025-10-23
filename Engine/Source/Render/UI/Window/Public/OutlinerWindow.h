#pragma once
#include "UIWindow.h"

/**
 * @brief Outliner 역할을 제공할 Window
 * Actor Transform UI를 제공한다
 */
class UOutlinerWindow : public UUIWindow
{
	DECLARE_CLASS(UOutlinerWindow, UUIWindow);
public:
	UOutlinerWindow();
	virtual ~UOutlinerWindow() override = default;

	void Initialize() override;
};
