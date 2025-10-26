#include "pch.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Camera.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Actor/Public/Actor.h"
#include "Editor/Public/Editor.h"
#include "Global/Quaternion.h"

UGizmo::UGizmo()
{
	UAssetManager& ResourceManager = UAssetManager::GetInstance();
	Primitives.resize(3);
	GizmoColor.resize(3);

	/* *
	* @brief 0: Forward(x), 1: Right(y), 2: Up(z)
	*/
	GizmoColor[0] = FVector4(1, 0, 0, 1);
	GizmoColor[1] = FVector4(0, 1, 0, 1);
	GizmoColor[2] = FVector4(0, 0, 1, 1);

	/* *
	* @brief Translation Setting
	*/
	const float ScaleT = TranslateCollisionConfig.Scale;
	Primitives[0].VertexBuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Arrow);
	Primitives[0].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Arrow);
	Primitives[0].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[0].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[0].bShouldAlwaysVisible = true;

	/* *
	* @brief Rotation Setting
	*/
	Primitives[1].VertexBuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::Ring);
	Primitives[1].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::Ring);
	Primitives[1].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[1].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[1].bShouldAlwaysVisible = true;

	/* *
	* @brief Scale Setting
	*/
	Primitives[2].VertexBuffer = ResourceManager.GetVertexbuffer(EPrimitiveType::CubeArrow);
	Primitives[2].NumVertices = ResourceManager.GetNumVertices(EPrimitiveType::CubeArrow);
	Primitives[2].Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	Primitives[2].Scale = FVector(ScaleT, ScaleT, ScaleT);
	Primitives[2].bShouldAlwaysVisible = true;

	/* *
	* @brief Render State
	*/
	RenderState.FillMode = EFillMode::Solid;
	RenderState.CullMode = ECullMode::None;
}

UGizmo::~UGizmo() = default;

void UGizmo::UpdateScale(UCamera* InCamera)
{
	TargetComponent = Cast<USceneComponent>(GEditor->GetEditorModule()->GetSelectedComponent());
	if (!TargetComponent || !InCamera) { return; }

	float Scale;
	if (InCamera->GetCameraType() == ECameraType::ECT_Perspective)
	{
		float DistanceToCamera = (InCamera->GetLocation() - TargetComponent->GetWorldLocation()).Length();
		Scale = DistanceToCamera * ScaleFactor;
		if (DistanceToCamera < MinScaleFactor)
			Scale = MinScaleFactor * ScaleFactor;
	}
	else // Orthographic
	{
		Scale = OrthoScaleFactor;
	}

	TranslateCollisionConfig.Scale = Scale;
	RotateCollisionConfig.Scale = Scale;
}

void UGizmo::RenderGizmo(UCamera* InCamera)
{
	TargetComponent = Cast<USceneComponent>(GEditor->GetEditorModule()->GetSelectedComponent());
	if (!TargetComponent || !InCamera) { return; }

	float RenderScale;
	if (InCamera->GetCameraType() == ECameraType::ECT_Perspective)
	{
		float DistanceToCamera = (InCamera->GetLocation() - TargetComponent->GetWorldLocation()).Length();
		RenderScale = DistanceToCamera * ScaleFactor;
		if (DistanceToCamera < MinScaleFactor)
			RenderScale = MinScaleFactor * ScaleFactor;
	}
	else // Orthographic
	{
		RenderScale = OrthoScaleFactor;
	}

	URenderer& Renderer = URenderer::GetInstance();
	const int Mode = static_cast<int>(GizmoMode);
	auto& P = Primitives[Mode];
	P.Location = TargetComponent->GetWorldLocation();

	P.Scale = FVector(RenderScale, RenderScale, RenderScale);

	FQuaternion LocalRot;
	if (GizmoMode == EGizmoMode::Scale)
	{
		LocalRot = TargetComponent->GetWorldRotationAsQuaternion();
	}
	else
	{
		LocalRot = bIsWorld ? FQuaternion::Identity() : TargetComponent->GetWorldRotationAsQuaternion();
	}
	FVector LocalRotEuler = LocalRot.ToEuler();
	
	// X축 (Forward) - 빨간색
	P.Rotation = FQuaternion::Identity() * LocalRot;
	P.Color = ColorFor(EGizmoDirection::Forward);
	Renderer.RenderEditorPrimitive(P, RenderState);
	
	// Y축 (Right) - 초록색 (Z축 주위로 -90도 회전)
	P.Rotation =  FQuaternion::FromAxisAngle(FVector::UpVector(), -90.0f * (PI / 180.0f)) * LocalRot;
	P.Color = ColorFor(EGizmoDirection::Right);
	Renderer.RenderEditorPrimitive(P, RenderState);
	
	// Z축 (Up) - 파란색 (Y축 주위로 90도 회전)
	P.Rotation =  FQuaternion::FromAxisAngle(FVector::RightVector(), 90.0f * (PI / 180.0f)) * LocalRot;
	P.Color = ColorFor(EGizmoDirection::Up);
	Renderer.RenderEditorPrimitive(P, RenderState);
}

void UGizmo::ChangeGizmoMode()
{
	switch (GizmoMode)
	{
	case EGizmoMode::Translate:
		GizmoMode = EGizmoMode::Rotate; break;
	case EGizmoMode::Rotate:
		GizmoMode = EGizmoMode::Scale; break;
	case EGizmoMode::Scale:
		GizmoMode = EGizmoMode::Translate;
	}
}

void UGizmo::SetLocation(const FVector& Location)
{
	TargetComponent->SetWorldLocation(Location);
}

bool UGizmo::IsInRadius(float Radius)
{
	if (Radius >= RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale && Radius <= RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale)
		return true;
	return false;
}

void UGizmo::OnMouseDragStart(FVector& CollisionPoint)
{
	bIsDragging = true;
	DragStartMouseLocation = CollisionPoint;

	if (TargetComponent)
	{
		DragStartActorLocation = TargetComponent->GetWorldLocation();
		DragStartActorRotation = TargetComponent->GetWorldRotation();
		DragStartActorScale = TargetComponent->GetWorldScale3D();
	}
}

// 하이라이트 색상은 렌더 시점에만 계산 (상태 오염 방지)
FVector4 UGizmo::ColorFor(EGizmoDirection InAxis) const
{
	const int Idx = AxisIndex(InAxis);
	//UE_LOG("%d", Idx);
	const FVector4& BaseColor = GizmoColor[Idx];
	const bool bIsHighlight = (InAxis == GizmoDirection);

	const FVector4 Paint = bIsHighlight ? FVector4(1,1,0,1) : BaseColor;
	//UE_LOG("InAxis: %d, Idx: %d, Dir: %d, base color: %.f, %.f, %.f, bHighLight: %d", InAxis, Idx, GizmoDirection, BaseColor.X, BaseColor.Y, BaseColor.Z, bIsHighlight);

	if (bIsDragging)
		return BaseColor;
	else
		return Paint;
}
