#pragma once
#include "Render/UI/Viewport/Public/Window.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"

class FViewport
{
public:
	FViewport();
	virtual ~FViewport();

public:

	// ----- Client -----
	void SetViewportClient(FViewportClient* InClient) { ViewportClient = InClient; }
	FViewportClient* GetViewportClient() const { return ViewportClient; }

	// Geometry
	void SetSize(uint32 InSizeX, uint32 InSizeY) { { Rect.Width = (LONG)InSizeX; Rect.Height = (LONG)InSizeY; } }
	void SetInitialPosition(uint32 InPosX, uint32 InPosY) { { Rect.Left = (LONG)InPosX; Rect.Top = (LONG)InPosY; } }
	void SetRect(const FRect& InRect) { Rect = InRect; }

	uint32       GetSizeX() const { return (uint32)max<LONG>(0, Rect.Width); }
	uint32       GetSizeY() const { return (uint32)max<LONG>(0, Rect.Height); }
	
	FRect GetRect()  const { return Rect; }

	// 보조
	float  GetAspect()  const { return (GetSizeY() > 0) ? float(GetSizeX()) / float(GetSizeY()) : 1.0f; }

	// 좌표 변환
	FORCEINLINE FPoint ToLocal(const FPoint& InScreen) const
	{
		return FPoint{ InScreen.X - Rect.Left, InScreen.Y - Rect.Width };
	}
	FORCEINLINE FPoint ToScreen(const FPoint& InLocal) const
	{
		return FPoint{ InLocal.X + Rect.Left, InLocal.Y + Rect.Width };
	}

	// ----- Input: 좌표는 "로컬(뷰) 공간" 기대 -----
	bool HandleMouseDown(int32 InButton, int32 InLocalX, int32 InLocalY);
	bool HandleMouseUp(int32 InButton, int32 InLocalX, int32 InLocalY);
	bool HandleMouseMove(int32 InLocalX, int32 InLocalY);
	bool HandleCapturedMouseMove(int32 InLocalX, int32 InLocalY);

	// InputManager로부터 현재 마우스 상태를 읽어 이 뷰포트 기준으로 라우팅
	// - 화면 좌표를 로컬로 변환하여 MouseMove/CapturedMouseMove, Down/Up을 전달
	void PumpMouseFromInputManager();

	// ----- D3D11 RS 바인딩 (옵션) -----
	void ApplyRasterizer(ID3D11DeviceContext* InDeviceContext) const
	{
		D3D11_VIEWPORT ViewPort{};
		ViewPort.TopLeftX = (FLOAT)Rect.Left;
		ViewPort.TopLeftY = (FLOAT)Rect.Top + (FLOAT)ToolBarLayPx; // 툴바 제외하고 그릴 때
		ViewPort.Width = (FLOAT)max<LONG>(0, Rect.Width);
		ViewPort.Height = (FLOAT)max<LONG>(0, Rect.Height - ToolBarLayPx);
		ViewPort.MinDepth = 0.0f; ViewPort.MaxDepth = 1.0f;
		InDeviceContext->RSSetViewports(1, &ViewPort);

		D3D11_RECT Sc{ Rect.Left, Rect.Top + ToolBarLayPx, Rect.Left + Rect.Width, Rect.Top + Rect.Height };
		InDeviceContext->RSSetScissorRects(1, &Sc);
	}

	void SetToolbarHeight(int32 InPx) { ToolBarLayPx = max(0, InPx); }
	int32 GetToolbarHeight() const { return ToolBarLayPx; }

	void SetRenderRect(D3D11_VIEWPORT InViewport)
	{
		RenderRect = InViewport;
	}

	D3D11_VIEWPORT GetRenderRect() const { return RenderRect; }
	
private:
	FViewportClient* ViewportClient = nullptr;
	FRect  Rect{ 0,0,0,0 };
	FPoint LastMousePos{ 0,0 };
	int32  ToolBarLayPx = 0;
	D3D11_VIEWPORT RenderRect;
	// 이 뷰포트가 현재 마우스 캡처(드래그) 중인지 여부
	bool   bCapturing = false;
};
