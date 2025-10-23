#include "pch.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Render/UI/Widget/Public/ConsoleWidget.h"
#include "Manager/Time/Public/TimeManager.h"
#include <algorithm>

IMPLEMENT_SINGLETON_CLASS(UConsoleWindow, UUIWindow)

UConsoleWindow::~UConsoleWindow()
{
	if (ConsoleWidget)
	{
		ConsoleWidget->CleanupSystemRedirect();
		ConsoleWidget->ClearLog();
	}
}

UConsoleWindow::UConsoleWindow()
	: ConsoleWidget(nullptr)
	, AnimationState(EConsoleAnimationState::Hidden)
	, AnimationProgress(0.0f)
	, AnimationDuration(0.25f)
	, BottomMargin(10.0f)
{
	// 콘솔 윈도우 기본 설정
	FUIWindowConfig Config;
	Config.WindowTitle = "Engine Console";
	Config.DefaultSize = ImVec2(1000, 260);
	Config.DefaultPosition = ImVec2(10, 770);
	Config.MinSize = ImVec2(1000, 260);
	Config.InitialState = EUIWindowState::Hidden;
	Config.bResizable = true;
	Config.bMovable = false;
	Config.bCollapsible = true;
	Config.DockDirection = EUIDockDirection::BottomLeft; // 왼쪽 하단 도킹
	Config.UpdateWindowFlags();
	SetConfig(Config);
	SetWindowState(EUIWindowState::Hidden);

	ConsoleWidget = &UConsoleWidget::GetInstance();
	AddWidget(ConsoleWidget);
}

void UConsoleWindow::Initialize()
{
	// ConsoleWidget이 유효한지 확인
	if (!ConsoleWidget)
	{
		// 싱글톤 인스턴스를 다시 가져오기 시도
		try
		{
			ConsoleWidget = &UConsoleWidget::GetInstance();
		}
		catch (...)
		{
			// 싱글톤 인스턴스 가져오기 실패 시 에러 발생
			return;
		}
	}

	// Initialize System Output Redirection
	try
	{
		ConsoleWidget->InitializeSystemRedirect();
		AddLog(ELogType::Success, "ConsoleWindow: Game Console 초기화 성공");
		AddLog(ELogType::System, "ConsoleWindow: Logging System Ready");
	}
	catch (const std::exception& Exception)
	{
		// 초기화 실패 시 기본 로그만 출력 (예외를 다시 던지지 않음)
		if (ConsoleWidget)
		{
			ConsoleWidget->AddLog(ELogType::Error, "ConsoleWindow: System Redirection Failed: %s", Exception.what());
		}
	}
	catch (...)
	{
		if (ConsoleWidget)
		{
			ConsoleWidget->AddLog(ELogType::Error, "ConsoleWindow: System Redirection Failed: Unknown Error");
		}
	}
}

void UConsoleWindow::ToggleConsoleVisibility()
{
	switch (AnimationState)
	{
	case EConsoleAnimationState::Hidden:
	case EConsoleAnimationState::Hiding:
		StartShowAnimation();
		break;
	case EConsoleAnimationState::Showing:
	case EConsoleAnimationState::Visible:
		StartHideAnimation();
		break;
	}
}

bool UConsoleWindow::IsConsoleVisible() const
{
	return AnimationState == EConsoleAnimationState::Visible || AnimationState == EConsoleAnimationState::Showing;
}

void UConsoleWindow::OnPreRenderWindow(float MenuBarOffset)
{
	const float DeltaTime = UTimeManager::GetInstance().GetDeltaTime();
	UpdateAnimation(DeltaTime);

	if (AnimationState == EConsoleAnimationState::Hidden)
	{
		return;
	}

	ApplyAnimatedLayout(MenuBarOffset);
}

void UConsoleWindow::StartShowAnimation()
{
	AnimationState = EConsoleAnimationState::Showing;
	SetWindowState(EUIWindowState::Visible);

	if (AnimationProgress <= 0.0f)
	{
		AnimationProgress = 0.0f;
	}
}

void UConsoleWindow::StartHideAnimation()
{
	if (AnimationState == EConsoleAnimationState::Hidden)
	{
		return;
	}

	AnimationState = EConsoleAnimationState::Hiding;
}

void UConsoleWindow::UpdateAnimation(float DeltaTime)
{
	if (AnimationState == EConsoleAnimationState::Showing)
	{
		float ProgressDelta = AnimationDuration > 0.0f ? DeltaTime / AnimationDuration : 1.0f;
		AnimationProgress = std::min(AnimationProgress + ProgressDelta, 1.0f);

		if (AnimationProgress >= 1.0f)
		{
			AnimationProgress = 1.0f;
			AnimationState = EConsoleAnimationState::Visible;
		}
	}
	else if (AnimationState == EConsoleAnimationState::Hiding)
	{
		float ProgressDelta = AnimationDuration > 0.0f ? DeltaTime / AnimationDuration : 1.0f;
		AnimationProgress = std::max(AnimationProgress - ProgressDelta, 0.0f);

		if (AnimationProgress <= 0.0f)
		{
			AnimationProgress = 0.0f;
			AnimationState = EConsoleAnimationState::Hidden;
			SetWindowState(EUIWindowState::Hidden);
		}
	}
	else if (AnimationState == EConsoleAnimationState::Visible)
	{
		AnimationProgress = 1.0f;
	}
	else
	{
		AnimationProgress = 0.0f;
	}
}

void UConsoleWindow::ApplyAnimatedLayout(float MenuBarOffset)
{
	const float ClampedProgress = std::clamp(AnimationProgress, 0.0f, 1.0f);
	float AnimatedHeight = Config.DefaultSize.y * ClampedProgress;
	AnimatedHeight = std::max(AnimatedHeight, 1.0f);

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	const ImVec2 WorkPos = Viewport ? Viewport->WorkPos : ImVec2(0.0f, 0.0f);
	const ImVec2 WorkSize = Viewport ? Viewport->WorkSize : ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

	const float BottomY = WorkPos.y + WorkSize.y - BottomMargin;
	float TopY = BottomY - AnimatedHeight;
	TopY = std::max(TopY, MenuBarOffset);

	ImVec2 TargetPos(WorkPos.x + BottomMargin, TopY);
	ImGui::SetNextWindowPos(TargetPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(Config.DefaultSize.x, AnimatedHeight), ImGuiCond_Always);
}

/**
 * @brief 외부에서 ConsoleWidget에 접근할 수 있도록 하는 래핑 함수
 */
void UConsoleWindow::AddLog(const char* fmt, ...) const
{
	if (ConsoleWidget)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		(void)vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		ConsoleWidget->AddLog(buf);
	}
}

void UConsoleWindow::AddLog(ELogType InType, const char* fmt, ...) const
{
	if (ConsoleWidget)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		(void)vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		ConsoleWidget->AddLog(InType, buf);
	}
}

void UConsoleWindow::AddSystemLog(const char* InText, bool bInIsError) const
{
	if (ConsoleWidget)
	{
		ConsoleWidget->AddSystemLog(InText, bInIsError);
	}
}
