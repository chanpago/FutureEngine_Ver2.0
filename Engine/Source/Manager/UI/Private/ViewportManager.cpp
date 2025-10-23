#include "pch.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/Window.h"
#include "Core/Public/AppWindow.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Layout/Public/Splitter.h"
#include "Render/UI/Layout/Public/SplitterV.h"
#include "Render/UI/Layout/Public/SplitterH.h"
#include "Render/UI/Window/Public/LevelTabBarWindow.h"
#include "Render/UI/Window/Public/MainMenuWindow.h"
#include "Editor/Public/Editor.h"
#include "Manager/Input/Public/InputManager.h"
#include "Utility/Public/JsonSerializer.h"
#include "Global/Enum.h"
namespace
{
	template <typename T>
	constexpr T Lerp(const T& A, const T& B, const T& Alpha)
	{
		return A + (B - A) * Alpha;
	}

	void SyncEditorViewMode(EViewModeIndex InMode)
	{
		if (UEditor* Editor = GEditor->GetEditorModule())
		{
			Editor->SetViewMode(InMode);
		}
	}
}


IMPLEMENT_SINGLETON_CLASS(UViewportManager,UObject)

UViewportManager::UViewportManager() = default;
UViewportManager::~UViewportManager()
{
	Release();
}

void UViewportManager::Initialize(FAppWindow* InWindow)
{
	Release();
	// 밖에서 윈도우를 가져와 크기를 가져온다
	int32 Width = 0, Height = 0;
	if (InWindow)
	{
		InWindow->GetClientSize(Width, Height);
	}
	AppWindow = InWindow;

	// 루트 윈도우에 새로운 윈도우를 할당합니다.
	ViewportLayout = EViewportLayout::Single;

	// 0번 인덱스의 뷰포트로 초기화
	ActiveIndex = 0;

	float MenuAndLevelBarHeight = ULevelTabBarWindow::GetInstance().GetLevelBarHeight() + UMainMenuWindow::GetInstance().GetMenuBarHeight();

	// 뷰포트 슬롯 최대치까진 준비(포인터만). 아직 RT/DSV 없음.
	// 4개의 뷰포트, 클라이언트, 카메라 를 할당받습니다.
	InitializeViewportAndClient();


	// 부모 스플리터
	SplitterV = new SSplitterV();
	SplitterV->Ratio = IniSaveSharedV;

	// 자식 스플리터
	LeftSplitterH = new SSplitterH();
	RightSplitterH = new SSplitterH();

	// 두 수평 스플리터가 동일한 비율 값을 공유하도록 설정합니다.
	LeftSplitterH->SetSharedRatio(&SplitterValueH);
	RightSplitterH->SetSharedRatio(&SplitterValueH);

	// 부모 스플리터의 자식 스플리터를 셋합니다.
	SplitterV->SetChildren(LeftSplitterH, RightSplitterH);

	// 자식 스플리터에 붙을 윈도우를 셋합니다.
	LeftTopWindow		= new SWindow();
	LeftBottomWindow	= new SWindow();
	RightTopWindow		= new SWindow();
	RightBottomWindow	= new SWindow();

	// 자식 스플리터에 자식 윈도우를 셋합니다.
	LeftSplitterH->SetChildren(LeftTopWindow, LeftBottomWindow);
	RightSplitterH->SetChildren(RightTopWindow, RightBottomWindow);

	// 리프 배열로 고정 매핑
	Leaves[0] = LeftTopWindow;
	Leaves[1] = LeftBottomWindow;
	Leaves[2] = RightTopWindow;
	Leaves[3] = RightBottomWindow;

	QuadRoot = SplitterV;

	// 싱글: Root를 선택된 리프로 바꾸고 그 리프만 크게
	Root = Leaves[ActiveIndex];
	Root->OnResize(ActiveViewportRect);

	SplitterValueV = IniSaveSharedV;
	SplitterValueH = IniSaveSharedH;
}

void UViewportManager::Update()
{
	if (!Root)
	{
		UE_LOG_WARNING("ViewportManager::Update - Root가 null입니다!");
		return;
	}
	
	// 초기화 상태 확인
	if (Viewports.empty() || Clients.empty())
	{
		UE_LOG_ERROR("ViewportManager::Update - Viewports(%zu) 또는 Clients(%zu)가 비어있습니다!", Viewports.size(), Clients.size());
		return;
	}

	int32 Width = 0, Height = 0;

	// AppWindow는 실제 윈도우의 메세지콜이나 크기를 담당합니다
	if (AppWindow)
	{
		AppWindow->GetClientSize(Width, Height);
	}

	// 91px height
	const int MenuAndLevelHeight = UMainMenuWindow::GetInstance().GetMenuBarHeight() + ULevelTabBarWindow::GetInstance().GetLevelBarHeight() - 12;

	// ActiveViewportRect는 실제로 렌더링이 될 영역의 뷰포트 입니다
	const int32 RightPanelWidth = static_cast<int32>(UUIManager::GetInstance().GetRightPanelWidth());
	const int32 ViewportWidth = Width - RightPanelWidth;
	ActiveViewportRect = FRect{ 0, MenuAndLevelHeight, max(0, ViewportWidth), max(0, Height - MenuAndLevelHeight) };


	if (ViewportLayout == EViewportLayout::Quad)
	{
		if (QuadRoot)
		{
			QuadRoot->OnResize(ActiveViewportRect);
		}
		// SWindow 레이아웃의 최종 Rect를 FViewport에 동기화합니다.
		// 이 작업을 통해 3D 렌더링이 올바른 화면 영역에 그려집니다.
		for (int32 i = 0; i < 4; ++i)
		{
			if (Leaves[i] && Viewports[i] && Clients[i])
			{
				Viewports[i]->SetRect(Leaves[i]->GetRect());

				// 각 카메라 종횡비 조정
				const float Aspect = Viewports[i]->GetAspect();
				// FutureEngine: Camera null 체크
				if (UCamera* Cam = Clients[i]->GetCamera())
				{
					Cam->SetAspect(Aspect);
				}
			}
		}


		int32 ViewportIndexUnderMouse = GetViewportIndexUnderMouse();
		if (ViewportIndexUnderMouse != -1 && ActiveIndex!= ViewportIndexUnderMouse && !UInputManager::GetInstance().IsKeyDown(EKeyInput::MouseRight))
		{
			ActiveIndex = ViewportIndexUnderMouse;
		}
	}
	else 
	{
		SetRoot(Leaves[ActiveIndex]);

		if (Root)
		{
			Root->OnResize(ActiveViewportRect);
		}

		// FutureEngine: 범위 체크
		if (ActiveIndex < Viewports.size() && ActiveIndex < 4)
		{
			if (Viewports[ActiveIndex] && Leaves[ActiveIndex])
			{
				Viewports[ActiveIndex]->SetRect(Leaves[ActiveIndex]->GetRect());
			}
		}
	}

	// 스플리터 드래그 처리 함수
	TickInput();


	// 애니메이션 처리함수
	UpdateViewportAnimation();
}


void UViewportManager::RenderOverlay()
{
	// FutureEngine 철학: Quad 모드나 애니메이션 중에만 스플리터 렌더링
	if (!(ViewportLayout == EViewportLayout::Quad || ViewportAnimation.bIsAnimating))
		return;

	if (QuadRoot)
		QuadRoot->OnPaint();
}

void UViewportManager::Release()
{
	for (size_t Index = 0; Index < Viewports.size(); ++Index)
	{
		FViewport*& Viewport = Viewports[Index];
		if (!Viewport) { continue; }

		if (Index < Clients.size())
		{
			FViewportClient*& ClientRef = Clients[Index];
			if (ClientRef)
			{
				ClientRef->SetOwningViewport(nullptr);
			}
			Viewport->SetViewportClient(nullptr);
		}

		SafeDelete(Viewport);
	}
	Viewports.clear();

	for (FViewportClient*& Client : Clients)
	{
		SafeDelete(Client);
	}
	Clients.clear();

	for (UCamera*& Camera : OrthoGraphicCameras)
	{
		SafeDelete(Camera);
	}
	OrthoGraphicCameras.clear();

	for (UCamera*& Camera : PerspectiveCameras)
	{
		SafeDelete(Camera);
	}
	PerspectiveCameras.clear();

	InitialOffsets.clear();
	ActiveRmbViewportIdx = -1;
	ActiveIndex = 0;

	SafeDelete(LeftTopWindow);
	SafeDelete(LeftBottomWindow);
	SafeDelete(RightTopWindow);
	SafeDelete(RightBottomWindow);
	LeftTopWindow = LeftBottomWindow = RightTopWindow = RightBottomWindow = nullptr;

	for (auto& Leaf : Leaves)
	{
		Leaf = nullptr;
	}

	SafeDelete(LeftSplitterH);
	SafeDelete(RightSplitterH);
	SafeDelete(SplitterV);
	LeftSplitterH = RightSplitterH = SplitterV = nullptr;

	Root = nullptr;
	QuadRoot = nullptr;
	Capture = nullptr;

	ActiveViewportRect = {};
	ViewportLayout = EViewportLayout::Single;
	SplitterValueV = 0.5f;
	SplitterValueH = 0.5f;
	IniSaveSharedV = 0.5f;
	IniSaveSharedH = 0.5f;
	SharedFovY = 150.0f;
	SharedY = 0.5f;
	LastPromotedIdx = -1;
	ViewLayoutChangeSplitterH = 0.5f;
	ViewLayoutChangeSplitterV = 0.5f;
	ViewportAnimation = {};
	AppWindow = nullptr;
}

void UViewportManager::TickInput()
{
	if (!(ViewportLayout == EViewportLayout::Quad || ViewportAnimation.bIsAnimating))
	{
		return;
	}
	if (!QuadRoot)
	{
		return;
	}

	auto& InputManager = UInputManager::GetInstance();
	const FVector& MousePosition = InputManager.GetMousePosition();
	FPoint Point{ static_cast<LONG>(MousePosition.X), static_cast<LONG>(MousePosition.Y) };

	SWindow* Target = nullptr;

	if (Capture != nullptr)
	{
		// 드래그 캡처 중이면, 히트 테스트 없이 캡처된 위젯으로 고정
		Target = static_cast<SWindow*>(Capture);
	}
	else
	{
		// 캡처가 아니면 화면 좌표로 히트 테스트
		Target = (QuadRoot != nullptr) ? QuadRoot->HitTest(Point) : nullptr;
	}

	if (InputManager.IsKeyPressed(EKeyInput::MouseLeft) || (!Capture && InputManager.IsKeyDown(EKeyInput::MouseLeft)))
	{
		if (Target && Target->OnMouseDown(Point, 0))
		{
			Capture = Target;
		}
	}

	const FVector& Delta = InputManager.GetMouseDelta();
	if ((Delta.X != 0.0f || Delta.Y != 0.0f) && Capture)
	{
		Capture->OnMouseMove(Point);
	}

	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		if (Capture)
		{
			Capture->OnMouseUp(Point, 0);
			Capture = nullptr;
		}
	}

	if (!InputManager.IsKeyDown(EKeyInput::MouseLeft) && Capture)
	{
		Capture->OnMouseUp(Point, 0);
		Capture = nullptr;
	}
}

void UViewportManager::BuildSingleLayout(int32 PromoteIdx)
{
}

void UViewportManager::BuildFourSplitLayout()
{
}

void UViewportManager::GetLeafRects(TArray<FRect>& OutRects) const
{
	OutRects.clear();
	if (!Root)
	{
		return;
	}

	// 재귀 수집
	struct Local
	{
		static void Collect(SWindow* Node, TArray<FRect>& Out)
		{
			if (auto* Split = Cast(Node))
			{
				Collect(Split->SideLT, Out);
				Collect(Split->SideRB, Out);
			}
			else
			{
				Out.emplace_back(Node->GetRect());
			}
		}
	};
	Local::Collect(Root, OutRects);
}

int32 UViewportManager::GetViewportIndexUnderMouse() const
{
	// 범위 검사
	if (Viewports.empty())
	{
		return -1;
	}
	
	const auto& InputManager = UInputManager::GetInstance();

	const FVector& MousePosition = InputManager.GetMousePosition();
	const LONG MousePositionX = static_cast<LONG>(MousePosition.X);
	const LONG MousePositionY = static_cast<LONG>(MousePosition.Y);

	for (int32 i = 0; i < static_cast<int32>(Viewports.size()); ++i)
	{
		// FutureEngine: null 체크
		if (!Viewports[i]) 
		{
			UE_LOG_WARNING("ViewportManager::GetViewportIndexUnderMouse - Viewports[%d]가 null", i);
			continue;
		}
		
		const FRect& Rect = Viewports[i]->GetRect();
		const int32 ToolbarHeight = Viewports[i]->GetToolbarHeight();

		const LONG RenderAreaTop = Rect.Top + ToolbarHeight;
		const LONG RenderAreaBottom = Rect.Top + Rect.Height;

		if (MousePositionX >= Rect.Left && MousePositionX < Rect.Left + Rect.Width &&
			MousePositionY >= RenderAreaTop && MousePositionY < RenderAreaBottom)
		{
			return i;
		}
	}
	return -1;
}

bool UViewportManager::ComputeLocalNDCForViewport(int32 Index, float& OutNdcX, float& OutNdcY) const
{
    return false;
}

EViewModeIndex UViewportManager::GetViewportViewMode(int32 Index) const
{
	// 범위 체크
	if (Index < 0 || Index >= static_cast<int32>(Clients.size()))
	{
		UE_LOG_WARNING("ViewportManager::GetViewportViewMode - Invalid Index: %d", Index);
		return EViewModeIndex::VMI_Gouraud; // 기본값 반환
	}
	
	// FViewportClient에서 ViewMode 가져오기
	if (Clients[Index])
	{
		return Clients[Index]->GetViewMode();
	}
	
	UE_LOG_WARNING("ViewportManager::GetViewportViewMode - Clients[%d] is null", Index);
	return EViewModeIndex::VMI_Gouraud; // 기본값 반환
}

void UViewportManager::SetViewportViewMode(int32 Index, EViewModeIndex InMode)
{
	// 범위 체크
	if (Index < 0 || Index >= static_cast<int32>(Clients.size()))
	{
		UE_LOG_WARNING("ViewportManager::SetViewportViewMode - Invalid Index: %d", Index);
		return;
	}
	
	// FViewportClient에 ViewMode 설정
	if (Clients[Index])
	{
		Clients[Index]->SetViewMode(InMode);
		UE_LOG("ViewportManager: Viewport[%d]의 ViewMode를 %d로 변경", Index, static_cast<int32>(InMode));
	}
	else
	{
		UE_LOG_WARNING("ViewportManager::SetViewportViewMode - Clients[%d] is null", Index);
	}
}


void UViewportManager::PersistSplitterRatios()
{
	// 세로(Vertical) 비율은 QuadRoot(= SplitterV)에서 읽는다
	if (auto* V = Cast(QuadRoot))
		IniSaveSharedV = V->Ratio;

	// 가로(Horizontal)는 공유값(SplitterValueH)을 그대로 저장
	IniSaveSharedH = SplitterValueH;

	ViewLayoutChangeSplitterV = IniSaveSharedV;
	ViewLayoutChangeSplitterH = IniSaveSharedH;
	
}

void UViewportManager::QuadToSingleAnimation()
{
	if (ActiveIndex == 0)
	{

	}
	if (ActiveIndex == 1)
	{

	}
	if (ActiveIndex == 2)
	{

	}
	if (ActiveIndex == 3)
	{

	}
}

void UViewportManager::SingleToQuadAnimation()
{
	if (ActiveIndex == 0)
	{

	}
	if (ActiveIndex == 1)
	{

	}
	if (ActiveIndex == 2)
	{

	}
	if (ActiveIndex == 3)
	{

	}
}

void UViewportManager::StartLayoutAnimation(bool bSingleToQuad, int32 ViewportIndex)
{
	ViewportAnimation.bSingleToQuad = bSingleToQuad;
	ViewportAnimation.bIsAnimating = true;
	ViewportAnimation.AnimationTime = 0.0f;
	ViewportAnimation.PromotedViewportIndex =
		(ViewportIndex >= 0) ? ViewportIndex : ActiveIndex;

	// 공통: 최소 사이즈 0으로 (끝까지 밀 수 있게)
	if (auto* RootSplit = Cast(QuadRoot))
	{
		ViewportAnimation.SavedMinChildSizeV = RootSplit->MinChildSize;
		RootSplit->MinChildSize = 0;

		if (auto* HLeft = Cast(RootSplit->SideLT)) {
			ViewportAnimation.SavedMinChildSizeH = HLeft->MinChildSize;
			HLeft->MinChildSize = 0;
		}
		if (auto* HRight = Cast(RootSplit->SideRB)) {
			if (auto* HR = Cast(HRight)) HR->MinChildSize = 0;
		}
	}

	if (!bSingleToQuad)
	{
		// ===== Quad -> Single (기존과 동일) =====
		ViewportAnimation.StartVRatio = SplitterValueV;
		ViewportAnimation.StartHRatio = SplitterValueH;

		switch (ViewportAnimation.PromotedViewportIndex)
		{
		case 0: ViewportAnimation.TargetVRatio = 1.0f; ViewportAnimation.TargetHRatio = 1.0f; break; // LT
		case 1: ViewportAnimation.TargetVRatio = 1.0f; ViewportAnimation.TargetHRatio = 0.0f; break; // LB
		case 2: ViewportAnimation.TargetVRatio = 0.0f; ViewportAnimation.TargetHRatio = 1.0f; break; // RT
		case 3: ViewportAnimation.TargetVRatio = 0.0f; ViewportAnimation.TargetHRatio = 0.0f; break; // RB
		default: ViewportAnimation.TargetVRatio = SplitterValueV; ViewportAnimation.TargetHRatio = SplitterValueH; break;
		}

		ViewportLayout = EViewportLayout::Quad;
		ViewportAnimation.BackupRoot = Root;
		Root = QuadRoot; // 애니 동안 쿼드
	}
	else
	{
		// ===== Single -> Quad =====
		ActiveIndex = ViewportAnimation.PromotedViewportIndex;

		// 시작값: 현재 싱글이 차지하는 방향으로 핸들을 끝까지 보낸 비율
		switch (ActiveIndex)
		{
		case 0: ViewportAnimation.StartVRatio = 1.0f; ViewportAnimation.StartHRatio = 1.0f; break; // LT
		case 1: ViewportAnimation.StartVRatio = 1.0f; ViewportAnimation.StartHRatio = 0.0f; break; // LB
		case 2: ViewportAnimation.StartVRatio = 0.0f; ViewportAnimation.StartHRatio = 1.0f; break; // RT
		case 3: ViewportAnimation.StartVRatio = 0.0f; ViewportAnimation.StartHRatio = 0.0f; break; // RB
		default: ViewportAnimation.StartVRatio = 0.5f; ViewportAnimation.StartHRatio = 0.5f; break;
		}

		// 목표값: 이전에 저장해 둔 쿼드 배치 비율
		ViewportAnimation.TargetVRatio = std::clamp(ViewLayoutChangeSplitterV, 0.0f, 1.0f);
		ViewportAnimation.TargetHRatio = std::clamp(ViewLayoutChangeSplitterH, 0.0f, 1.0f);
		// fallback
		if (!(ViewportAnimation.TargetVRatio >= 0.0f && ViewportAnimation.TargetVRatio <= 1.0f))
			ViewportAnimation.TargetVRatio = IniSaveSharedV;
		if (!(ViewportAnimation.TargetHRatio >= 0.0f && ViewportAnimation.TargetHRatio <= 1.0f))
			ViewportAnimation.TargetHRatio = IniSaveSharedH;

		// 현재 내부 상태도 시작값으로 셋
		SplitterValueV = ViewportAnimation.StartVRatio;
		SplitterValueH = ViewportAnimation.StartHRatio;

		// 루트 백업
		ViewportAnimation.BackupRoot = Root;

		// 실제 스플리터에 시작값을 반영하고 배치 (루트 전환 전에 먼저 수행)
		if (auto* RootSplit = Cast(QuadRoot))
		{
			RootSplit->SetEffectiveRatio(SplitterValueV);
			if (auto* HLeft = Cast(RootSplit->SideLT))  HLeft->SetEffectiveRatio(SplitterValueH);
			if (auto* HRight = Cast(RootSplit->SideRB)) HRight->SetEffectiveRatio(SplitterValueH);
			RootSplit->OnResize(ActiveViewportRect);
		}

		// 뷰포트 동기화 (렌더링 전 정확한 상태 설정)
		SyncRectsToViewports();

		// 애니 시작 시점에 쿼드 레이아웃으로 전환 (렌더링 직전에 수행)
		ViewportLayout = EViewportLayout::Quad;
		Root = QuadRoot;
	}
}

void UViewportManager::SyncRectsToViewports() const
{
	// 쿼드일 때 4개 모두, 싱글일 때 Active만
	if (ViewportLayout == EViewportLayout::Quad)
	{
		for (int i = 0; i < 4; ++i)
		{
			if (Leaves[i] && Viewports[i] && Clients[i])
			{
				Viewports[i]->SetRect(Leaves[i]->GetRect());
				const float Aspect = Viewports[i]->GetAspect();
				// FutureEngine: Camera null 체크
				if (UCamera* Cam = Clients[i]->GetCamera())
				{
					Cam->SetAspect(Aspect);
				}
			}
		}
	}
	else
	{
		if (Leaves[ActiveIndex] && Viewports[ActiveIndex] && Clients[ActiveIndex])
		{
			Viewports[ActiveIndex]->SetRect(Leaves[ActiveIndex]->GetRect());
			const float Aspect = Viewports[ActiveIndex]->GetAspect();
			// FutureEngine: Camera null 체크
			if (UCamera* Cam = Clients[ActiveIndex]->GetCamera())
			{
				Cam->SetAspect(Aspect);
			}
		}
	}
}

void UViewportManager::SyncAnimationRectsToViewports() const
{
}

void UViewportManager::PumpAllViewportInput() const
{
}

void UViewportManager::TickCameras() const
{
}

void UViewportManager::UpdateActiveRmbViewportIndex()
{
}

void UViewportManager::InitializeViewportAndClient()
{
	UE_LOG("ViewportManager: InitializeViewportAndClient 시작");
	
	for (int i = 0; i < 4; i++)
	{
		FViewport* Viewport = new FViewport();
		FViewportClient* ViewportClient = new FViewportClient();

		// FutureEngine 철학: 양방향 관계 설정
		Viewport->SetViewportClient(ViewportClient);
		ViewportClient->SetOwningViewport(Viewport);

		Clients.push_back(ViewportClient);
		Viewports.push_back(Viewport);
		
		UE_LOG("ViewportManager: Viewport[%d] 초기화 완료", i);
	}
	// 이 값들은 조절가능 일단 기본으로 셋
	Clients[0]->SetViewType(EViewType::Perspective);
	Clients[0]->SetViewMode(EViewModeIndex::VMI_BlinnPhong);

	Clients[1]->SetViewType(EViewType::Perspective);
	Clients[1]->SetViewMode(EViewModeIndex::VMI_Lambert);

	Clients[2]->SetViewType(EViewType::Perspective);
	Clients[2]->SetViewMode(EViewModeIndex::VMI_Gouraud);

	Clients[3]->SetViewType(EViewType::Perspective);
	Clients[3]->SetViewMode(EViewModeIndex::VMI_Unlit);
	UE_LOG("ViewportManager: 총 %zu개 Viewport, %zu개 Client 생성", Viewports.size(), Clients.size());
}


	


void UViewportManager::InitializeOrthoGraphicCamera()
{
}

void UViewportManager::InitializePerspectiveCamera()
{
}

void UViewportManager::UpdateOrthoGraphicCameraPoint()
{
}

void UViewportManager::UpdateOrthoGraphicCameraFov() const
{
}

void UViewportManager::BindOrthoGraphicCameraToClient() const
{
}

void UViewportManager::ForceRefreshOrthoViewsAfterLayoutChange()
{
}

void UViewportManager::ApplySharedOrthoCenterToAllCameras() const
{
}

EViewportLayout UViewportManager::GetViewportLayout() const
{
	return ViewportLayout;
}

void UViewportManager::SetViewportLayout(EViewportLayout InViewportLayout)
{
	ViewportLayout = InViewportLayout;
}

void UViewportManager::StartViewportAnimation(bool bSingleToQuad, int32 PromoteIdx)
{
}

void UViewportManager::UpdateViewportAnimation()
{
	if (!ViewportAnimation.bIsAnimating) return;

	ViewportAnimation.AnimationTime += DT;

	float t = ViewportAnimation.AnimationTime / ViewportAnimation.AnimationDuration;
	float k = EaseInOutCubic(std::clamp(t, 0.0f, 1.0f));

	// 보간
	SplitterValueV = Lerp(ViewportAnimation.StartVRatio, ViewportAnimation.TargetVRatio, k);
	SplitterValueH = Lerp(ViewportAnimation.StartHRatio, ViewportAnimation.TargetHRatio, k);

	if (auto* RootSplit = Cast(QuadRoot))
	{
		RootSplit->SetEffectiveRatio(SplitterValueV);
		if (auto* HLeft = Cast(RootSplit->SideLT))  HLeft->SetEffectiveRatio(SplitterValueH);
		if (auto* HRight = Cast(RootSplit->SideRB)) HRight->SetEffectiveRatio(SplitterValueH);
		RootSplit->OnResize(ActiveViewportRect);
	}

	SyncRectsToViewports();

	if (t >= 1.0f)
	{
		if (ViewportAnimation.bSingleToQuad)
			FinalizeFourSplitLayoutFromAnimation();
		else
			FinalizeSingleLayoutFromAnimation();
	}
}

float UViewportManager::EaseInOutCubic(float t) const
{
	// 0~1 범위 입력 보장
	t = std::clamp(t, 0.0f, 1.0f);
	return (t < 0.5f) ? (4.0f * t * t * t) : (1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f);
}

void UViewportManager::CreateAnimationSplitters()
{
}

void UViewportManager::AnimateSplitterTransition(float Progress)
{
}

void UViewportManager::RestoreOriginalLayout()
{
}

void UViewportManager::FinalizeSingleLayoutFromAnimation()
{
	ViewportAnimation.bIsAnimating = false;

	// (1) 드래그 캡처 중이면 해제
	Capture = nullptr;

	// (2) MinChildSize 원복
	if (auto* RootSplit = Cast(QuadRoot))
	{
		RootSplit->MinChildSize = ViewportAnimation.SavedMinChildSizeV;
		if (auto* HLeft = Cast(RootSplit->SideLT)) HLeft->MinChildSize = ViewportAnimation.SavedMinChildSizeH;
		if (auto* HRight = Cast(RootSplit->SideRB)) HRight->MinChildSize = ViewportAnimation.SavedMinChildSizeH;
	}

	// (3) 싱글 모드 전환
	ActiveIndex = ViewportAnimation.PromotedViewportIndex;
	ViewportLayout = EViewportLayout::Single;

	// (4) 싱글 루트 지정 + 전체 크기로 리사이즈
	Root = Leaves[ActiveIndex];
	if (Root)
		Root->OnResize(ActiveViewportRect);

	// (5) 저장
	IniSaveSharedV = SplitterValueV;
	IniSaveSharedH = SplitterValueH;

	// (6) 뷰포트 동기화 (싱글만 활성)
	SyncRectsToViewports();
}

void UViewportManager::FinalizeFourSplitLayoutFromAnimation()
{
	ViewportAnimation.bIsAnimating = false;
	Capture = nullptr;

	// 최소 사이즈 원복
	if (auto* RootSplit = Cast(QuadRoot))
	{
		RootSplit->MinChildSize = ViewportAnimation.SavedMinChildSizeV;
		if (auto* HLeft = Cast(RootSplit->SideLT))  HLeft->MinChildSize = ViewportAnimation.SavedMinChildSizeH;
		if (auto* HRight = Cast(RootSplit->SideRB)) HRight->MinChildSize = ViewportAnimation.SavedMinChildSizeH;
	}

	// 최종 비율 고정
	SplitterValueV = ViewportAnimation.TargetVRatio;
	SplitterValueH = ViewportAnimation.TargetHRatio;

	// 쿼드 레이아웃 확정
	ViewportLayout = EViewportLayout::Quad;
	Root = QuadRoot;

	// 한 번 더 적용/배치
	if (auto* RootSplit = Cast(QuadRoot))
	{
		RootSplit->SetEffectiveRatio(SplitterValueV);
		if (auto* HLeft = Cast(RootSplit->SideLT))  HLeft->SetEffectiveRatio(SplitterValueH);
		if (auto* HRight = Cast(RootSplit->SideRB)) HRight->SetEffectiveRatio(SplitterValueH);
		RootSplit->OnResize(ActiveViewportRect);
	}

	SyncRectsToViewports();

	// 다음 전환 대비 저장(선택)
	IniSaveSharedV = SplitterValueV;
	IniSaveSharedH = SplitterValueH;
}

void UViewportManager::SerializeViewports(const bool bInIsLoading, JSON& InOutHandle)
{
	// 저장/로드 모두에서 쓸 공통 키
	const char* ViewportSystemKey = "ViewportSystem";

	if (bInIsLoading)
	{
		JSON ViewportSystemJson;
		if (!FJsonSerializer::ReadObject(InOutHandle, ViewportSystemKey, ViewportSystemJson))
		{
			return; // 저장된 뷰포트 정보가 없으면 무시
		}

		// 1) 레이아웃/활성뷰/스플리터 비율
		int32 LayoutInt = 0;
		int32 LoadedActiveIndex = 0;
		float LoadedSplitterV = 0.5f;
		float LoadedSplitterH = 0.5f;

		FJsonSerializer::ReadInt32(ViewportSystemJson, "Layout", LayoutInt, 0);
		FJsonSerializer::ReadInt32(ViewportSystemJson, "ActiveIndex", LoadedActiveIndex, 0);
		FJsonSerializer::ReadFloat(ViewportSystemJson, "SplitterV", LoadedSplitterV);
		FJsonSerializer::ReadFloat(ViewportSystemJson, "SplitterH", LoadedSplitterH);

		SetViewportLayout(static_cast<EViewportLayout>(LayoutInt));
		SetActiveIndex(LoadedActiveIndex);
		// 내부 비율 값에 적재
		// 주의: 실제 코드에선 멤버 접근자 사용 권장. 여기에선 IniSaveSharedV/H를 복원해 둡니다.
		//      PersistSplitterRatios()가 이 값들을 복사해 사용합니다.
		//      파일 접근 없이 멤버를 직접 만지는 대신 아래 두 라인을 적절히 “설정 함수”로 대체해도 됩니다.
		//      (현재 코드베이스에는 setter가 없으니 멤버를 그대로 쓰는 패턴을 유지)
		IniSaveSharedV = LoadedSplitterV;
		IniSaveSharedH = LoadedSplitterH;

		// 2) 뷰포트 배열
		JSON ViewportsArray;
		if (FJsonSerializer::ReadArray(ViewportSystemJson, "Viewports", ViewportsArray))
		{
			// 최대 4개 기준
			const int32 SavedCount = static_cast<int32>(ViewportsArray.size());
			const int32 ClientCount = static_cast<int32>(Clients.size());
			const int32 RestoreCount = std::min(SavedCount, ClientCount);

			for (int32 Index = 0; Index < RestoreCount; ++Index)
			{
				JSON& ViewportJson = ViewportsArray[Index];

				// View type
				int32 ViewTypeInt = 0;
				FJsonSerializer::ReadInt32(ViewportJson, "ViewType", ViewTypeInt, 0);

				if (FViewportClient* Client = Clients[Index])
				{
					Client->SetViewType(static_cast<EViewType>(ViewTypeInt));

					// ViewMode(있으면)
					int32 ViewModeInt = static_cast<int32>(Client->GetViewMode());
					FJsonSerializer::ReadInt32(ViewportJson, "ViewMode", ViewModeInt, ViewModeInt);
					Client->SetViewMode(static_cast<EViewModeIndex>(ViewModeInt));

					// Camera
					if (UCamera* Camera = Client->GetCamera())
					{
						JSON CameraJson;
						if (FJsonSerializer::ReadObject(ViewportJson, "Camera", CameraJson))
						{
							int32 CameraTypeInt = static_cast<int32>(Camera->GetCameraType());
							FJsonSerializer::ReadInt32(CameraJson, "CameraType", CameraTypeInt, CameraTypeInt);
							Camera->SetCameraType(static_cast<ECameraType>(CameraTypeInt));

							FVector SavedLocation, SavedRotation;
							FJsonSerializer::ReadVector(CameraJson, "Location", SavedLocation, Camera->GetLocation());
							FJsonSerializer::ReadVector(CameraJson, "Rotation", SavedRotation, Camera->GetRotation());

							float SavedFovY = Camera->GetFovY();
							float SavedNearZ = Camera->GetNearZ();
							float SavedFarZ = Camera->GetFarZ();
							float SavedOrthoWidth = Camera->GetOrthoWidth();
							float SavedMoveSpeed = Camera->GetMoveSpeed();

							FJsonSerializer::ReadFloat(CameraJson, "FovY", SavedFovY);
							FJsonSerializer::ReadFloat(CameraJson, "NearZ", SavedNearZ);
							FJsonSerializer::ReadFloat(CameraJson, "FarZ", SavedFarZ);
							FJsonSerializer::ReadFloat(CameraJson, "OrthoWidth", SavedOrthoWidth);
							FJsonSerializer::ReadFloat(CameraJson, "MoveSpeed", SavedMoveSpeed);

							Camera->SetLocation(SavedLocation);
							Camera->SetRotation(SavedRotation);
							Camera->SetFovY(SavedFovY);
							Camera->SetNearZ(SavedNearZ);
							Camera->SetFarZ(SavedFarZ);
							Camera->SetOrthoWidth(SavedOrthoWidth);
							Camera->SetMoveSpeed(SavedMoveSpeed);
						}
					}
				}
			}
		}
	}
	else
	{
		JSON ViewportSystemJson = json::Object();

		// 1) 레이아웃/활성뷰/스플리터 비율 저장
		ViewportSystemJson["Layout"] = static_cast<int32>(GetViewportLayout());
		ViewportSystemJson["ActiveIndex"] = GetActiveIndex();
		ViewportSystemJson["SplitterV"] = IniSaveSharedV;
		ViewportSystemJson["SplitterH"] = IniSaveSharedH;

		// 2) 각 뷰포트 상태 저장
		JSON ViewportsArray = json::Array();
		const int32 ClientCount = static_cast<int32>(Clients.size());
		for (int32 Index = 0; Index < ClientCount; ++Index)
		{
			FViewportClient* Client = Clients[Index];
			if (!Client)
			{
				continue;
			}

			JSON ViewportJson = json::Object();
			ViewportJson["Index"] = Index;
			ViewportJson["ViewType"] = static_cast<int32>(Client->GetViewType());
			ViewportJson["ViewMode"] = static_cast<int32>(Client->GetViewMode());

			if (UCamera* Camera = Client->GetCamera())
			{
				JSON CameraJson = json::Object();
				CameraJson["CameraType"] = static_cast<int32>(Camera->GetCameraType());
				CameraJson["Location"] = FJsonSerializer::VectorToJson(Camera->GetLocation());
				CameraJson["Rotation"] = FJsonSerializer::VectorToJson(Camera->GetRotation());
				CameraJson["FovY"] = Camera->GetFovY();
				CameraJson["NearZ"] = Camera->GetNearZ();
				CameraJson["FarZ"] = Camera->GetFarZ();
				CameraJson["OrthoWidth"] = Camera->GetOrthoWidth();
				CameraJson["MoveSpeed"] = Camera->GetMoveSpeed();

				ViewportJson["Camera"] = CameraJson;
			}

			ViewportsArray.append(ViewportJson);
		}

		ViewportSystemJson["Viewports"] = ViewportsArray;

		InOutHandle[ViewportSystemKey] = ViewportSystemJson;
	}

}