#pragma once
#include "Core/Public/Object.h"

using std::chrono::high_resolution_clock;

UCLASS()
class UTimeManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UTimeManager, UObject)

public:
	void Update();

	// Getter & Setter
	float GetFPS() const { return 1 / DeltaTime; }
	float GetDeltaTime() const { return DeltaTime; }
	float GetGameTime() const { return GameTime; }
	bool IsPaused() const { return bIsPaused; }
	
	void SetDeltaTime(float InDeltaTime) { DeltaTime = InDeltaTime; }

	void PauseGame() { bIsPaused = true; }
	void ResumeGame() { bIsPaused = false; }

private:
	float GameTime;
	float DeltaTime;

	bool bIsPaused;

	void Initialize();
	void CalculateFPS();
};
