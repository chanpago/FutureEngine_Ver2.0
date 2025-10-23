#pragma once

#include "Component/Public/PrimitiveComponent.h"
#include "Physics/Public/AABB.h"

class FOctree;

enum class EBoundCheckResult
{
	Outside,
	Intersect,
	Inside
};

struct FFrustum
{
    FVector4 Planes[6];

    EBoundCheckResult CheckIntersection(const FAABB& BBox) const
    {
        EBoundCheckResult Result = EBoundCheckResult::Inside;

        for (int i = 0; i < 6; ++i)
        {
            const FVector4& P = Planes[i];

            // positive vertex: 법선 방향으로 가장 먼 꼭짓점
            FVector PositiveVertex(
                (P.X >= 0) ? BBox.Max.X : BBox.Min.X,
                (P.Y >= 0) ? BBox.Max.Y : BBox.Min.Y,
                (P.Z >= 0) ? BBox.Max.Z : BBox.Min.Z
            );

            if (P.Dot3(PositiveVertex) + P.W > 0)
            {
                // 박스가 평면 바깥(+측)으로 완전히 나감
                return EBoundCheckResult::Outside;
            }

            // negative vertex: 반대쪽 꼭짓점
            FVector NegativeVertex(
                (P.X >= 0) ? BBox.Min.X : BBox.Max.X,
                (P.Y >= 0) ? BBox.Min.Y : BBox.Max.Y,
                (P.Z >= 0) ? BBox.Min.Z : BBox.Max.Z
            );

            if (P.Dot3(NegativeVertex) + P.W < 0)
            {
                // 박스가 평면 안쪽(-측)으로 완전히 들어옴 → 계속 검사
                continue;
            }

            // 완전 안쪽도 아니고 완전 바깥도 아니면 교차
            Result = EBoundCheckResult::Intersect;
        }

        return Result;

    }

    void Clear() { for (int i = 0; i < 6; ++i) { Planes[i] = FVector4::Zero(); }; }
};

class ViewVolumeCuller
{
public:
	ViewVolumeCuller() = default;
	~ViewVolumeCuller() = default;
	ViewVolumeCuller(const ViewVolumeCuller& Other) = default;
	ViewVolumeCuller& operator=(const ViewVolumeCuller& Other) = default;

	void Cull(
        FOctree* StaticOctree,
        TArray<UPrimitiveComponent*>& DynamicPrimitives,
		const FCameraConstants& ViewProjConstants
	);

	const TArray<UPrimitiveComponent*>& GetRenderableObjects();
private:
    void CullOctree(FOctree* Octree);

    FFrustum CurrentFrustum{};
    TArray<UPrimitiveComponent*> RenderableObjects{};
};