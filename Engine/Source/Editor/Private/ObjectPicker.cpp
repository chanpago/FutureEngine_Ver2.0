#include "pch.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Global/Octree.h"
#include "Physics/Public/AABB.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"

FRay UObjectPicker::GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive)
{
	FMatrix ModelInverse = Primitive->GetWorldTransformMatrixInverse();

	FRay ModelRay;
	ModelRay.Origin = Ray.Origin * ModelInverse;
	ModelRay.Direction = Ray.Direction * ModelInverse;
	ModelRay.Direction.Normalize();
	return ModelRay;
}

UPrimitiveComponent* UObjectPicker::PickPrimitive(UCamera* InActiveCamera, const FRay& WorldRay, TArray<UPrimitiveComponent*> Candidate, float* Distance)
{
	UPrimitiveComponent* ShortestPrimitive = nullptr;
	float ShortestDistance = D3D11_FLOAT32_MAX;
	float PrimitiveDistance = D3D11_FLOAT32_MAX;

	for (UPrimitiveComponent* Primitive : Candidate)
	{
		if (!Primitive->CanPick()) { continue; }
		FMatrix ModelMat = Primitive->GetWorldTransformMatrix();
		if (IsRayPrimitiveCollided(InActiveCamera, WorldRay, Primitive, ModelMat, &PrimitiveDistance))
			//Ray와 Primitive가 충돌했다면 거리 테스트 후 가까운 Actor Picking
		{
			if (PrimitiveDistance < ShortestDistance)
			{
				ShortestPrimitive = Primitive;
				ShortestDistance = PrimitiveDistance;
			}
		}
	}
	*Distance = ShortestDistance;

	return ShortestPrimitive;
}

void UObjectPicker::PickGizmo(UCamera* InActiveCamera, const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint)
{
	//Forward, Right, Up순으로 테스트할거임.
	//원기둥 위의 한 점 P, 축 위의 임의의 점 A에(기즈모 포지션) 대해, AP벡터와 축 벡터 V와 피타고라스 정리를 적용해서 점 P의 축부터의 거리 r을 구할 수 있음.
	//r이 원기둥의 반지름과 같다고 방정식을 세운 후 근의공식을 적용해서 충돌여부 파악하고 distance를 구할 수 있음.

	//FVector4 PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
	//dot(PointOnCylinder - GizmoLocation)*Dot(PointOnCylinder - GizmoLocation) - Dot(PointOnCylinder - GizmoLocation, GizmoAxis)^2 = r^2 = radiusOfGizmo
	//이 t에 대한 방정식을 풀어서 근의공식 적용하면 됨.

	FVector GizmoLocation = Gizmo.GetGizmoLocation();
	FVector GizmoAxises[3] = { FVector{1, 0, 0}, FVector{0, 1, 0}, FVector{0, 0, 1} };

	if (Gizmo.GetGizmoMode() == EGizmoMode::Scale || !Gizmo.IsWorldMode())
	{
		FQuaternion q = Gizmo.GetTargetComponent()->GetWorldRotationAsQuaternion();
		for (int i = 0; i < 3; i++)
		{
			// 쿼터니언을 사용해 기본 축을 회전시킵니다.
			GizmoAxises[i] = q.RotateVector(GizmoAxises[i]);
		}
	}

	FVector WorldRayOrigin{ WorldRay.Origin.X,WorldRay.Origin.Y ,WorldRay.Origin.Z };
	FVector WorldRayDirection(WorldRay.Direction.X, WorldRay.Direction.Y, WorldRay.Direction.Z);
	WorldRayDirection.Normalize();

	switch (Gizmo.GetGizmoMode())
	{
	case EGizmoMode::Translate:
	case EGizmoMode::Scale:
	{
		FVector GizmoDistanceVector = WorldRayOrigin - GizmoLocation;
		bool bIsCollide = false;

		float GizmoRadius = Gizmo.GetTranslateRadius();
		float GizmoHeight = Gizmo.GetTranslateHeight();
		float A, B, C; //Ax^2 + Bx + C의 ABC
		float X; //해
		float Det; //판별식
		//0 = forward 1 = Right 2 = UP

		for (int a = 0; a < 3; a++)
		{
			FVector GizmoAxis = GizmoAxises[a];
			A = 1 - static_cast<float>(pow(WorldRay.Direction.Dot3(GizmoAxis), 2));
			B = WorldRay.Direction.Dot3(GizmoDistanceVector) - WorldRay.Direction.Dot3(GizmoAxis) * GizmoDistanceVector.
				Dot(GizmoAxis); //B가 2의 배수이므로 미리 약분
			C = static_cast<float>(GizmoDistanceVector.Dot(GizmoDistanceVector) -
				pow(GizmoDistanceVector.Dot(GizmoAxis), 2)) - GizmoRadius * GizmoRadius;

			Det = B * B - A * C;
			if (Det >= 0) //판별식 0이상 => 근 존재. 높이테스트만 통과하면 충돌
			{
				X = (-B + sqrtf(Det)) / A;
				FVector PointOnCylinder = WorldRayOrigin + WorldRayDirection * X;
				float Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
				if (Height <= GizmoHeight && Height >= 0) //충돌
				{
					CollisionPoint = PointOnCylinder;
					bIsCollide = true;

				}
				X = (-B - sqrtf(Det)) / A;
				PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
				Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
				if (Height <= GizmoHeight && Height >= 0)
				{
					CollisionPoint = PointOnCylinder;
					bIsCollide = true;
				}
				if (bIsCollide)
				{
					switch (a)
					{
					case 0:	Gizmo.SetGizmoDirection(EGizmoDirection::Forward);	return;
					case 1:	Gizmo.SetGizmoDirection(EGizmoDirection::Right);	return;
					case 2:	Gizmo.SetGizmoDirection(EGizmoDirection::Up);		return;
					}
				}
			}
		}
	} break;
	case EGizmoMode::Rotate:
	{
		for (int a = 0; a < 3; a++)
		{
			if (IsRayCollideWithPlane(WorldRay, GizmoLocation, GizmoAxises[a], CollisionPoint))
			{
				FVector RadiusVector = CollisionPoint - GizmoLocation;
				if (Gizmo.IsInRadius(RadiusVector.Length()))
				{
					switch (a)
					{
					case 0:	Gizmo.SetGizmoDirection(EGizmoDirection::Forward);	return;
					case 1:	Gizmo.SetGizmoDirection(EGizmoDirection::Right);	return;
					case 2:	Gizmo.SetGizmoDirection(EGizmoDirection::Up);		return;
					}
				}
			}
		}
	} break;
	default: break;
	}

	Gizmo.SetGizmoDirection(EGizmoDirection::None);
}

//개별 primitive와 ray 충돌 검사
bool UObjectPicker::IsRayPrimitiveCollided(UCamera* InActiveCamera, const FRay& WorldRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance)
{
	// 1. World Bounding Box를 통해 rough한 충돌 체크
	FVector Min, Max;
	Primitive->GetWorldAABB(Min, Max);
	FAABB WorldAABB(Min, Max);
	if (!CheckIntersectionRayBox(WorldRay, WorldAABB))
	{
		return false; //AABB와 충돌하지 않으면 false반환
	}

	// 2. 삼각형 단위로 정밀 충돌 체크
	float Distance = D3D11_FLOAT32_MAX; //Distance 초기화
	bool bIsHit = false;
	
	const TArray<FNormalVertex>* Vertices = Primitive->GetVerticesData();
	const TArray<uint32>* Indices = Primitive->GetIndicesData();

	FRay ModelRay = GetModelRay(WorldRay, Primitive);
	
	// 충돌 가능성 있는 삼각형 인덱스 수집
	// Triangle Ordinal(인덱스 버퍼를 3개 단위로 묶었을 때의 삼각형 번호)로 반환
	TArray<int32> CandidateTriangleIndices;
	GatherCandidateTriangles(Primitive, ModelRay, CandidateTriangleIndices);

	for (int32 TriIndex : CandidateTriangleIndices)
	{
		FVector V0, V1, V2;
		if (Indices)
		{
			V0 = (*Vertices)[(*Indices)[TriIndex * 3 + 0]].Position;
			V1 = (*Vertices)[(*Indices)[TriIndex * 3 + 1]].Position;
			V2 = (*Vertices)[(*Indices)[TriIndex * 3 + 2]].Position;
		}
		else
		{
			V0 = (*Vertices)[TriIndex * 3 + 0].Position;
			V1 = (*Vertices)[TriIndex * 3 + 1].Position;
			V2 = (*Vertices)[TriIndex * 3 + 2].Position;
		}

		if (IsRayTriangleCollided(InActiveCamera, ModelRay, V0, V1, V2, ModelMatrix, &Distance))
		{
			bIsHit = true;
			*ShortestDistance = std::min(*ShortestDistance, Distance);
		}
	}

	return bIsHit;
}

bool UObjectPicker::IsRayTriangleCollided(UCamera* InActiveCamera, const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
                           const FMatrix& ModelMatrix, float* Distance)
{
	FVector CameraForward = InActiveCamera->GetForward(); //카메라 정보 필요
	float NearZ = InActiveCamera->GetNearZ();
	float FarZ = InActiveCamera->GetFarZ();
	FMatrix ModelTransform; //Primitive로부터 얻어내야함.(카메라가 처리하는게 나을듯)


	//삼각형 내의 점은 E1*V + E2*U + Vertex1.Position으로 표현 가능( 0<= U + V <=1,  Y>=0, V>=0 )
	//Ray.Direction * T + Ray.Origin = E1*V + E2*U + Vertex1.Position을 만족하는 T U V값을 구해야 함.
	//[E1 E2 RayDirection][V U T] = [RayOrigin-Vertex1.Position]에서 cramer's rule을 이용해서 T U V값을 구하고
	//U V값이 저 위의 조건을 만족하고 T값이 카메라의 near값 이상이어야 함.
	FVector RayDirection{Ray.Direction.X, Ray.Direction.Y, Ray.Direction.Z};
	FVector RayOrigin{Ray.Origin.X, Ray.Origin.Y, Ray.Origin.Z};
	FVector E1 = Vertex2 - Vertex1;
	FVector E2 = Vertex3 - Vertex1;
	FVector Result = (RayOrigin - Vertex1); //[E1 E2 -RayDirection]x = [RayOrigin - Vertex1.Position] 의 result임.


	FVector CrossE2Ray = RayDirection.Cross(E2);
	FVector CrossE1Result = Result.Cross(E1);

	float Determinant = E1.Dot(CrossE2Ray);

	float NoInverse = 0.0001f; //0.0001이하면 determinant가 0이라고 판단=>역행렬 존재 X
	if (abs(Determinant) <= NoInverse)
	{
		return false;
	}


	float V = Result.Dot(CrossE2Ray) / Determinant; //cramer's rule로 해를 구했음. 이게 0미만 1초과면 충돌하지 않음.

	if (V < 0 || V > 1)
	{
		return false;
	}

	float U = RayDirection.Dot(CrossE1Result) / Determinant;
	if (U < 0 || U + V > 1)
	{
		return false;
	}

	float T = E2.Dot(CrossE1Result) / Determinant;

	FVector HitPoint = RayOrigin + RayDirection * T; //모델 좌표계에서의 충돌점
	FVector4 HitPoint4{HitPoint.X, HitPoint.Y, HitPoint.Z, 1};
	//이제 이것을 월드 좌표계로 변환해서 view Frustum안에 들어가는지 판단할 것임.(near, far plane만 테스트하면 됨)

	FVector4 HitPointWorld = HitPoint4 * ModelMatrix;
	FVector4 RayOriginWorld = Ray.Origin * ModelMatrix;

	FVector4 DistanceVec = HitPointWorld - RayOriginWorld;
	if (DistanceVec.Dot3(CameraForward) >= NearZ && DistanceVec.Dot3(CameraForward) <= FarZ)
	{
		*Distance = DistanceVec.Length();
		return true;
	}
	return false;
}

bool UObjectPicker::IsRayCollideWithPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane)
{
	FVector WorldRayOrigin{ WorldRay.Origin.X, WorldRay.Origin.Y ,WorldRay.Origin.Z };

	if (abs(WorldRay.Direction.Dot3(Normal)) < 0.01f)
		return false;

	float Distance = (PlanePoint - WorldRayOrigin).Dot(Normal) / WorldRay.Direction.Dot3(Normal);

	if (Distance < 0)
		return false;
	PointOnPlane = WorldRay.Origin + WorldRay.Direction * Distance;


	return true;
}

/**
 * 레이와 충돌하는 후보 노드들을 찾아 그 안의 프리미티브들을 OutCandidate에 담습니다.
 * @return 후보를 찾았으면 true, 못 찾았으면 false를 반환합니다.
 */
bool UObjectPicker::FindCandidateFromOctree(FOctree* Node, const FRay& WorldRay, TArray<UPrimitiveComponent*>& OutCandidate)
{
	// 0. nullptr인지 검사.
	if (!Node) { return false; }

	// 1. 레이가 현재 노드와 겹치지 않으면 검사 생략.
	if (CheckIntersectionRayBox(WorldRay, Node->GetBoundingBox()) == false) { return false; }

	// 2. 현재 노드와 레이가 교차하므로, 이 노드에 직접 포함된 프리미티브들을 후보에 추가합니다.
	const auto& CurrentNodePrimitives = Node->GetPrimitives();
	if (!CurrentNodePrimitives.empty())
	{
		OutCandidate.insert(OutCandidate.end(), CurrentNodePrimitives.begin(), CurrentNodePrimitives.end());
	}

	// 3. 리프 노드가 아니라면, 자식 노드를 재귀적으로 탐색합니다.
	if (!Node->IsLeafNode())
	{
		for (FOctree* Child : Node->GetChildren())
		{
			FindCandidateFromOctree(Child, WorldRay, OutCandidate);
		}
	}

	return true;
}

void UObjectPicker::GatherCandidateTriangles(UPrimitiveComponent* Primitive, const FRay& ModelRay, TArray<int32>& OutCandidateIndices)
{
	if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Primitive))
	{
		if (FStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh()->GetStaticMeshAsset())
		{
			if (StaticMesh->BVH.TraverseRay(ModelRay, OutCandidateIndices))
            {
				return;
            }
		}
	}

	// fallback: 전체 삼각형 인덱스 채우기
	const TArray<FNormalVertex>* Vertices = Primitive->GetVerticesData();
	const TArray<uint32>* Indices = Primitive->GetIndicesData();

	const int32 NumVertices = Primitive->GetNumVertices();
	const int32 NumIndices = Primitive->GetNumIndices();
	const int32 NumTriangles = (NumIndices > 0) ? (NumIndices / 3) : (NumVertices / 3);

	OutCandidateIndices.reserve(NumTriangles);
	for (int32 TriIndex = 0; TriIndex < NumTriangles; TriIndex++)
	{
		OutCandidateIndices.push_back(TriIndex);
	}
	return;
}