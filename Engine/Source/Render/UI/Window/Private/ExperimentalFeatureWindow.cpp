#include "pch.h"
#include "Render/UI/Window/Public/ExperimentalFeatureWindow.h"

#include "Render/UI/Widget/Public/InputInformationWidget.h"
#include "Render/UI/Widget/Public/SceneIOWidget.h"

IMPLEMENT_CLASS(UExperimentalFeatureWindow, UUIWindow)

/**
 * @brief Window Constructor
 */
UExperimentalFeatureWindow::UExperimentalFeatureWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Experimental Feature";
	Config.DefaultSize = ImVec2(350, 230);
	Config.DefaultPosition = ImVec2(370, 33); // 메뉴바만큼 하향 이동
	Config.MinSize = ImVec2(350, 10);
	Config.DockDirection = EUIDockDirection::Right;
	Config.Priority = 5;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(NewObject<UInputInformationWidget>());
	AddWidget(NewObject<USceneIOWidget>());
}

/**
 * @brief Initializer
 */
void UExperimentalFeatureWindow::Initialize()
{
	UE_LOG("ExperimentalFeatureWindow: Window가 성공적으로 생성되었습니다");

	// 실험실은 일단 Hidden으로 초기화
	SetWindowState(EUIWindowState::Hidden);
}
