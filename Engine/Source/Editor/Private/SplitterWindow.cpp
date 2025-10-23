#include "pch.h"
#include "Editor/Public/SplitterWindow.h"

bool SWindow::IsHovered(const FPoint& InMousePosition) const
{
	return (InMousePosition.X >= Rect.Left &&
		InMousePosition.X <= Rect.GetRight() &&
		InMousePosition.Y >= Rect.Top &&
		InMousePosition.Y <= Rect.GetBottom());
}

void SSplitter::SetChildren(SWindow* InSideLT, SWindow* InSideRB)
{
	SideLT = InSideLT;
	SideRB = InSideRB;
}

void SSplitter::SetRatio(float NewRatio)
{
	if (NewRatio < CollapseThreshold)
	{
		CollapseState = ECollapseState::SideLT;
	}
	else if (NewRatio > (1.0f - CollapseThreshold))
	{
		CollapseState = ECollapseState::SideRB;
	}
	else
	{
		CollapseState = ECollapseState::Normal;
		Ratio = NewRatio;
	}
}

void SSplitterH::Resize(const FRect& ParentRect)
{
	FRect RectTop, RectBottom;

	// 접힘 상태에 따라 영역을 다르게 계산
	switch (CollapseState)
	{
	case ECollapseState::SideLT: // 위쪽 뷰포트가 접혔을 때
		// 스플리터는 상단에 작은 탭으로 표시
		Rect.Left = ParentRect.Left;
		Rect.Width = ParentRect.Width;
		Rect.Top = ParentRect.Top;
		Rect.Height = Thickness;

		// 위쪽 자식은 크기를 0으로 설정
		RectTop = { ParentRect.Left, ParentRect.Top, 0.0f, 0.0f };
		// 아래쪽 자식이 부모 영역 전체를 차지
		RectBottom = { ParentRect.Left, Rect.GetBottom(), ParentRect.Width, ParentRect.Height - Thickness };
		break;

	case ECollapseState::SideRB: // 아래쪽 뷰포트가 접혔을 때
		// 스플리터는 하단에 작은 탭으로 표시
		Rect.Left = ParentRect.Left;
		Rect.Width = ParentRect.Width;
		Rect.Top = ParentRect.GetBottom() - Thickness;
		Rect.Height = Thickness;

		// 위쪽 자식이 부모 영역 전체를 차지
		RectTop = { ParentRect.Left, ParentRect.Top, ParentRect.Width, ParentRect.Height - Thickness };
		// 아래쪽 자식은 크기를 0으로 설정
		RectBottom = { ParentRect.Left, Rect.GetBottom(), 0.0f, 0.0f };
		break;

	case ECollapseState::Normal: // 일반 상태
	default:
		Rect.Left = ParentRect.Left;
		Rect.Width = ParentRect.Width;
		Rect.Top = ParentRect.Top + (ParentRect.Height * Ratio);
		Rect.Height = Thickness;

		RectTop = { ParentRect.Left, ParentRect.Top, ParentRect.Width, Rect.Top - ParentRect.Top };
		RectBottom = { ParentRect.Left, Rect.GetBottom(), ParentRect.Width, ParentRect.GetBottom() - Rect.GetBottom() };
		break;
	}

	if (SideLT && SideRB)
	{
		SideLT->Resize(RectTop);
		SideRB->Resize(RectBottom);
	}
}

void SSplitterV::Resize(const FRect& ParentRect)
{
	FRect RectLeft, RectRight;

	// 접힘 상태에 따라 영역을 다르게 계산
	switch (CollapseState)
	{
	case ECollapseState::SideLT: // 왼쪽 뷰포트가 접혔을 때
		// 스플리터는 왼쪽에 작은 탭으로 표시
		Rect.Top = ParentRect.Top;
		Rect.Height = ParentRect.Height;
		Rect.Left = ParentRect.Left;
		Rect.Width = Thickness;

		// 왼쪽 자식은 크기를 0으로 설정
		RectLeft = { ParentRect.Left, ParentRect.Top, 0.0f, 0.0f };
		// 오른쪽 자식이 부모 영역 전체를 차지
		RectRight = { Rect.GetRight(), ParentRect.Top, ParentRect.Width - Thickness, ParentRect.Height };
		break;

	case ECollapseState::SideRB: // 오른쪽 뷰포트가 접혔을 때
		// 스플리터는 오른쪽에 작은 탭으로 표시
		Rect.Top = ParentRect.Top;
		Rect.Height = ParentRect.Height;
		Rect.Left = ParentRect.GetRight() - Thickness;
		Rect.Width = Thickness;

		// 왼쪽 자식이 부모 영역 전체를 차지
		RectLeft = { ParentRect.Left, ParentRect.Top, ParentRect.Width - Thickness, ParentRect.Height };
		// 오른쪽 자식은 크기를 0으로 설정
		RectRight = { Rect.GetRight(), ParentRect.Top, 0.0f, 0.0f };
		break;

	case ECollapseState::Normal: // 일반 상태 (기존 로직)
	default:
		Rect.Top = ParentRect.Top;
		Rect.Height = ParentRect.Height;
		Rect.Left = ParentRect.Left + (ParentRect.Width * Ratio);
		Rect.Width = Thickness;

		RectLeft = { ParentRect.Left, ParentRect.Top, Rect.Left - ParentRect.Left, ParentRect.Height };
		RectRight = { Rect.GetRight(), ParentRect.Top, ParentRect.GetRight() - Rect.GetRight(), ParentRect.Height };
		break;
	}

	if (SideLT && SideRB)
	{
		SideLT->Resize(RectLeft);
		SideRB->Resize(RectRight);
	}
}
