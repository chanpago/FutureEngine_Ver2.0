#pragma once
#include "Core/Public/Object.h"

class FAppWindow;

UCLASS()
class UInputManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UInputManager, UObject)

public:
	void Update(const FAppWindow* InWindow);
	void UpdateMousePosition(const FAppWindow* InWindow);
	void ProcessKeyMessage(uint32 InMessage, WPARAM WParam, LPARAM LParam);

	bool IsKeyDown(EKeyInput InKey) const;
	bool IsKeyPressed(EKeyInput InKey) const;
	bool IsKeyReleased(EKeyInput InKey) const;

	// Key Status
	EKeyStatus GetKeyStatus(EKeyInput InKey) const;
	const TArray<EKeyInput>& GetKeysByStatus(EKeyStatus InStatus);

	const TArray<EKeyInput>& GetPressedKeys();
	const TArray<EKeyInput>& GetNewlyPressedKeys();
	const TArray<EKeyInput>& GetReleasedKeys();

	// Window Focus Management
	void SetWindowFocus(bool bInFocused);
	bool IsWindowFocused() const { return bIsWindowFocused; }

	// Helper Function
	static FString KeyInputToString(EKeyInput InKey);

	// Mouse Wheel
	float GetMouseWheelDelta() const { return MouseWheelDelta; }

	// Double Click Detection
	bool IsMouseDoubleClicked(EKeyInput InMouseButton) const;

	// Getter
	const FVector& GetMouseNDCPosition() const { return NDCMousePosition; }
	const FVector& GetMousePosition() const { return CurrentMousePosition; }
	const FVector& GetMouseDelta() const { return MouseDelta; }

	void ClearMouseWheelDelta()
	{
		MouseWheelDelta = 0.0f;
	}
private:
	// Key Status
	TMap<EKeyInput, bool> CurrentKeyState;
	TMap<EKeyInput, bool> PreviousKeyState;
	TMap<int32, EKeyInput> VirtualKeyMap;
	TArray<EKeyInput> KeysInStatus;

	// Mouse Position
	FVector CurrentMousePosition;
	FVector PreviousMousePosition;
	FVector MouseDelta;

	// Mouse Wheel
	float MouseWheelDelta;

	// NDC Mouse Position
	FVector NDCMousePosition;

	// Window Focus
	bool bIsWindowFocused;

	// Double Click Detection
	float DoubleClickTime;
	TMap<EKeyInput, float> LastClickTime;
	TMap<EKeyInput, bool> DoubleClickState;
	TMap<EKeyInput, int> ClickCount;

	void InitializeKeyMapping();
	void InitializeMouseClickStatus();
	void UpdateDoubleClickDetection();
	void HandleConsoleShortcut();
};
