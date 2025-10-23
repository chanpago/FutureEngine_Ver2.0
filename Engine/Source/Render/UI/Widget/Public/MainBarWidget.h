#pragma once
#include "Widget.h"

class UUIManager;

/**
 * @brief ImGui 메인 메뉴바를 관리하는 위젯 클래스
 * FutureEngine 스타일: 보기, 표시옵션, 창, 도움말 메뉴만 제공
 */
UCLASS()
class UMainBarWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UMainBarWidget, UWidget)

public:
	UMainBarWidget() = default;
	~UMainBarWidget() override = default;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	float GetMenuBarHeight() const { return MenuBarHeight; }
	bool IsMenuBarVisible() const { return bIsMenuBarVisible; }

private:
	bool bIsMenuBarVisible = true;
	float MenuBarHeight = 0.0f;
	UUIManager* UIManager = nullptr;
	bool bShowInfoPopup = false;

	// FutureEngine 스타일: 필수 메뉴만 유지
	void RenderWindowsMenu() const;
	static void RenderFileMenu();
	static void RenderViewMenu();
	static void RenderShowFlagsMenu();
	static void RenderToolsMenu();
	void RenderHelpMenu();
	
	// 파일 메뉴 기능
	static void CreateNewLevel();
	static void LoadLevel();
	static void SaveCurrentLevel();
	static path OpenLoadFileDialog();
	static path OpenSaveFileDialog();

};
