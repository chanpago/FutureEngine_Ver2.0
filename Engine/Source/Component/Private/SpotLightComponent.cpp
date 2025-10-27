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
    FSpotLightInfo Info{};
    Info.Color = FVector4(LightColor, 1);
    Info.Position = GetWorldLocation();
    Info.Intensity = Intensity;
    Info.Range = AttenuationRadius;
    Info.DistanceFalloffExponent = DistanceFalloffExponent;
    Info.InnerConeAngle = InnerConeAngleRad;
    Info.OuterConeAngle = OuterConeAngleRad;
    Info.AngleFalloffExponent = AngleFalloffExponent;
    Info.Direction = GetForwardVector();

    // Build shadow view/proj for spotlight (row-vector LH)
    FVector Eye = Info.Position;
    FVector Fwd = Info.Direction.GetNormalized();
    FVector At = Eye + Fwd;
    FVector Up = (fabsf(Fwd.Z) > 0.99f) ? FVector(1,0,0) : FVector(0,0,1);

    auto NormalizeSafe = [](const FVector& v, float eps=1e-6f){ float len=v.Length(); return (len>eps)? (v/len):FVector(0,0,0); };
    FVector Forward = NormalizeSafe(At - Eye);
    if (fabsf(Forward.Dot(Up)) > 0.99f) Up = (fabsf(Forward.Z) > 0.9f) ? FVector(1,0,0) : FVector(0,0,1);
    FVector Right = NormalizeSafe(Up.Cross(Forward));
    Up = Forward.Cross(Right);

    FMatrix V = FMatrix::Identity();
    V.Data[0][0]=Right.X;  V.Data[0][1]=Up.X;   V.Data[0][2]=Forward.X;
    V.Data[1][0]=Right.Y;  V.Data[1][1]=Up.Y;   V.Data[1][2]=Forward.Y;
    V.Data[2][0]=Right.Z;  V.Data[2][1]=Up.Z;   V.Data[2][2]=Forward.Z;
    V.Data[3][0]= -Eye.Dot(Right);
    V.Data[3][1]= -Eye.Dot(Up);
    V.Data[3][2]= -Eye.Dot(Forward);
    V.Data[3][3]= 1.0f;

    float fovY = std::max(Info.OuterConeAngle * 2.0f, 0.1f);
    float aspect = 1.0f;
    float zn = 0.1f;
    float zf = std::max(Info.Range, 1.0f);

    FMatrix P = FMatrix::Identity();
    float yScale = 1.0f / tanf(fovY * 0.5f);
    float xScale = yScale / std::max(1e-6f, aspect);
    P.Data[0][0] = xScale;
    P.Data[1][1] = yScale;
    P.Data[2][2] = zf / (zf - zn);
    P.Data[3][2] = (-zn * zf) / (zf - zn);
    P.Data[2][3] = 1.0f;
    P.Data[3][3] = 0.0f;

    Info.LightView = V;
    Info.LightProj = P;
    return Info;
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
