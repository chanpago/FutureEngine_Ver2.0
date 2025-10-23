#pragma once

#include "LightComponent.h"

UCLASS()
class UPointLightComponent : public ULightComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UPointLightComponent, ULightComponent)

public:
    UPointLightComponent() { Intensity = 3.0f; }

    virtual ~UPointLightComponent() = default;
    
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

    virtual UClass* GetSpecificWidgetClass() const override; 

    /*-----------------------------------------------------------------------------
        ULightComponent Features
     -----------------------------------------------------------------------------*/
public:
    virtual ELightComponentType GetLightType() const override { return ELightComponentType::LightType_Point; }

    /*-----------------------------------------------------------------------------
        UPointLightComponent Features
     -----------------------------------------------------------------------------*/
public:
    // --- Getters & Setters ---

    float GetDistanceFalloffExponent() const { return DistanceFalloffExponent; }

    float GetAttenuationRadius() const { return AttenuationRadius;}
    
    /** @note Sets the light falloff exponent and clamps it to the same range as Unreal Engine (2.0 - 16.0). */
    void SetDistanceFalloffExponent(float InDistanceFalloffExponent) { DistanceFalloffExponent = std::clamp(InDistanceFalloffExponent, 0.0f, 16.0f); } // 이지만, 최소 0이 되도록 해보자
    
    void SetAttenuationRadius(float InAttenuationRadius) { AttenuationRadius = InAttenuationRadius; }

    FPointLightInfo GetPointlightInfo() const;

private:
    void EnsureVisualizationBillboard()override;
    
protected:
    /**
     * 작을수록 attenuation이 선형에 가깝게 완만하게 감소하고,
     * 커질수록 중심 인근에서는 거의 감소하지 않다가 가장자리 부근에서 급격히 감소합니다.
     */
    float DistanceFalloffExponent = 2.0f;
    
    /** Radius of light source shape. */
    float AttenuationRadius = 10.0f;
};