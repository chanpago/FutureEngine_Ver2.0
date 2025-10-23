#pragma once

#include "Component/Public/LightComponent.h"

UCLASS()
class UAmbientLightComponent : public ULightComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UAmbientLightComponent, ULightComponent)

public:
    UAmbientLightComponent()
    {
        Intensity = 0.05f;
    }

    virtual ~UAmbientLightComponent() = default;

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
    virtual ELightComponentType GetLightType() const override { return ELightComponentType::LightType_Ambient; }
    /*-----------------------------------------------------------------------------
        UAmbientLightComponent Features
     -----------------------------------------------------------------------------*/

    FAmbientLightInfo GetAmbientLightInfo() const;

private:
    void EnsureVisualizationBillboard()override;
};