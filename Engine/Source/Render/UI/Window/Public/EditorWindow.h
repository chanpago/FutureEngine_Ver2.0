#pragma once
#include "Render/UI/Window/Public/UIWindow.h"

/**
 * @brief Editor의 전반적인 UI를 담당하는 Window
 * FutureEngine 철학: Splitter는 ViewportManager가 관리하므로 별도의 디버그 위젯 불필요
 */
class UEditorWindow : public UUIWindow
{
	DECLARE_CLASS(UEditorWindow, UUIWindow);
public:
	UEditorWindow();
	virtual ~UEditorWindow() override = default;

	void Initialize() override;
};
