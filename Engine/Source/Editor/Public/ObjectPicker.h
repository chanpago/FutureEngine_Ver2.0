#pragma once
#include "pch.h"
#include "Editor/Public/Gizmo.h"

class UPrimitiveComponent;
class AActor;
class ULevel;
class UCamera;
class UGizmo;
class FOctree;
struct FRay;

class UObjectPicker : public UObject
{
public:
	UObjectPicker() = default;
	UPrimitiveComponent* PickPrimitive(UCamera* InActiveCamera, const FRay& WorldRay, TArray<UPrimitiveComponent*> Candidate, float* Distance);
	void PickGizmo(UCamera* InActiveCamera, const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint);
	bool IsRayCollideWithPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane);

	bool FindCandidateFromOctree(FOctree* Node, const FRay& WorldRay, TArray<UPrimitiveComponent*>& OutCandidate);

private:
	void GatherCandidateTriangles(UPrimitiveComponent* Primitive, const FRay& ModelRay, TArray<int32>& OutCandidateTriangleIndices);
	bool IsRayPrimitiveCollided(UCamera* InActiveCamera, const FRay& WorldRay, UPrimitiveComponent* Primitive, const FMatrix& ModelMatrix, float* ShortestDistance);
	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
	bool IsRayTriangleCollided(UCamera* InActiveCamera, const FRay& Ray, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
		const FMatrix& ModelMatrix, float* Distance);
};
