#include "pch.h"
#include "Component/Public/SpotLightComponent.h"
#include "Render/UI/Widget/Public/SpotLightComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/EditorPrimitive.h"

IMPLEMENT_CLASS(USpotLightComponent, UPointLightComponent)

void USpotLightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    if (bInIsLoading)
    {
        FJsonSerializer::ReadFloat(InOutHandle, "AngleFalloffExponent", AngleFalloffExponent);
        SetAngleFalloffExponent(AngleFalloffExponent); // clamping을 위해 Setter 사용
        FJsonSerializer::ReadFloat(InOutHandle, "AttenuationAngle", OuterConeAngleRad);
    }
    else
    {
        InOutHandle["AngleFalloffExponent"] = AngleFalloffExponent;
        InOutHandle["AttenuationAngle"] = OuterConeAngleRad;
    }
}

UObject* USpotLightComponent::Duplicate()
{
    USpotLightComponent* NewSpotLightComponent = Cast<USpotLightComponent>(Super::Duplicate());
    NewSpotLightComponent->SetAngleFalloffExponent(AngleFalloffExponent);
    NewSpotLightComponent->SetOuterAngle(OuterConeAngleRad);

    return NewSpotLightComponent;
}

void USpotLightComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
    Super::DuplicateSubObjects(DuplicatedObject);
}

UClass* USpotLightComponent::GetSpecificWidgetClass() const
{
    return USpotLightComponentWidget::StaticClass();
}

FVector USpotLightComponent::GetForwardVector() const
{
    FQuaternion Rotation = GetWorldRotationAsQuaternion();
    return Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
}

void USpotLightComponent::SetOuterAngle(float const InAttenuationAngleRad)
{
    OuterConeAngleRad = std::clamp(InAttenuationAngleRad, 0.0f, PI/2.0f - MATH_EPSILON);
    InnerConeAngleRad = std::min(InnerConeAngleRad, OuterConeAngleRad);
}

void USpotLightComponent::SetInnerAngle(float const InAttenuationAngleRad)
{
    InnerConeAngleRad = std::clamp(InAttenuationAngleRad, 0.0f, OuterConeAngleRad);
}

void USpotLightComponent::RenderLightDirectionGizmo(UCamera* InCamera)
{
}
FSpotLightInfo USpotLightComponent::GetSpotLightInfo() const
{
    return FSpotLightInfo{ FVector4(LightColor, 1), GetWorldLocation(), Intensity, AttenuationRadius, DistanceFalloffExponent,
    InnerConeAngleRad, OuterConeAngleRad, AngleFalloffExponent, GetForwardVector() };
}

void USpotLightComponent::EnsureVisualizationBillboard()
{
    if (VisualizationBillboard)
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    if (GWorld)
    {
        EWorldType WorldType = GWorld->GetWorldType();
        if (WorldType != EWorldType::Editor && WorldType != EWorldType::EditorPreview)
        {
            return;
        }
    }

    UBillBoardComponent* Billboard = OwnerActor->AddComponent<UBillBoardComponent>();
    if (!Billboard)
    {
        return;
    }
    Billboard->AttachToComponent(this);
    Billboard->SetIsVisualizationComponent(true);
    Billboard->SetSprite(UAssetManager::GetInstance().LoadTexture("Data/Icons/S_LightSpot.png"));
    Billboard->SetRelativeScale3D(FVector(2.f,2.f,2.f));
    Billboard->SetScreenSizeScaled(true);

    VisualizationBillboard = Billboard;
    UpdateVisualizationBillboardTint();
}
