#pragma once
#include "Core/Public/Class.h"       // UObject 기반 클래스 및 매크로
#include "Source/Component/Public/PrimitiveComponent.h"

UCLASS();
class UMeshComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UMeshComponent, UPrimitiveComponent)
};
