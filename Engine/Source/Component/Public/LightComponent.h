#pragma once

#include "SceneComponent.h"
#include "LightComponentBase.h"
#include "Component/Public/BillBoardComponent.h"
#include "Manager/Asset/Public/AssetManager.h"

class UBillBoardComponent;

UENUM()
enum class ELightComponentType
{
    LightType_Directional = 0,
    LightType_Point       = 1,
    LightType_Spot        = 2,
    LightType_Ambient     = 3,
    LightType_Rect        = 4,
    LightType_Max         = 5
};
DECLARE_ENUM_REFLECTION(ELightComponentType)

UCLASS()
class ULightComponent : public ULightComponentBase
{
    GENERATED_BODY()
    DECLARE_CLASS(ULightComponent, ULightComponentBase)

public:
    ULightComponent() = default;

    virtual ~ULightComponent() = default;
    
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
    virtual void BeginPlay() override { Super::BeginPlay(); }
    
    virtual void TickComponent(float DeltaTime) override { Super::TickComponent(DeltaTime); }

    virtual void EndPlay() override { Super::EndPlay(); }

    /*-----------------------------------------------------------------------------
        ULightComponent Features
     -----------------------------------------------------------------------------*/
public:
    // --- Getters & Setters ---

    virtual ELightComponentType GetLightType() const { return ELightComponentType::LightType_Max; }

    // --- [UE Style] ---

    // virtual FBox GetBoundingBox() const;

    // virtual FSphere GetBoundingSphere() const;
    
    /** @note Sets the light intensity and clamps it to the same range as Unreal Engine (0.0 - 20.0). */

    void SetIntensity(float InIntensity) override;
    void SetLightColor(FVector InLightColor) override;

    virtual void EnsureVisualizationBillboard(){};

    UBillBoardComponent* GetBillBoardComponent() const
    {
        return VisualizationBillboard;
    }

    void SetBillBoardComponent(UBillBoardComponent* InBillBoardComponent)
    {
        VisualizationBillboard = InBillBoardComponent;
    }

    void RefreshVisualizationBillboardBinding();

    
protected:
    void UpdateVisualizationBillboardTint();

    UBillBoardComponent* VisualizationBillboard = nullptr;
private:
    float ShadowResolutionScale;
    float ShadowBias;
    float ShadowSlopeBias;
    float ShadowSharpen;
};
