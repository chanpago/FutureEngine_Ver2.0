#pragma once

class UConsoleWindow;
class UControlPanelWindow;
class UExperimentalFeatureWindow;
class UPerformanceWindow;
class UOutlinerWindow;
class UCameraPanelWindow;
class UDetailWindow;
class UMainMenuWindow;
class UEditorWindow;
class UViewportClientWindow;
class ULevelTabBarWindow;
/**
 * @brief UI 윈도우 도킹 방향
 */
enum class EUIDockDirection : uint8_t
{
	None, // 도킹 없음
	Left, // 왼쪽 도킹
	Right, // 오른쪽 도킹
	Top, // 상단 도킹
	Bottom, // 하단 도킹 (전체 폭)
	BottomLeft, // 왼쪽 하단 도킹
	Center, // 중앙 도킹
};

/**
* @brief UI 윈도우들을 쉽게 생성하기 위한 팩토리 클래스
 */
class UUIWindowFactory
{
public:
	static void CreateDefaultUILayout();
	static UMainMenuWindow& CreateMainMenuWindow();
	static UConsoleWindow* CreateConsoleWindow(EUIDockDirection InDockDirection = EUIDockDirection::BottomLeft);
	static UControlPanelWindow* CreateControlPanelWindow(EUIDockDirection InDockDirection = EUIDockDirection::Left);
	static UOutlinerWindow* CreateOutlinerWindow(EUIDockDirection InDockDirection = EUIDockDirection::Center);
	static ULevelTabBarWindow* CreateLevelTabBarWindow();
	static UDetailWindow* CreateDetailWindow(EUIDockDirection InDockDirection = EUIDockDirection::Right);
	static UExperimentalFeatureWindow*
		CreateExperimentalFeatureWindow(EUIDockDirection InDockDirection = EUIDockDirection::Right);
	static UEditorWindow* CreateEditorWindow(EUIDockDirection InDockDirection = EUIDockDirection::None);
	static UViewportClientWindow* CreateViewportClientWindow(EUIDockDirection InDockDirection = EUIDockDirection::None);
};

