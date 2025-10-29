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

    void SetShadowwResolutionScale(float InShadowResolutionScale) { ShadowResolutionScale = InShadowResolutionScale; }
    void SetShadowBias(float InShadowBias) { ShadowBias = InShadowBias; }
    void SetShadowSlopeBias(float InShadowSlopeBias) { ShadowSlopeBias = InShadowSlopeBias; }
    void SetShadowSharpen(float InShadowSharpen) { ShadowSharpen = InShadowSharpen; }

    float GetShadowResolutionScale() const{ return ShadowResolutionScale; }
    float GetShadowBias() const  { return ShadowBias; }
    float GetShadowSlopeBias() const { return ShadowSlopeBias;}
    float GetShadowSharpen() const { return ShadowSharpen; }

protected:
    void UpdateVisualizationBillboardTint();

    UBillBoardComponent* VisualizationBillboard = nullptr;

    /** Shadow map resolution scale (0.25 = 25%, 1.0 = 100%, 2.0 = 200%) */
    float ShadowResolutionScale = 1.0f;
    /** Shadow depth bias to reduce shadow acne */
    float ShadowBias = 0.001f;
    /** Shadow slope bias (used with PSM) */
    float ShadowSlopeBias = 0.1f;
    /** Shadow edge sharpening factor */
    float ShadowSharpen = 0.0f;
private:
    
};
