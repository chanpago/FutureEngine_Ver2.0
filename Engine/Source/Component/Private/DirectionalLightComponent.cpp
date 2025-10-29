#include "pch.h"

#include "Component/Public/DirectionalLightComponent.h"
#include "Render/UI/Widget/Public/DirectionalLightComponentWidget.h"
#include "Utility/Public/JsonSerializer.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Component/Public/ActorComponent.h"
#include "Editor/Public/EditorEngine.h"
#include "Actor/Public/Actor.h"
#include "Level/Public/World.h"

IMPLEMENT_CLASS(UDirectionalLightComponent, ULightComponent)

namespace
{
    constexpr float GizmoScaleFactor = 1.0f / 3.0f;
    constexpr float BaseArrowLength = 18.0f;
    constexpr float BaseArrowThickness = 15.0f;
}

UDirectionalLightComponent::UDirectionalLightComponent()
{
    Intensity = 3.0f;

    ShadowBias = 0.002f;       // 0.001 → 0.002 (shadow acne 방지)
    ShadowSlopeBias = 0.05f;   // 0.01 → 0.05 (adaptive bias 강화)

    UAssetManager& ResourceManager = UAssetManager::GetInstance();

    // 화살표 프리미티브 설정 (빛 방향 표시용)
    LightDirectionArrow.InputLayout = URenderer::GetInstance().GetGizmoInputLayout();
    LightDirectionArrow.VertexShader = URenderer::GetInstance().GetGizmoVertexShader();
    LightDirectionArrow.PixelShader = URenderer::GetInstance().GetGizmoPixelShader();
    LightDirectionArrow.VertexBuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Arrow);
    LightDirectionArrow.NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Arrow);
    LightDirectionArrow.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    LightDirectionArrow.Color = FVector4(0.95f, 0.95f, 0.95f, 1.0f);
    LightDirectionArrow.Scale = FVector(BaseArrowLength * GizmoScaleFactor, BaseArrowThickness * GizmoScaleFactor, BaseArrowThickness * GizmoScaleFactor);
    LightDirectionArrow.RenderState.CullMode = ECullMode::None;
    LightDirectionArrow.RenderState.FillMode = EFillMode::Solid;
    LightDirectionArrow.bShouldAlwaysVisible = true;

    //EnsureVisualizationBillboard();
}

void UDirectionalLightComponent::BeginPlay()
{
    Super::BeginPlay();
    
}

void UDirectionalLightComponent::EndPlay()
{
    Super::EndPlay();
    VisualizationBillboard = nullptr;
}

void UDirectionalLightComponent::EnsureVisualizationBillboard()
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
    Billboard->SetSprite(UAssetManager::GetInstance().LoadTexture("Data/Icons/S_LightDirectional.png"));
    Billboard->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));
    Billboard->SetScreenSizeScaled(true);

    VisualizationBillboard = Billboard;
    UpdateVisualizationBillboardTint();
}

void UDirectionalLightComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    
}

UObject* UDirectionalLightComponent::Duplicate()
{
    UDirectionalLightComponent* DirectionalLightComponent = Cast<UDirectionalLightComponent>(Super::Duplicate());
    return DirectionalLightComponent;
}

void UDirectionalLightComponent::DuplicateSubObjects(UObject* DuplicatedObject)
{
    Super::DuplicateSubObjects(DuplicatedObject);
}

UClass* UDirectionalLightComponent::GetSpecificWidgetClass() const
{
    return UDirectionalLightComponentWidget::StaticClass();
}

FVector UDirectionalLightComponent::GetForwardVector() const
{
    FQuaternion LightRotation = GetWorldRotationAsQuaternion();

    FQuaternion ArrowToNegZ = FQuaternion::FromAxisAngle(FVector::RightVector(), -90.0f * (PI / 180.0f));
    FQuaternion FinalRotation = ArrowToNegZ * LightRotation;

    return FinalRotation.RotateVector(FVector::ForwardVector());
}

void UDirectionalLightComponent::RenderLightDirectionGizmo(UCamera* InCamera)
{
    if (!InCamera) return;

    FVector LightLocation = GetWorldLocation();
    FQuaternion LightRotation = GetWorldRotationAsQuaternion();

    float Distance = (InCamera->GetLocation() - LightLocation).Length();
    float Scale = Distance * 0.2f;
    if (Distance < 7.0f) Scale = 7.0f * 0.2f;

    FQuaternion ArrowToNegZ = FQuaternion::FromAxisAngle(FVector::RightVector(), -90.0f * (PI / 180.0f));
    FQuaternion FinalRotation = ArrowToNegZ * LightRotation;

    LightDirectionArrow.Location = LightLocation;
    LightDirectionArrow.Rotation = FinalRotation;
    //LightDirectionArrow.Scale = FVector(Scale, Scale, Scale);

    FRenderState RenderState;
    RenderState.FillMode = EFillMode::Solid;
    RenderState.CullMode = ECullMode::Front;

    URenderer::GetInstance().RenderEditorPrimitive(LightDirectionArrow, RenderState);
}

FDirectionalLightInfo UDirectionalLightComponent::GetDirectionalLightInfo() const
{
    return FDirectionalLightInfo{ FVector4(LightColor, 1), GetForwardVector(), Intensity,
        ResolutionScale, Bias, SlopeBias, Sharpen };
}
