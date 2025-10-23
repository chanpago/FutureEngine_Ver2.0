#pragma once
#include "Widget.h"

class UActorSpawnWidget	: public UWidget
{
	DECLARE_CLASS(UActorSpawnWidget, UWidget)
	
public:
	UActorSpawnWidget();
	~UActorSpawnWidget() override;
	
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void SpawnActors();

private:
	void LoadActorClasses();

	TArray<FString> ActorClassNames;
	TArray<UClass*> ActorClasses;
	FName SelectedActorClassName = FName::None;
	int32 SelectedActorClassIndex = 0;
	int32 NumberOfSpawn = 1;
	float SpawnRangeMin = -10.0f;
	float SpawnRangeMax = 10.0f;
};
