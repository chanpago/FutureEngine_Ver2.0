#include "pch.h"
#include "Render/UI/Widget/Public/ViewportControlWidget.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Editor/Public/Editor.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Texture/Public/Texture.h"

// 정적 멤버 정의 - KTLWeek07의 뷰 모드 기능 사용
const char* UViewportControlWidget::ViewModeLabels[] = {
	"Gouraud", "Lambert", "BlinnPhong", "Unlit", "Wireframe", "SceneDepth", "WorldNormal"
};

const char* UViewportControlWidget::ViewTypeLabels[] = {
	"Perspective", "Top", "Bottom", "Left", "Right", "Front", "Back"
};

UViewportControlWidget::UViewportControlWidget()
	: UWidget("Viewport Control Widget")
{
}

UViewportControlWidget::~UViewportControlWidget() = default;

void UViewportControlWidget::Initialize()
{
	LoadViewIcons();
	UE_LOG("ViewportControlWidget: Initialized");
}

void UViewportControlWidget::LoadViewIcons()
{
	if (bIconsLoaded) return;

	UE_LOG("ViewportControlWidget: 아이콘 로드 시작...");
	UAssetManager& AssetManager = UAssetManager::GetInstance();
	UPathManager& PathManager = UPathManager::GetInstance();
	FString IconBasePath = PathManager.GetAssetPath().string() + "\\Icon\\";

	int32 LoadedCount = 0;

	IconPerspective = AssetManager.LoadTexture(IconBasePath + "ViewPerspective.png");
	if (IconPerspective) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'ViewPerspective' -> %p", IconPerspective);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "ViewPerspective.png").c_str());
	}

	IconTop = AssetManager.LoadTexture(IconBasePath + "ViewTop.png");
	if (IconTop) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'ViewTop' -> %p", IconTop);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "ViewTop.png").c_str());
	}

	IconBottom = AssetManager.LoadTexture(IconBasePath + "ViewBottom.png");
	if (IconBottom) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'ViewBottom' -> %p", IconBottom);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "ViewBottom.png").c_str());
	}

	IconLeft = AssetManager.LoadTexture(IconBasePath + "ViewLeft.png");
	if (IconLeft) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'ViewLeft' -> %p", IconLeft);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "ViewLeft.png").c_str());
	}

	IconRight = AssetManager.LoadTexture(IconBasePath + "ViewRight.png");
	if (IconRight) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'ViewRight' -> %p", IconRight);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "ViewRight.png").c_str());
	}

	IconFront = AssetManager.LoadTexture(IconBasePath + "ViewFront.png");
	if (IconFront) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'ViewFront' -> %p", IconFront);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "ViewFront.png").c_str());
	}

	IconBack = AssetManager.LoadTexture(IconBasePath + "ViewBack.png");
	if (IconBack) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'ViewBack' -> %p", IconBack);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "ViewBack.png").c_str());
	}

	// ViewMode 아이콘 로드
	IconLitCube = AssetManager.LoadTexture(IconBasePath + "LitCube.png");
	if (IconLitCube) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'LitCube' -> %p", IconLitCube);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "LitCube.png").c_str());
	}

	// 레이아웃 전환 아이콘 로드
	IconQuad = AssetManager.LoadTexture(IconBasePath + "quad.png");
	if (IconQuad) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'quad' -> %p", IconQuad);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "quad.png").c_str());
	}

	IconSquare = AssetManager.LoadTexture(IconBasePath + "square.png");
	if (IconSquare) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'square' -> %p", IconSquare);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "square.png").c_str());
	}

	// 카메라 설정 아이콘 로드
	IconCamera = AssetManager.LoadTexture(IconBasePath + "Camera.png");
	if (IconCamera) {
		UE_LOG("ViewportControlWidget: 아이콘 로드 성공: 'Camera' -> %p", IconCamera);
		LoadedCount++;
	} else {
		UE_LOG_WARNING("ViewportControlWidget: 아이콘 로드 실패: %s", (IconBasePath + "Camera.png").c_str());
	}

	UE_LOG_SUCCESS("ViewportControlWidget: 아이콘 로드 완료 (%d/11)", LoadedCount);
	bIconsLoaded = true;
}

void UViewportControlWidget::Update()
{
	// 필요시 업데이트 로직 추가
	
}

void UViewportControlWidget::RenderWidget()
{
	auto& ViewportManager = UViewportManager::GetInstance();
	if (!ViewportManager.GetRoot())
	{
		return;
	}

	// 먼저 스플리터 선 렌더링 (UI 뒤에서)
	//RenderSplitterLines();

	// 싱글 모드에서는 하나만 렌더링
	if (ViewportManager.GetViewportLayout() == EViewportLayout::Single)
	{
		int32 ActiveViewportIndex = ViewportManager.GetActiveIndex();
		RenderViewportToolbar(ActiveViewportIndex);
	}
	else
	{
		for (int32 i = 0;i < 4;++i)
		{
			RenderViewportToolbar(i);
		}
	}
}

void UViewportControlWidget::RenderViewportToolbar(int32 ViewportIndex)
{
	auto& ViewportManager = UViewportManager::GetInstance();
	const auto& Viewports = ViewportManager.GetViewports();
	const auto& Clients = ViewportManager.GetClients();

	// 뷰포트 범위가 벗어날 경우 종료
	if (ViewportIndex >= static_cast<int32>(Viewports.size()) || ViewportIndex >= static_cast<int32>(Clients.size()))
	{
		return;
	}

	// FutureEngine: null 체크 추가
	if (!Viewports[ViewportIndex] || !Clients[ViewportIndex])
	{
		return;
	}

	const FRect& Rect = Viewports[ViewportIndex]->GetRect();
	if (Rect.Width <= 0 || Rect.Height <= 0)
	{
		return;
	}


	const int32 ToolbarH = 32;
	const ImVec2 Vec1{ static_cast<float>(Rect.Left), static_cast<float>(Rect.Top) };
	const ImVec2 Vec2{ static_cast<float>(Rect.Left + Rect.Width), static_cast<float>(Rect.Top + ToolbarH) };

	// 1) 툴바 배경 그리기
	ImDrawList* DrawLine = ImGui::GetBackgroundDrawList();
	DrawLine->AddRectFilled(Vec1, Vec2, IM_COL32(30, 30, 30, 100));
	DrawLine->AddLine(ImVec2(Vec1.x, Vec2.y), ImVec2(Vec2.x, Vec2.y), IM_COL32(70, 70, 70, 120), 1.0f);

	// 2) 툴바 UI 요소들을 직접 배치 (별도 창 생성하지 않음)
	// UI 요소들을 화면 좌표계에서 직접 배치
	ImGui::SetNextWindowPos(ImVec2(Vec1.x, Vec1.y));
	ImGui::SetNextWindowSize(ImVec2(Vec2.x - Vec1.x, Vec2.y - Vec1.y));

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoTitleBar;

	// 스타일 설정
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));

	char WinName[64];
	(void)snprintf(WinName, sizeof(WinName), "##ViewportToolbar%d", ViewportIndex);

	ImGui::PushID(ViewportIndex);
	if (ImGui::Begin(WinName, nullptr, flags))
	{
		// ViewMode 커스텀 버튼
		EViewModeIndex CurrentMode = Clients[ViewportIndex]->GetViewMode();
		int32 CurrentModeIndex = static_cast<int32>(CurrentMode);

		// 버튼 스타일 설정
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

		// 커스텀 버튼 (LitCube 아이콘 + 텍스트)
		const float buttonWidth = 140.0f;
		const float buttonHeight = 20.0f;
		const float iconSize = 16.0f;
		const float padding = 4.0f;

		ImVec2 buttonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##ViewModeButton", ImVec2(buttonWidth, buttonHeight));
		bool bClicked = ImGui::IsItemClicked();
		bool bHovered = ImGui::IsItemHovered();

		// 버튼 배경 그리기
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImU32 bgColor = bHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive())
		{
			bgColor = IM_COL32(38, 38, 38, 255);
		}
		drawList->AddRectFilled(buttonPos, ImVec2(buttonPos.x + buttonWidth, buttonPos.y + buttonHeight), bgColor, 4.0f);
		drawList->AddRect(buttonPos, ImVec2(buttonPos.x + buttonWidth, buttonPos.y + buttonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

		// LitCube 아이콘 그리기
		if (IconLitCube && IconLitCube->GetTextureSRV())
		{
			ImVec2 iconPos = ImVec2(buttonPos.x + padding, buttonPos.y + (buttonHeight - iconSize) * 0.5f);
			drawList->AddImage(
				(ImTextureID)IconLitCube->GetTextureSRV(),
				iconPos,
				ImVec2(iconPos.x + iconSize, iconPos.y + iconSize)
			);
		}

		// 텍스트 그리기
		ImVec2 textPos = ImVec2(buttonPos.x + padding + iconSize + padding, buttonPos.y + (buttonHeight - ImGui::GetTextLineHeight()) * 0.5f);
		drawList->AddText(textPos, IM_COL32(220, 220, 220, 255), ViewModeLabels[CurrentModeIndex]);

		// 버튼 클릭 시 팝업 열기
		if (bClicked)
		{
			ImGui::OpenPopup("##ViewModePopup");
		}

		// 팝업 메뉴
		if (ImGui::BeginPopup("##ViewModePopup"))
		{
			for (int i = 0; i < IM_ARRAYSIZE(ViewModeLabels); ++i)
			{
				// 각 항목에 LitCube 아이콘 표시
				if (IconLitCube && IconLitCube->GetTextureSRV())
				{
					ImGui::Image((ImTextureID)IconLitCube->GetTextureSRV(), ImVec2(16, 16));
					ImGui::SameLine();
				}
				
				if (ImGui::MenuItem(ViewModeLabels[i], nullptr, i == CurrentModeIndex))
				{
					Clients[ViewportIndex]->SetViewMode(static_cast<EViewModeIndex>(i));
					UE_LOG("ViewportControlWidget: Viewport[%d]의 ViewMode를 %s로 변경",
						ViewportIndex, ViewModeLabels[i]);
				}
			}
			ImGui::EndPopup();
		}
		
		ImGui::PopStyleColor(4);

		// 구분자
		ImGui::SameLine(0.0f, 10.0f);
		ImGui::TextDisabled("|");
		ImGui::SameLine(0.0f, 10.0f);

		// ViewType 버튼 + 팝업 메뉴
		EViewType CurType = Clients[ViewportIndex]->GetViewType();
		int32 CurrentIdx = static_cast<int32>(CurType);

		// 현재 뷰 타입에 맞는 아이콘 가져오기
		UTexture* CurrentIcon = nullptr;
		UTexture* Icons[7] = { IconPerspective, IconTop, IconBottom, IconLeft, IconRight, IconFront, IconBack };
		
		switch (CurType)
		{
			case EViewType::Perspective: CurrentIcon = IconPerspective; break;
			case EViewType::OrthoTop: CurrentIcon = IconTop; break;
			case EViewType::OrthoBottom: CurrentIcon = IconBottom; break;
			case EViewType::OrthoLeft: CurrentIcon = IconLeft; break;
			case EViewType::OrthoRight: CurrentIcon = IconRight; break;
			case EViewType::OrthoFront: CurrentIcon = IconFront; break;
			case EViewType::OrthoBack: CurrentIcon = IconBack; break;
		}

		// 버튼 스타일 설정
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

		// 커스텀 버튼 (아이콘 + 텍스트 포함)
		const float viewTypeButtonWidth = 140.0f;
		const float viewTypeButtonHeight = 20.0f;
		const float viewTypeIconSize = 16.0f;
		const float viewTypePadding = 4.0f;

		// 보이지 않는 버튼으로 클릭 감지
		ImVec2 viewTypeButtonPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##ViewTypeButton", ImVec2(viewTypeButtonWidth, viewTypeButtonHeight));
		bool bViewTypeClicked = ImGui::IsItemClicked();
		bool bViewTypeHovered = ImGui::IsItemHovered();

		// 버튼 배경 그리기
		ImDrawList* viewTypeDrawList = ImGui::GetWindowDrawList();
		ImU32 viewTypeBgColor = bViewTypeHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
		if (ImGui::IsItemActive())
		{
			viewTypeBgColor = IM_COL32(38, 38, 38, 255);
		}
		viewTypeDrawList->AddRectFilled(viewTypeButtonPos, ImVec2(viewTypeButtonPos.x + viewTypeButtonWidth, viewTypeButtonPos.y + viewTypeButtonHeight), viewTypeBgColor, 4.0f);
		viewTypeDrawList->AddRect(viewTypeButtonPos, ImVec2(viewTypeButtonPos.x + viewTypeButtonWidth, viewTypeButtonPos.y + viewTypeButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

		// 아이콘 그리기
		if (CurrentIcon && CurrentIcon->GetTextureSRV())
		{
			ImVec2 viewTypeIconPos = ImVec2(viewTypeButtonPos.x + viewTypePadding, viewTypeButtonPos.y + (viewTypeButtonHeight - viewTypeIconSize) * 0.5f);
			viewTypeDrawList->AddImage(
				(ImTextureID)CurrentIcon->GetTextureSRV(),
				viewTypeIconPos,
				ImVec2(viewTypeIconPos.x + viewTypeIconSize, viewTypeIconPos.y + viewTypeIconSize)
			);
		}

		// 텍스트 그리기
		ImVec2 viewTypeTextPos = ImVec2(viewTypeButtonPos.x + viewTypePadding + viewTypeIconSize + viewTypePadding, viewTypeButtonPos.y + (viewTypeButtonHeight - ImGui::GetTextLineHeight()) * 0.5f);
		viewTypeDrawList->AddText(viewTypeTextPos, IM_COL32(220, 220, 220, 255), ViewTypeLabels[CurrentIdx]);

		// 버튼 클릭 시 팝업 열기
		if (bViewTypeClicked)
		{
			ImGui::OpenPopup("##ViewTypePopup");
		}

		// 팝업 메뉴
		if (ImGui::BeginPopup("##ViewTypePopup"))
		{
			for (int i = 0; i < IM_ARRAYSIZE(ViewTypeLabels); ++i)
			{
				// 각 항목에 아이콘 표시
				if (Icons[i] && Icons[i]->GetTextureSRV())
				{
					ImGui::Image((ImTextureID)Icons[i]->GetTextureSRV(), ImVec2(16, 16));
					ImGui::SameLine();
				}
				
				if (ImGui::MenuItem(ViewTypeLabels[i], nullptr, i == CurrentIdx))
				{
					EViewType NewType = static_cast<EViewType>(i);
					Clients[ViewportIndex]->SetViewType(NewType);
					UE_LOG("ViewportControlWidget: Viewport[%d]의 ViewType을 %s로 변경",
						ViewportIndex, ViewTypeLabels[i]);
				}
			}
			ImGui::EndPopup();
		}
		
		ImGui::PopStyleColor(4);

		// Perspective 모드일 때만 카메라 설정 버튼 표시
		if (CurType == EViewType::Perspective)
		{
			// 구분자
			ImGui::SameLine(0.0f, 10.0f);
			ImGui::TextDisabled("|");
			ImGui::SameLine(0.0f, 10.0f);

			// 카메라 설정 버튼
			if (IconCamera && IconCamera->GetTextureSRV())
			{
				const float cameraButtonHeight = 24.0f;
				const float cameraIconSize = 16.0f;
				const float cameraPadding = 6.0f;  // 패딩 증가
				const char* cameraText = "Camera Settings";
				const float textWidth = ImGui::CalcTextSize(cameraText).x;
				const float cameraButtonWidth = cameraIconSize + cameraPadding * 3 + textWidth;  // 패딩 공간 증가

				ImVec2 cameraButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##CameraSettingsButton", ImVec2(cameraButtonWidth, cameraButtonHeight));
				bool bCameraClicked = ImGui::IsItemClicked();
				bool bCameraHovered = ImGui::IsItemHovered();

				// 버튼 배경 그리기
				ImDrawList* cameraDrawList = ImGui::GetWindowDrawList();
				ImU32 cameraBgColor = bCameraHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
				if (ImGui::IsItemActive())
				{
					cameraBgColor = IM_COL32(38, 38, 38, 255);
				}
				cameraDrawList->AddRectFilled(cameraButtonPos, ImVec2(cameraButtonPos.x + cameraButtonWidth, cameraButtonPos.y + cameraButtonHeight), cameraBgColor, 4.0f);
				cameraDrawList->AddRect(cameraButtonPos, ImVec2(cameraButtonPos.x + cameraButtonWidth, cameraButtonPos.y + cameraButtonHeight), IM_COL32(96, 96, 96, 255), 4.0f);

				// 아이콘 그리기 (왼쪽 패딩)
				ImVec2 cameraIconPos = ImVec2(
					cameraButtonPos.x + cameraPadding,
					cameraButtonPos.y + (cameraButtonHeight - cameraIconSize) * 0.5f
				);
				cameraDrawList->AddImage(
					(ImTextureID)IconCamera->GetTextureSRV(),
					cameraIconPos,
					ImVec2(cameraIconPos.x + cameraIconSize, cameraIconPos.y + cameraIconSize)
				);

				// 텍스트 그리기 (아이콘 오른쪽)
				ImVec2 cameraTextPos = ImVec2(
					cameraButtonPos.x + cameraPadding + cameraIconSize + cameraPadding,
					cameraButtonPos.y + (cameraButtonHeight - ImGui::GetTextLineHeight()) * 0.5f
				);
				cameraDrawList->AddText(cameraTextPos, IM_COL32(220, 220, 220, 255), cameraText);

				if (bCameraClicked)
				{
					ImGui::OpenPopup("##CameraSettingsPopup");
				}

				// 카메라 설정 팝업
				if (ImGui::BeginPopup("##CameraSettingsPopup"))
				{
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

					if (UCamera* Camera = Clients[ViewportIndex]->GetCamera())
					{
						ImGui::Text("Camera Settings");
						ImGui::Separator();

						// 카메라 이동 속도
						float moveSpeed = Camera->GetMoveSpeed();
						if (ImGui::SliderFloat("Move Speed", &moveSpeed, UCamera::MIN_SPEED, UCamera::MAX_SPEED, "%.1f"))
						{
							Camera->SetMoveSpeed(moveSpeed);
						}

						// 카메라 위치
						auto& location = Camera->GetLocation();
						ImGui::DragFloat3("Location", &location.X, 0.1f);

						// 카메라 회전
						auto& rotation = Camera->GetRotation();
						ImGui::DragFloat3("Rotation", &rotation.X, 0.5f);

						ImGui::Separator();

						// FOV
						float fov = Camera->GetFovY();
						if (ImGui::SliderFloat("FOV", &fov, 1.0f, 170.0f, "%.1f"))
						{
							Camera->SetFovY(fov);
						}

						// Near/Far Plane
						float nearZ = Camera->GetNearZ();
						if (ImGui::DragFloat("Near Z", &nearZ, 0.01f, 0.01f, 100.0f, "%.3f"))
						{
							Camera->SetNearZ(nearZ);
						}

						float farZ = Camera->GetFarZ();
						if (ImGui::DragFloat("Far Z", &farZ, 1.0f, 1.0f, 10000.0f, "%.1f"))
						{
							Camera->SetFarZ(farZ);
						}
					}

					ImGui::PopStyleColor(3);
					ImGui::EndPopup();
				}
			}
		}

		// 레이아웃 전환 버튼들을 우측 정렬
		{
			constexpr float RightPadding = 6.0f;
			constexpr float BetweenWidth = 24.0f;

			const float ContentRight = ImGui::GetWindowContentRegionMax().x;
			float TargetX = ContentRight - RightPadding - BetweenWidth;
			TargetX = std::max(TargetX, ImGui::GetCursorPosX());

			ImGui::SameLine();
			ImGui::SetCursorPosX(TargetX);
		}


		// 레이아웃 전환 버튼들 (커스텀 박스)
		const float layoutButtonSize = 24.0f;
		const float layoutIconSize = 16.0f;  // 아이콘 크기 조정

		if (ViewportManager.GetViewportLayout() == EViewportLayout::Single)
		{
			// Quad 아이콘 버튼
			if (IconQuad && IconQuad->GetTextureSRV())
			{
				ImVec2 layoutButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##QuadButton", ImVec2(layoutButtonSize, layoutButtonSize));
				bool bLayoutClicked = ImGui::IsItemClicked();
				bool bLayoutHovered = ImGui::IsItemHovered();

				// 버튼 배경 그리기
				ImDrawList* layoutDrawList = ImGui::GetWindowDrawList();
				ImU32 layoutBgColor = bLayoutHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
				if (ImGui::IsItemActive())
				{
					layoutBgColor = IM_COL32(38, 38, 38, 255);
				}
				layoutDrawList->AddRectFilled(layoutButtonPos, ImVec2(layoutButtonPos.x + layoutButtonSize, layoutButtonPos.y + layoutButtonSize), layoutBgColor, 4.0f);
				layoutDrawList->AddRect(layoutButtonPos, ImVec2(layoutButtonPos.x + layoutButtonSize, layoutButtonPos.y + layoutButtonSize), IM_COL32(96, 96, 96, 255), 4.0f);

				// 아이콘 그리기 (중앙 정렬)
				ImVec2 layoutIconPos = ImVec2(
					layoutButtonPos.x + (layoutButtonSize - layoutIconSize) * 0.5f,
					layoutButtonPos.y + (layoutButtonSize - layoutIconSize) * 0.5f
				);
				layoutDrawList->AddImage(
					(ImTextureID)IconQuad->GetTextureSRV(),
					layoutIconPos,
					ImVec2(layoutIconPos.x + layoutIconSize, layoutIconPos.y + layoutIconSize)
				);

				if (bLayoutClicked)
				{
					CurrentLayout = ELayout::Quad;
					ViewportManager.SetViewportLayout(EViewportLayout::Quad);
					ViewportManager.StartLayoutAnimation(true, ViewportIndex);
				}
			}
		}

		if (ViewportManager.GetViewportLayout() == EViewportLayout::Quad)
		{
			// Square 아이콘 버튼
			if (IconSquare && IconSquare->GetTextureSRV())
			{
				ImVec2 layoutButtonPos = ImGui::GetCursorScreenPos();
				ImGui::InvisibleButton("##SquareButton", ImVec2(layoutButtonSize, layoutButtonSize));
				bool bLayoutClicked = ImGui::IsItemClicked();
				bool bLayoutHovered = ImGui::IsItemHovered();

				// 버튼 배경 그리기
				ImDrawList* layoutDrawList = ImGui::GetWindowDrawList();
				ImU32 layoutBgColor = bLayoutHovered ? IM_COL32(26, 26, 26, 255) : IM_COL32(0, 0, 0, 255);
				if (ImGui::IsItemActive())
				{
					layoutBgColor = IM_COL32(38, 38, 38, 255);
				}
				layoutDrawList->AddRectFilled(layoutButtonPos, ImVec2(layoutButtonPos.x + layoutButtonSize, layoutButtonPos.y + layoutButtonSize), layoutBgColor, 4.0f);
				layoutDrawList->AddRect(layoutButtonPos, ImVec2(layoutButtonPos.x + layoutButtonSize, layoutButtonPos.y + layoutButtonSize), IM_COL32(96, 96, 96, 255), 4.0f);

				// 아이콘 그리기 (중앙 정렬)
				ImVec2 layoutIconPos = ImVec2(
					layoutButtonPos.x + (layoutButtonSize - layoutIconSize) * 0.5f,
					layoutButtonPos.y + (layoutButtonSize - layoutIconSize) * 0.5f
				);
				layoutDrawList->AddImage(
					(ImTextureID)IconSquare->GetTextureSRV(),
					layoutIconPos,
					ImVec2(layoutIconPos.x + layoutIconSize, layoutIconPos.y + layoutIconSize)
				);

				if (bLayoutClicked)
				{
					CurrentLayout = ELayout::Single;
					ViewportManager.SetViewportLayout(EViewportLayout::Single);
					// 스플리터 비율을 저장하고 애니메이션 시작: Quad → Single
					ViewportManager.PersistSplitterRatios();
					ViewportManager.StartLayoutAnimation(false, ViewportIndex);
				}
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(3);
	ImGui::PopID();
}



