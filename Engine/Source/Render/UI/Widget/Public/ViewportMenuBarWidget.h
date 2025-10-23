#pragma once
#include "Widget.h"

class FViewport;
class UEditor;
class UTexture;

/**
 * @brief 레거시 위젯 - ViewportControlWidget으로 교체 권장
 * @deprecated 이 위젯은 FutureEngine 철학에 맞게 수정되었지만,
 *             ViewportControlWidget이 더 나은 디자인과 기능을 제공합니다.
 * 
 * FutureEngine 철학:
 *   ViewportManager -> Viewport -> ViewportClient -> Camera
 *   - Viewport가 ViewportClient를 소유
 *   - ViewportClient가 Camera를 관리
 */
class UViewportMenuBarWidget : public UWidget
{
public:
	UViewportMenuBarWidget();
	virtual ~UViewportMenuBarWidget() override;

	void Initialize() override {}
	void Update() override {}
	void RenderWidget() override;

	// 레거시 호환성을 위해 유지
	void SetViewportClient(FViewport* InViewportClient) { Viewport = InViewportClient; }

private:
	void RenderCameraControls(UCamera& InCamera);
	void LoadViewIcons(); // 아이콘 로드 함수

	FViewport* Viewport = nullptr; // 레거시 참조
	UEditor* Editor = nullptr; // 레거시 에디터 참조

	bool bIsSingleViewportClient = true;

	// View Type 아이콘들
	UTexture* IconPerspective = nullptr;
	UTexture* IconTop = nullptr;
	UTexture* IconBottom = nullptr;
	UTexture* IconLeft = nullptr;
	UTexture* IconRight = nullptr;
	UTexture* IconFront = nullptr;
	UTexture* IconBack = nullptr;
	bool bIconsLoaded = false;
};

