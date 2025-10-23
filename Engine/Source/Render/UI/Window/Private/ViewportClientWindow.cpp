#include "pch.h"
#include "Render/UI/Window/Public/ViewportClientWindow.h"
#include "Render/Renderer/Public/Renderer.h" 
#include "Render/UI/Viewport/Public/ViewportClient.h"

IMPLEMENT_CLASS(UViewportClientWindow, UUIWindow)

UViewportClientWindow::UViewportClientWindow()
{
	SetupConfig();
	
	// FutureEngine 철학: ViewportControlWidget은 LevelTabBarWindow에서만 관리
	// 중복 렌더링 방지를 위해 여기서는 생성하지 않음
	
	UE_LOG("ViewportClientWindow: 윈도우가 초기화되었습니다");
}

void UViewportClientWindow::Initialize()
{
	UE_LOG("ViewportClientWindow: Window가 성공적으로 생성되었습니다.");

	SetWindowState(EUIWindowState::Visible);
}

void UViewportClientWindow::SetupConfig()
{
	const D3D11_VIEWPORT& ViewportInfo = URenderer::GetInstance().GetDeviceResources()->GetViewportInfo();
	FUIWindowConfig Config;
	Config.WindowTitle = "ViewportClient";
	Config.DefaultSize = ImVec2(ViewportInfo.Width, ViewportInfo.Height);
	Config.DefaultPosition = ImVec2(0, 0);
	Config.DockDirection = EUIDockDirection::None;
	Config.Priority = 999;
	Config.bResizable = false;
	Config.bMovable = false;
	Config.bCollapsible = false;

	// 메뉴바만 보이도록 하기 위해 전체적으로 숨김 처리
	Config.WindowFlags = ImGuiWindowFlags_NoTitleBar |
						 ImGuiWindowFlags_NoResize |
						 ImGuiWindowFlags_NoMove |
						 ImGuiWindowFlags_NoCollapse |
						 ImGuiWindowFlags_NoScrollbar |
						 ImGuiWindowFlags_NoBackground |
						 ImGuiWindowFlags_NoBringToFrontOnFocus |
						 ImGuiWindowFlags_NoNavFocus |
						 ImGuiWindowFlags_NoDecoration |
						 ImGuiWindowFlags_NoInputs;

	SetConfig(Config);
}
