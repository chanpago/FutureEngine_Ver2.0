#pragma once

#include "LightComponent.h"
#include "Editor/Public/EditorPrimitive.h"

namespace json { class JSON; }

struct FEditorPrimitive;
class UCamera;
class UBillBoardComponent;

UCLASS()
class UDirectionalLightComponent : public ULightComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UDirectionalLightComponent, ULightComponent)

public:
    UDirectionalLightComponent();
    virtual ~UDirectionalLightComponent() = default;

    /*-----------------------------------------------------------------------------
        UObject Features
     -----------------------------------------------------------------------------*/
public:
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    virtual UObject* Duplicate() override;

    virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

    /*-----------------------------------------------------------------------------
        UActorComponent Features
     -----------------------------------------------------------------------------*/
public:
    virtual void BeginPlay() override;
    virtual void EndPlay() override;

    virtual UClass* GetSpecificWidgetClass() const override;

    /*-----------------------------------------------------------------------------
        ULightComponent Features
     -----------------------------------------------------------------------------*/
public:
    virtual ELightComponentType GetLightType() const override { return ELightComponentType::LightType_Directional; }

    /*-----------------------------------------------------------------------------
        UDirectionalLightComponent Features
     -----------------------------------------------------------------------------*/
public:
    FVector GetForwardVector() const;
    void RenderLightDirectionGizmo(UCamera* InCamera);
    FDirectionalLightInfo GetDirectionalLightInfo() const;

    float GetCSMLambda() { return CSMLambda; }
    void SetCSMLambda(float InLambda) { CSMLambda = InLambda; }

private:
    void EnsureVisualizationBillboard()override;

private:
    FEditorPrimitive LightDirectionArrow;
    float CSMLambda = 0.8f;
};

