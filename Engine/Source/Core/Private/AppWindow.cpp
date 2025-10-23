#include "pch.h"
#include "Core/Public/AppWindow.h"
#include "Core/Public/resource.h"

#include "ImGui/imgui.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Render/Renderer/Public/Renderer.h"

FAppWindow::FAppWindow(FClientApp* InOwner)
	: Owner(InOwner), InstanceHandle(nullptr), MainWindowHandle(nullptr)
{
}

FAppWindow::~FAppWindow() = default;

bool FAppWindow::Init(HINSTANCE InInstance, int InCmdShow)
{
	InstanceHandle = InInstance;

	WCHAR WindowClass[] = L"UnlearnEngineWindowClass";

	// 아이콘 로드
	HICON hIcon = LoadIconW(InInstance, MAKEINTRESOURCEW(IDI_ICON1));
	HICON hIconSm = LoadIconW(InInstance, MAKEINTRESOURCEW(IDI_ICON1));

	WNDCLASSW wndclass = {};
	wndclass.style = 0;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = InInstance;
	wndclass.hIcon = hIcon; // 큰 아이콘 (타이틀바용)
	wndclass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wndclass.hbrBackground = nullptr;
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = WindowClass;

	RegisterClassW(&wndclass);

	MainWindowHandle = CreateWindowExW(0, WindowClass, L"",
	                                   WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
	                                   CW_USEDEFAULT, CW_USEDEFAULT,
	                                   Render::INIT_SCREEN_WIDTH, Render::INIT_SCREEN_HEIGHT,
	                                   nullptr, nullptr, InInstance, nullptr);

	if (!MainWindowHandle)
	{
		return false;
	}

	if (hIcon)
	{
		SendMessageW(MainWindowHandle, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSm));
		SendMessageW(MainWindowHandle, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
	}

	ShowWindow(MainWindowHandle, InCmdShow);
	UpdateWindow(MainWindowHandle);
	SetNewTitle(L"Future Engine");

	return true;
}

/**
 * @brief Initialize Console & Redirect IO
 * 현재 ImGui로 기능을 넘기면서 사용은 하지 않으나 코드는 유지
 */
void FAppWindow::InitializeConsole()
{
	// Error Handle
	if (!AllocConsole())
	{
		MessageBoxW(nullptr, L"콘솔 생성 실패", L"Error", 0);
	}

	// Console 출력 지정
	FILE* FilePtr;
	(void)freopen_s(&FilePtr, "CONOUT$", "w", stdout);
	(void)freopen_s(&FilePtr, "CONOUT$", "w", stderr);
	(void)freopen_s(&FilePtr, "CONIN$", "r", stdin);

	// Console Menu Setting
	HWND ConsoleWindow = GetConsoleWindow();
	HMENU MenuHandle = GetSystemMenu(ConsoleWindow, FALSE);
	if (MenuHandle != nullptr)
	{
		EnableMenuItem(MenuHandle, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		DeleteMenu(MenuHandle, SC_CLOSE, MF_BYCOMMAND);
	}
}

FAppWindow* FAppWindow::GetWindowInstance(HWND InWindowHandle, uint32 InMessage, LPARAM InLParam)
{
	if (InMessage == WM_NCCREATE)
	{
		CREATESTRUCT* CreateStruct = reinterpret_cast<CREATESTRUCT*>(InLParam);
		FAppWindow* WindowInstance = static_cast<FAppWindow*>(CreateStruct->lpCreateParams);
		SetWindowLongPtr(InWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(WindowInstance));

		return WindowInstance;
	}

	return reinterpret_cast<FAppWindow*>(GetWindowLongPtr(InWindowHandle, GWLP_USERDATA));
}

LRESULT CALLBACK FAppWindow::WndProc(HWND InWindowHandle, uint32 InMessage, WPARAM InWParam,
                                     LPARAM InLParam)
{
	if (UUIManager::WndProcHandler(InWindowHandle, InMessage, InWParam, InLParam))
	{
		if (ImGui::GetIO().WantCaptureMouse)
		{
			// 휠 메시지는 ImGui가 캡처해도 InputManager에 전달하여 카메라 속도 조절이 가능하도록 함
			if (InMessage == WM_MOUSEWHEEL)
			{
				UInputManager::GetInstance().ProcessKeyMessage(InMessage, InWParam, InLParam);
			}
			return true;
		}
	}

	UInputManager::GetInstance().ProcessKeyMessage(InMessage, InWParam, InLParam);

	switch (InMessage)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_ENTERSIZEMOVE: //드래그 시작
		URenderer::GetInstance().SetIsResizing(true);
		break;
	case WM_EXITSIZEMOVE: //드래그 종료
		{
			URenderer::GetInstance().SetIsResizing(false);
			RECT ClientRect{};
			if (GetClientRect(InWindowHandle, &ClientRect))
			{
				const uint32 NewWidth = static_cast<uint32>(ClientRect.right - ClientRect.left);
				const uint32 NewHeight = static_cast<uint32>(ClientRect.bottom - ClientRect.top);
				if (NewWidth > 0 && NewHeight > 0)
				{
					URenderer::GetInstance().OnResize(NewWidth, NewHeight);
					UUIManager::GetInstance().RepositionImGuiWindows();
				}
			}
			break;
		}
	case WM_SIZE:
		if (InWParam != SIZE_MINIMIZED)
		{
			if (!URenderer::GetInstance().GetIsResizing())
			{ // 드래그 X 일때 추가 처리 (최대화 버튼, ...)
				URenderer::GetInstance().OnResize(LOWORD(InLParam), HIWORD(InLParam));
				UUIManager::GetInstance().RepositionImGuiWindows();
			}
		}
		else // SIZE_MINIMIZED
		{
			// 윈도우가 최소화될 때 입력 비활성화 및 상태 저장
			UE_LOG("AppWindow: Window 최소화 (WM_SIZE - SIZE_MINIMIZED)");
			UInputManager::GetInstance().SetWindowFocus(false);
			UUIManager::GetInstance().OnWindowMinimized();
		}
		break;

	case WM_SETFOCUS:
		// 윈도우가 포커스를 얻었을 때 입력 활성화 및 상태 복원
		UE_LOG("AppWindow: Window 포커스 획득 (WM_SETFOCUS)");
		UInputManager::GetInstance().SetWindowFocus(true);
		UUIManager::GetInstance().OnWindowRestored();
		break;

	case WM_KILLFOCUS:
		// 윈도우가 포커스를 잃었을 때 입력 비활성화
		UInputManager::GetInstance().SetWindowFocus(false);
		break;

	case WM_ACTIVATE:
		// 윈도우 활성/비활성 상태 변화
		if (LOWORD(InWParam) == WA_INACTIVE)
		{
			// 윈도우가 비활성화될 때
			UInputManager::GetInstance().SetWindowFocus(false);
			// 주의: WM_ACTIVATE에서 OnWindowMinimized를 호출하지 않음 (최소화가 아닌 단순 비활성화)
		}
		else
		{
			// 윈도우가 활성화될 때 (WA_ACTIVE 또는 WA_CLICKACTIVE)
			UE_LOG("AppWindow: Window 활성화 (WM_ACTIVATE)");
			UInputManager::GetInstance().SetWindowFocus(true);
			UUIManager::GetInstance().OnWindowRestored();
		}
		break;

	default:
		return DefWindowProc(InWindowHandle, InMessage, InWParam, InLParam);
	}

	return 0;
}

void FAppWindow::SetNewTitle(const wstring& InNewTitle) const
{
	SetWindowTextW(MainWindowHandle, InNewTitle.c_str());
}

void FAppWindow::GetClientSize(int32& OutWidth, int32& OutHeight) const
{
	RECT ClientRectangle;
	GetClientRect(MainWindowHandle, &ClientRectangle);
	OutWidth = ClientRectangle.right - ClientRectangle.left;
	OutHeight = ClientRectangle.bottom - ClientRectangle.top;
}
