#include "pch.h"
#include "Component/Public/HeightFogComponent.h"
#include "Render/UI/Widget/Public/HeightFogComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"

IMPLEMENT_CLASS(UHeightFogComponent, USceneComponent)

UHeightFogComponent::UHeightFogComponent()
{
    bCanEverTick = false;
}

UHeightFogComponent::~UHeightFogComponent()
{
    
}

void UHeightFogComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
}

void UHeightFogComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    // 불러오기
    if (bInIsLoading)
    {
        FJsonSerializer::ReadVector(InOutHandle, "FogInScatteringColor", FogInScatteringColor, {0.5f, 0.5f, 0.5f});
        FJsonSerializer::ReadFloat(InOutHandle, "FogDensity", FogDensity, 0.05f);
        FJsonSerializer::ReadFloat(InOutHandle, "FogHeightFalloff", FogHeightFalloff, 0.01f);
        FJsonSerializer::ReadFloat(InOutHandle, "StartDistance", StartDistance, 1.5f);
        FJsonSerializer::ReadFloat(InOutHandle, "FogCutoffDistance", FogCutoffDistance, 50000.0f);
        FJsonSerializer::ReadFloat(InOutHandle, "FogMaxOpacity", FogMaxOpacity, 0.98f);
    }
    // 저장
    else
    {
        InOutHandle["FogInScatteringColor"] = FJsonSerializer::VectorToJson(FogInScatteringColor);
        InOutHandle["FogDensity"] = FogDensity;
        InOutHandle["FogHeightFalloff"] = FogHeightFalloff;
        InOutHandle["StartDistance"] = StartDistance;
        InOutHandle["FogCutoffDistance"] = FogCutoffDistance;
        InOutHandle["FogMaxOpacity"] = FogMaxOpacity;
    }
}

UClass* UHeightFogComponent::GetSpecificWidgetClass() const
{
	return UHeightFogComponentWidget::StaticClass();
}

UObject* UHeightFogComponent::Duplicate()
{
    UHeightFogComponent* HeightFogComponent = Cast<UHeightFogComponent>(Super::Duplicate());
    
    HeightFogComponent->FogDensity = FogDensity;
    HeightFogComponent->FogHeightFalloff = FogHeightFalloff;
    HeightFogComponent->StartDistance = StartDistance;
    HeightFogComponent->FogCutoffDistance = FogCutoffDistance;
    HeightFogComponent->FogMaxOpacity = FogMaxOpacity;
    HeightFogComponent->FogInScatteringColor = FogInScatteringColor;
    HeightFogComponent->bVisible = bVisible;  // 가시성 상태 복사

    return HeightFogComponent;
}
