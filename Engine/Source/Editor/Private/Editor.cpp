#include "pch.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/Camera.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Level/Public/Level.h"
#include "Global/Quaternion.h"
#include "Utility/Public/ScopeCycleCounter.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Component/Public/DecalSpotLightComponent.h"
#include "Component/Public/PointLightComponent.h"
#include "Physics/Public/BoundingSphere.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Component/Public/SpotLightComponent.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/ui/Viewport/Public/ViewportClient.h"
#include "Render/UI/Viewport/Public/Viewport.h"

IMPLEMENT_CLASS(UEditor, UObject)

UEditor::UEditor()
{
	int32 ActiveIndex = UViewportManager::GetInstance().GetActiveIndex();
	ActiveViewportIndex = ActiveIndex;
	Camera = UViewportManager::GetInstance().GetClients()[ActiveIndex]->GetCamera();
}

UEditor::~UEditor()
{
	// 레거시 코드 제거: ViewportManager가 뷰포트 관리
}

void UEditor::Update()
{
	UViewportManager& ViewportManager = UViewportManager::GetInstance();
	const UInputManager& Input = UInputManager::GetInstance();

	// 활성 카메라 업데이트
	int32 ActiveIndex = ViewportManager.GetActiveIndex();
	if (ActiveViewportIndex != ActiveIndex)
	{
		ActiveViewportIndex = ActiveIndex;
		Camera = ViewportManager.GetClients()[ActiveIndex]->GetCamera();
	}

	// KTLWeek07: 각 뷰포트에서 마우스 우클릭 시 해당 카메라만 입력 활성화
	int32 HoveredViewportIndex = ViewportManager.GetViewportIndexUnderMouse();
	bool bIsRightMouseDown = Input.IsKeyDown(EKeyInput::MouseRight);
	
	if (ViewportManager.GetViewportLayout() == EViewportLayout::Quad)
	{
		// Quad 모드: 마우스 우클릭 중이고 해당 뷰포트 위에 있을 때만 그 카메라 입력 활성화
		
		for (int32 i = 0; i < 4; ++i)
		{
			if (ViewportManager.GetClients()[i] && ViewportManager.GetClients()[i]->GetCamera())
			{
				UCamera* Cam = ViewportManager.GetClients()[i]->GetCamera();
				// 마우스 우클릭 중이고 해당 뷰포트 위에 마우스가 있으면 카메라 입력 활성화
				bool bEnableInput = (HoveredViewportIndex == i && bIsRightMouseDown);
				Cam->SetInputEnabled(bEnableInput);
			}
		}
	}
	else
	{
		// 싱글 모드: 뷰포트 위에서 마우스 우클릭 시 카메라 입력 활성화
		if (Camera)
		{
			bool bEnableInput = (HoveredViewportIndex == 0 && bIsRightMouseDown);
			Camera->SetInputEnabled(bEnableInput);
		}
	}

	UpdateBatchLines();
	BatchLines.UpdateVertexBuffer();

	ProcessMouseInput();
}

void UEditor::RenderEditor()
{
	if (GEditor->IsPIESessionActive()) { return; }
	BatchLines.Render();
	Axis.Render();
}

void UEditor::RenderGizmo(UCamera* InCamera)
{
    if (GEditor->IsPIESessionActive()) { return; }
    if (bRenderGizmo)
    {
        Gizmo.RenderGizmo(InCamera);
    }
	
    // 모든 DirectionalLight/SpotLight의 빛 방향 기즈모 렌더링 (선택 여부 무관)
	if (ULevel* CurrentLevel = GWorld->GetLevel())
	{
		const TArray<ULightComponent*>& LightComponents = CurrentLevel->GetLightComponents();
		for (ULightComponent* LightComp : LightComponents)
		{
			if (UDirectionalLightComponent* DirLight = Cast<UDirectionalLightComponent>(LightComp))
			{
				DirLight->RenderLightDirectionGizmo(InCamera);
			}
			if (USpotLightComponent* DirLight = Cast<USpotLightComponent>(LightComp))
			{
				DirLight->RenderLightDirectionGizmo(InCamera);
			}
		}
	}
}




void UEditor::UpdateBatchLines()
{
	uint64 ShowFlags = GWorld->GetLevel()->GetShowFlags();

	if (ShowFlags & EEngineShowFlags::SF_Octree)
	{
		BatchLines.UpdateOctreeVertices(GWorld->GetLevel()->GetStaticOctree());
	}
	else
	{
		// If we are not showing the octree, clear the lines, so they don't persist
		BatchLines.ClearOctreeLines();
	}

	if (UActorComponent* Component = GetSelectedComponent())
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
		{
			if (ShowFlags & EEngineShowFlags::SF_Bounds)
			{
				if (PrimitiveComponent->GetBoundingBox()->GetType() == EBoundingVolumeType::AABB)
				{
					FVector WorldMin, WorldMax; PrimitiveComponent->GetWorldAABB(WorldMin, WorldMax);
					FAABB AABB(WorldMin, WorldMax);
					BatchLines.UpdateBoundingBoxVertices(&AABB);
				}
				else 
				{ 
					BatchLines.UpdateBoundingBoxVertices(PrimitiveComponent->GetBoundingBox()); 

					// 만약 선택된 타입이 decalspotlightcomponent라면
					if (Component->IsA(UDecalSpotLightComponent::StaticClass()))
					{
						BatchLines.UpdateDecalSpotLightVertices(Cast<UDecalSpotLightComponent>(Component));
					}
				}
				return; 
			}
		}
		if (ULightComponent* LightComponent = Cast<ULightComponent>(Component))
		{
			if (ShowFlags & EEngineShowFlags::SF_Bounds)
			{
				if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(Component))
				{
					if (USpotLightComponent* SpotLightComponent = Cast<USpotLightComponent>(Component))
					{
						const FVector Center = SpotLightComponent->GetWorldLocation();
						const float Radius = SpotLightComponent->GetAttenuationRadius();
						const float OuterRadian = SpotLightComponent->GetOuterConeAngle();
						const float InnerRadian = SpotLightComponent->GetInnerConeAngle();
						FQuaternion Rotation = SpotLightComponent->GetWorldRotationAsQuaternion();
						BatchLines.UpdateConeVertices(Center, Radius, OuterRadian, InnerRadian, Rotation);
						return;
					}
					const FVector Center = PointLightComponent->GetWorldLocation();
					const float Radius = PointLightComponent->GetAttenuationRadius();

					FBoundingSphere PointSphere(Center, Radius);
					BatchLines.UpdateBoundingBoxVertices(&PointSphere);
					return;
				}
				
			}
		}
	}

	BatchLines.DisableRenderBoundingBox();
}


void UEditor::ProcessMouseInput()
{
	// KTLWeek07: 활성 카메라 사용 (ViewportManager에서 관리)
	UCamera* CurrentCamera = Camera; // 생성자에서 설정된 활성 카메라
	if (!CurrentCamera) { return; }

	AActor* ActorPicked = GetSelectedActor();
	if (ActorPicked)
	{
		// 피킹 전 현재 카메라에 맞는 기즈모 스케일 업데이트
		Gizmo.UpdateScale(CurrentCamera);
	}

	const UInputManager& InputManager = UInputManager::GetInstance();
	const FVector& MousePos = InputManager.GetMousePosition();
	
	// KTLWeek07: 활성 뷰포트의 정보 가져오기
	auto& ViewportManager = UViewportManager::GetInstance();
	FViewport* ActiveViewport = ViewportManager.GetViewports()[ViewportManager.GetActiveIndex()];
	if (!ActiveViewport) { return; }
	
	const D3D11_VIEWPORT& ViewportInfo = ActiveViewport->GetRenderRect();

	const float NdcX = ((MousePos.X - ViewportInfo.TopLeftX) / ViewportInfo.Width) * 2.0f - 1.0f;
	const float NdcY = -(((MousePos.Y - ViewportInfo.TopLeftY) / ViewportInfo.Height) * 2.0f - 1.0f);

	FRay WorldRay = CurrentCamera->ConvertToWorldRay(NdcX, NdcY);

	static EGizmoDirection PreviousGizmoDirection = EGizmoDirection::None;
	FVector CollisionPoint;
	float ActorDistance = -1;

	if (InputManager.IsKeyPressed(EKeyInput::Tab))
	{
		Gizmo.IsWorldMode() ? Gizmo.SetLocal() : Gizmo.SetWorld();
	}
	if (InputManager.IsKeyPressed(EKeyInput::Space))
	{
		Gizmo.ChangeGizmoMode();
	}
	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		Gizmo.EndDrag();
	}

	if (Gizmo.IsDragging() && IsValid<USceneComponent>(Gizmo.GetSelectedComponent()))
	{
		switch (Gizmo.GetGizmoMode())
		{
		case EGizmoMode::Translate:
		{
			FVector GizmoDragLocation = GetGizmoDragLocation(CurrentCamera, WorldRay);
			Gizmo.SetLocation(GizmoDragLocation);
			break;
		}
		case EGizmoMode::Rotate:
		{
			FVector GizmoDragRotation = GetGizmoDragRotation(CurrentCamera, WorldRay);
			Gizmo.SetComponentRotation(GizmoDragRotation);
			break;
		}
		case EGizmoMode::Scale:
		{
			FVector GizmoDragScale = GetGizmoDragScale(CurrentCamera, WorldRay);
			Gizmo.SetComponentScale(GizmoDragScale);
		}
		}
	}
	else
	{
		if (GetSelectedActor() && Gizmo.HasComponent())
		{
			ObjectPicker.PickGizmo(CurrentCamera, WorldRay, Gizmo, CollisionPoint);
		}
		else
		{
			Gizmo.SetGizmoDirection(EGizmoDirection::None);
		}

		if (!ImGui::GetIO().WantCaptureMouse && InputManager.IsKeyPressed(EKeyInput::MouseLeft))
		{
			if (GWorld->GetLevel()->GetShowFlags())
			{
				TArray<UPrimitiveComponent*> Candidate;

				ULevel* CurrentLevel = GWorld->GetLevel();
				ObjectPicker.FindCandidateFromOctree(CurrentLevel->GetStaticOctree(), WorldRay, Candidate);

				TArray<UPrimitiveComponent*>& DynamicCandidates = CurrentLevel->GetDynamicPrimitives();
				if (!DynamicCandidates.empty())
				{
					Candidate.insert(Candidate.end(), DynamicCandidates.begin(), DynamicCandidates.end());
				}
				

				TStatId StatId("Picking");
				FScopeCycleCounter PickCounter(StatId);
				UPrimitiveComponent* PrimitiveCollided = ObjectPicker.PickPrimitive(CurrentCamera, WorldRay, Candidate, &ActorDistance);
				ActorPicked = PrimitiveCollided ? PrimitiveCollided->GetOwner() : nullptr;
				float ElapsedMs = PickCounter.Finish(); // 피킹 시간 측정 종료
				UStatOverlay::GetInstance().RecordPickingStats(ElapsedMs);
			}
		}

		if (Gizmo.GetGizmoDirection() == EGizmoDirection::None)
		{
			SelectActor(ActorPicked);
			if (PreviousGizmoDirection != EGizmoDirection::None)
			{
				Gizmo.OnMouseRelease(PreviousGizmoDirection);
			}
		}
		else
		{
			PreviousGizmoDirection = Gizmo.GetGizmoDirection();
			if (InputManager.IsKeyPressed(EKeyInput::MouseLeft))
			{
				Gizmo.OnMouseDragStart(CollisionPoint);
			}
			else
			{
				Gizmo.OnMouseHovering();
			}
		}
	}
}

FVector UEditor::GetGizmoDragLocation(UCamera* InActiveCamera, FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	FVector GizmoAxis = Gizmo.GetGizmoAxis();

	if (!Gizmo.IsWorldMode())
	{
		// RotationMatrix 로직에 문제
		// FVector4 GizmoAxis4{ GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z, 0.0f };
		// FVector RadRotation = FVector::GetDegreeToRadian(Gizmo.GetComponentRotation());
		// GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);
		FQuaternion q = Gizmo.GetTargetComponent()->GetWorldRotationAsQuaternion();
		GizmoAxis = q.RotateVector(GizmoAxis); 
	}

	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, GizmoAxis.Cross(InActiveCamera->CalculatePlaneNormal(GizmoAxis)), MouseWorld))
	{
		FVector MouseDistance = MouseWorld - Gizmo.GetDragStartMouseLocation();
		return Gizmo.GetDragStartActorLocation() + GizmoAxis * MouseDistance.Dot(GizmoAxis);
	}
	return Gizmo.GetGizmoLocation();
}

FVector UEditor::GetGizmoDragRotation(UCamera* InActiveCamera, FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	FVector GizmoAxis = Gizmo.GetGizmoAxis();

	if (!Gizmo.IsWorldMode())
	{
		FQuaternion q = Gizmo.GetTargetComponent()->GetWorldRotationAsQuaternion();
		GizmoAxis = q.RotateVector(GizmoAxis); 
	}

	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, GizmoAxis, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
		PlaneOriginToMouse.Normalize();
		PlaneOriginToMouseStart.Normalize();
		float DotResult = (PlaneOriginToMouseStart).Dot(PlaneOriginToMouse);
		float Angle = acosf(std::max(-1.0f, std::min(1.0f, DotResult)));
		if ((PlaneOriginToMouse.Cross(PlaneOriginToMouseStart)).Dot(GizmoAxis) < 0)
		{
			Angle = -Angle;
		}

		FQuaternion StartRotQuat = FQuaternion::FromEuler(Gizmo.GetDragStartActorRotation());
		FQuaternion DeltaRotQuat = FQuaternion::FromAxisAngle(Gizmo.GetGizmoAxis(), Angle);
		if (Gizmo.IsWorldMode())
		{
			return (StartRotQuat * DeltaRotQuat).ToEuler();
		}
		else
		{
			return (DeltaRotQuat * StartRotQuat).ToEuler();
		}
	}
	return Gizmo.GetComponentRotation();
}

FVector UEditor::GetGizmoDragScale(UCamera* InActiveCamera, FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin = Gizmo.GetGizmoLocation();
	FVector CardinalAxis = Gizmo.GetGizmoAxis();
	
	FVector GizmoAxis = Gizmo.GetGizmoAxis();
	FQuaternion q = Gizmo.GetTargetComponent()->GetWorldRotationAsQuaternion();
	GizmoAxis = q.RotateVector(GizmoAxis); 

	FVector PlaneNormal = GizmoAxis.Cross(InActiveCamera->CalculatePlaneNormal(GizmoAxis));
	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
		float DragStartAxisDistance = PlaneOriginToMouseStart.Dot(GizmoAxis);
		float DragAxisDistance = PlaneOriginToMouse.Dot(GizmoAxis);
		float ScaleFactor = 1.0f;
		if (abs(DragStartAxisDistance) > 0.1f)
		{
			ScaleFactor = DragAxisDistance / DragStartAxisDistance;
		}

		FVector DragStartScale = Gizmo.GetDragStartActorScale();
		if (ScaleFactor > MinScale)
		{
			if (Gizmo.GetSelectedComponent()->IsUniformScale())
			{
				float UniformValue = DragStartScale.Dot(CardinalAxis);
				return FVector(1.0f, 1.0f, 1.0f) * UniformValue * ScaleFactor;
			}
			else
			{
				return DragStartScale + CardinalAxis * (ScaleFactor - 1.0f) * DragStartScale.Dot(CardinalAxis);
			}
		}
		return Gizmo.GetComponentScale();
	}
	return Gizmo.GetComponentScale();
}

void UEditor::SelectActor(AActor* InActor)
{
	if (InActor == SelectedActor) return;
	
	SelectedActor = InActor;
	if (SelectedActor) { SelectComponent(InActor->GetRootComponent()); }
	else { SelectComponent(nullptr); }
}

void UEditor::SelectComponent(UActorComponent* InComponent)
{
	if (InComponent == SelectedComponent) return;
	
	if (SelectedComponent)
	{
		SelectedComponent->OnDeselected();
	}

	SelectedComponent = InComponent;
	if (SelectedComponent)
	{
		SelectedComponent->OnSelected();
	}
	UUIManager::GetInstance().OnSelectedComponentChanged(SelectedComponent);
}
