#include "pch.h"
#include "Render/UI/Window/Public/ControlPanelWindow.h"

#include "Render/UI/Widget/Public/FPSWidget.h"
#include "Render/UI/Widget/Public/ActorSpawnWidget.h"

IMPLEMENT_CLASS(UControlPanelWindow, UUIWindow)
/**
 * @brief Control Panel Constructor
 * 적절한 사이즈의 윈도우 제공
 */
UControlPanelWindow::UControlPanelWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Control Panel";
	Config.DefaultSize = ImVec2(350, 550);
	Config.DefaultPosition = ImVec2(10, 33); // 메뉴바만큼 하향 이동
	Config.MinSize = ImVec2(350, 200);
	Config.DockDirection = EUIDockDirection::Left;
	Config.Priority = 15;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.InitialState = EUIWindowState::Hidden; // 기본적으로 숨김

	Config.UpdateWindowFlags();
	SetConfig(Config);
	
	// FutureEngine 철학: InitialState를 명시적으로 설정한 후 현재 상태도 동기화
	SetWindowState(EUIWindowState::Hidden);

	UWidget* FPSWidget = NewObject<UFPSWidget>();
	FPSWidget->Initialize();
	AddWidget(FPSWidget);
	
	UWidget* ActorSpawnWidget = NewObject<UActorSpawnWidget>();
	ActorSpawnWidget->Initialize();
	AddWidget(ActorSpawnWidget);
	// 현재 ViewportMenuBarWidget.cpp 에서 사용 중
	//AddWidget(new UCameraControlWidget);
}

/**
 * @brief 초기화 함수
 */
void UControlPanelWindow::Initialize()
{
	UE_LOG("ControlPanelWindow: Initialized");
}
