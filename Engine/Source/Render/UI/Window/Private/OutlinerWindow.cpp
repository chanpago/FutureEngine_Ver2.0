#include "pch.h"
#include "Render/UI/Window/Public/OutlinerWindow.h"

#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"

IMPLEMENT_CLASS(UOutlinerWindow, UUIWindow)
UOutlinerWindow::UOutlinerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Outliner";
	Config.DefaultSize = ImVec2(340, 520);
	Config.DefaultPosition = ImVec2(1565, 91); // 메뉴바 + 레벨바 아래
	Config.MinSize = ImVec2(200, 50);
	Config.bResizable = true;
	Config.bMovable = false; // FutureEngine 철학: 오른쪽 패널 고정
	Config.bCollapsible = false;
	Config.DockDirection = EUIDockDirection::Center;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(NewObject<USceneHierarchyWidget>());
}

void UOutlinerWindow::Initialize()
{
	// Widget 초기화
	for (UWidget* Widget : GetWidgets())
	{
		if (Widget)
		{
			Widget->Initialize();
		}
	}
	UE_LOG("OutlinerWindow: Window가 성공적으로 생성되었습니다");
}
