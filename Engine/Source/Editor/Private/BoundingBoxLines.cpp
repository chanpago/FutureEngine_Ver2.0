#include "pch.h"
#include "Editor/Public/BoundingBoxLines.h"
#include "Physics/Public/AABB.h"
#include "Physics/Public/OBB.h"
#include "Physics/Public/BoundingSphere.h"

UBoundingBoxLines::UBoundingBoxLines()
	: Vertices(TArray<FVector>()),
	BoundingBoxLineIdx{
		// 앞면
		0, 1,
		1, 2,
		2, 3,
		3, 0,

		// 뒷면
		4, 5,
		5, 6,
		6, 7,
		7, 4,

		// 옆면 연결
		0, 4,
		1, 5,
		2, 6,
		3, 7
	}
{
	Vertices.reserve(NumVertices);
	UpdateVertices(GetDisabledBoundingBox());

}

void UBoundingBoxLines::MergeVerticesAt(TArray<FVector>& DestVertices, size_t InsertStartIndex)
{
	// 인덱스 범위 보정
	InsertStartIndex = std::min(InsertStartIndex, DestVertices.size());

	// 미리 메모리 확보
	DestVertices.reserve(DestVertices.size() + std::distance(Vertices.begin(), Vertices.end()));

	// 덮어쓸 수 있는 개수 계산
	size_t OverwriteCount = std::min(
		Vertices.size(),
		DestVertices.size() - InsertStartIndex
	);

	// 기존 요소 덮어쓰기
	std::copy(
		Vertices.begin(),
		Vertices.begin() + OverwriteCount,
		DestVertices.begin() + InsertStartIndex
	);
}

void UBoundingBoxLines::UpdateVertices(const IBoundingVolume* NewBoundingVolume)
{
	switch (NewBoundingVolume->GetType())
	{
	case EBoundingVolumeType::AABB:
		{
			const FAABB* AABB = static_cast<const FAABB*>(NewBoundingVolume);

			float MinX = AABB->Min.X, MinY = AABB->Min.Y, MinZ = AABB->Min.Z;
			float MaxX = AABB->Max.X, MaxY = AABB->Max.Y, MaxZ = AABB->Max.Z;

			CurrentType = EBoundingVolumeType::AABB;
			CurrentNumVertices = NumVertices;
			Vertices.resize(NumVertices);

			uint32 Idx = 0;
			Vertices[Idx++] = {MinX, MinY, MinZ}; // Front-Bottom-Left
			Vertices[Idx++] = {MaxX, MinY, MinZ}; // Front-Bottom-Right
			Vertices[Idx++] = {MaxX, MaxY, MinZ}; // Front-Top-Right
			Vertices[Idx++] = {MinX, MaxY, MinZ}; // Front-Top-Left
			Vertices[Idx++] = {MinX, MinY, MaxZ}; // Back-Bottom-Left
			Vertices[Idx++] = {MaxX, MinY, MaxZ}; // Back-Bottom-Right
			Vertices[Idx++] = {MaxX, MaxY, MaxZ}; // Back-Top-Right
			Vertices[Idx] = {MinX, MaxY, MaxZ}; // Back-Top-Left
			break;
		}
	case EBoundingVolumeType::OBB:
		{
			const FOBB* OBB = static_cast<const FOBB*>(NewBoundingVolume);
			const FVector& Extents = OBB->Extents;

			FMatrix OBBToWorld = OBB->ScaleRotation;
			OBBToWorld *= FMatrix::TranslationMatrix(OBB->Center);

			FVector LocalCorners[8] =
			{
				FVector(-Extents.X, -Extents.Y, -Extents.Z), // 0: FBL
				FVector(+Extents.X, -Extents.Y, -Extents.Z), // 1: FBR
				FVector(+Extents.X, +Extents.Y, -Extents.Z), // 2: FTR
				FVector(-Extents.X, +Extents.Y, -Extents.Z), // 3: FTL

				FVector(-Extents.X, -Extents.Y, +Extents.Z), // 4: BBL
				FVector(+Extents.X, -Extents.Y, +Extents.Z), // 5: BBR
				FVector(+Extents.X, +Extents.Y, +Extents.Z), // 6: BTR
				FVector(-Extents.X, +Extents.Y, +Extents.Z)  // 7: BTL
			};

			CurrentType = EBoundingVolumeType::OBB;
			CurrentNumVertices = NumVertices;
			Vertices.resize(NumVertices);

			for (uint32 Idx = 0; Idx < 8; ++Idx)
			{
				FVector WorldCorner = OBBToWorld.TransformPosition(LocalCorners[Idx]);

				Vertices[Idx] = {WorldCorner.X, WorldCorner.Y, WorldCorner.Z};
			}
			break;
		}
	case EBoundingVolumeType::SpotLight:
	{
		const FOBB* OBB = static_cast<const FOBB*>(NewBoundingVolume);
		const FVector& Extents = OBB->Extents;

		FMatrix OBBToWorld = OBB->ScaleRotation;
		OBBToWorld *= FMatrix::TranslationMatrix(OBB->Center);

		// 61개의 점들을 로컬에서 찍는다.
		constexpr uint32 NumSegments = 60;
		FVector LocalSpotLight[61];
		LocalSpotLight[0] = FVector(-Extents.X, 0.0f, 0.0f); // 0: Center

		for (int32 i = 0; i < 60;++i)
		{
			LocalSpotLight[i + 1] = FVector(Extents.X, cosf((6.0f * i) * (PI / 180.0f)) * 0.5f, sinf((6.0f * i) * (PI / 180.0f)) * 0.5f); 
		}

		CurrentType = EBoundingVolumeType::SpotLight;
		CurrentNumVertices = SpotLightVeitices;
		Vertices.resize(SpotLightVeitices);

		// 61개의 점을 월드로 변환하고 Vertices에 넣는다.
		// 인덱스에도 넣는다.

		// 0번인덱스는 미리 처리
		FVector WorldCorner = OBBToWorld.TransformPosition(LocalSpotLight[0]);
		Vertices[0] = { WorldCorner.X, WorldCorner.Y, WorldCorner.Z };

		SpotLightLineIdx.clear();
		SpotLightLineIdx.reserve(NumSegments * 4);

		// 꼭지점에서 원으로 뻗는 60개의 선을 월드로 바꾸고 인덱스 번호를 지정한다
		for (uint32 Idx = 1; Idx < 61; ++Idx)
		{
			FVector WorldCorner = OBBToWorld.TransformPosition(LocalSpotLight[Idx]);

			Vertices[Idx] = { WorldCorner.X, WorldCorner.Y, WorldCorner.Z };
			SpotLightLineIdx.emplace_back(0);
			SpotLightLineIdx.emplace_back(static_cast<int32>(Idx));
		}

		// 원에서 각 점을 잇는 선의 인덱스 번호를 지정한다
		SpotLightLineIdx.emplace_back(60);
		SpotLightLineIdx.emplace_back(1);
		for (uint32 Idx = 1; Idx < 60; ++Idx)
		{
			SpotLightLineIdx.emplace_back(static_cast<int32>(Idx));
			SpotLightLineIdx.emplace_back(static_cast<int32>(Idx + 1));
		}
		break;
	}
	case EBoundingVolumeType::Sphere:
	{
		const FBoundingSphere* Sphere = static_cast<const FBoundingSphere*>(NewBoundingVolume);

		const FVector Center = Sphere->Center;
		const float Radius = Sphere->Radius;

		CurrentType = EBoundingVolumeType::Sphere;
		CurrentNumVertices = SphereVertices;
		Vertices.resize(SphereVertices);

		uint32 VertexIndex = 0;

		// 1) XY 평면 대원
		for (int32 Step = 0; Step < 60; ++Step)
		{
			const float AngleRadians = (6.0f * Step) * (PI / 180.0f);
			const float CosValue = cosf(AngleRadians);
			const float SinValue = sinf(AngleRadians);

			Vertices[VertexIndex++] = FVector(
				Center.X + CosValue * Radius,
				Center.Y + SinValue * Radius,
				Center.Z
			);
		}

		// 2) XZ 평면 대원
		for (int32 Step = 0; Step < 60; ++Step)
		{
			const float AngleRadians = (6.0f * Step) * (PI / 180.0f);
			const float CosValue = cosf(AngleRadians);
			const float SinValue = sinf(AngleRadians);

			Vertices[VertexIndex++] = FVector(
				Center.X + CosValue * Radius,
				Center.Y,
				Center.Z + SinValue * Radius
			);
		}

		// 3) YZ 평면 대원
		for (int32 Step = 0; Step < 60; ++Step)
		{
			const float AngleRadians = (6.0f * Step) * (PI / 180.0f);
			const float CosValue = cosf(AngleRadians);
			const float SinValue = sinf(AngleRadians);

			Vertices[VertexIndex++] = FVector(
				Center.X,
				Center.Y + CosValue * Radius,
				Center.Z + SinValue * Radius
			);
		}

		int32 LineIndex = 0;

		// XY 원 인덱스
		{
			const int32 Base = 0;
			for (int32 Step = 0; Step < 59; ++Step)
			{
				SphereLineIdx[LineIndex++] = Base + Step;
				SphereLineIdx[LineIndex++] = Base + Step + 1;
			}
			SphereLineIdx[LineIndex++] = Base + 59;
			SphereLineIdx[LineIndex++] = Base + 0;
		}

		// XZ 원 인덱스
		{
			const int32 Base = 60;
			for (int32 Step = 0; Step < 59; ++Step)
			{
				SphereLineIdx[LineIndex++] = Base + Step;
				SphereLineIdx[LineIndex++] = Base + Step + 1;
			}
			SphereLineIdx[LineIndex++] = Base + 59;
			SphereLineIdx[LineIndex++] = Base + 0;
		}

		// YZ 원 인덱스
		{
			const int32 Base = 120;
			for (int32 Step = 0; Step < 59; ++Step)
			{
				SphereLineIdx[LineIndex++] = Base + Step;
				SphereLineIdx[LineIndex++] = Base + Step + 1;
			}
			SphereLineIdx[LineIndex++] = Base + 59;
			SphereLineIdx[LineIndex++] = Base + 0;
		}

		break;
	}
	default:
		break;
	}
}

void UBoundingBoxLines::UpdateSpotLightVertices(const TArray<FVector>& InVertices)
{
	SpotLightLineIdx.clear();

	if (InVertices.empty())
	{
		CurrentType = EBoundingVolumeType::SpotLight;
		CurrentNumVertices = 0;
		Vertices.clear();
		SpotLightLineIdx.clear();
		return;
	}

	const uint32 NumVerticesRequested = static_cast<uint32>(InVertices.size());

	CurrentType = EBoundingVolumeType::SpotLight;
	CurrentNumVertices = NumVerticesRequested;
	Vertices.resize(NumVerticesRequested);

	std::copy(InVertices.begin(), InVertices.end(), Vertices.begin());

	constexpr uint32 NumSegments = 40;
	const int32 ApexIndex = 2 * (NumSegments + 1);
	const uint32 OuterStart = ApexIndex + 1;
	const uint32 OuterCount = NumVerticesRequested > OuterStart ? std::min(NumSegments, NumVerticesRequested - OuterStart) : 0;
	const uint32 InnerStart = OuterStart + OuterCount;
	const uint32 InnerCount = NumVerticesRequested > InnerStart ? std::min(NumSegments, NumVerticesRequested - InnerStart) : 0;

	if (OuterCount == 0)
	{
		return;
	}

	SpotLightLineIdx.reserve((NumSegments * 4) + (OuterCount * 4) + (InnerCount * 4));

	for (uint32 Segment = 0; Segment < NumSegments; ++Segment)
	{
		// xy 평면 위 호
		SpotLightLineIdx.emplace_back(Segment);
		SpotLightLineIdx.emplace_back(Segment + 1);

		// zx 평면 위 호
		SpotLightLineIdx.emplace_back(Segment + NumSegments + 1);
		SpotLightLineIdx.emplace_back(Segment + NumSegments + 2);
	}
	
	// Apex에서 outer cone 밑면 각 점까지의 선분
	for (uint32 Segment = 0; Segment < OuterCount; ++Segment)
	{
		SpotLightLineIdx.emplace_back(ApexIndex);
		SpotLightLineIdx.emplace_back(static_cast<int32>(OuterStart + Segment));
	}
	
	// outer cone 밑면 둘레 선분
	for (uint32 Segment = 0; Segment < OuterCount; ++Segment)
	{
		const int32 Start = static_cast<int32>(OuterStart + Segment);
		const int32 End = static_cast<int32>(OuterStart + ((Segment + 1) % OuterCount));
		SpotLightLineIdx.emplace_back(Start);
		SpotLightLineIdx.emplace_back(End);
	}

	if (InnerCount >= 2)
	{
		// Apex에서 inner cone 밑면 각 점까지의 선분
		for (uint32 Segment = 0; Segment < InnerCount; ++Segment)
		{
			SpotLightLineIdx.emplace_back(ApexIndex);
			SpotLightLineIdx.emplace_back(static_cast<int32>(InnerStart + Segment));
		}

		// inner cone 밑면 둘레 선분
		for (uint32 Segment = 0; Segment < InnerCount; ++Segment)
		{
			const int32 Start = static_cast<int32>(InnerStart + Segment);
			const int32 End = static_cast<int32>(InnerStart + ((Segment + 1) % InnerCount));
			SpotLightLineIdx.emplace_back(Start);
			SpotLightLineIdx.emplace_back(End);
		}
	}
}

int32* UBoundingBoxLines::GetIndices(EBoundingVolumeType BoundingVolumeType)
{
	switch (BoundingVolumeType)
	{
	case EBoundingVolumeType::AABB:
	{
		return BoundingBoxLineIdx;
	}
	case EBoundingVolumeType::OBB:
	{
		return BoundingBoxLineIdx;
	}
	case EBoundingVolumeType::SpotLight:
	{
		return SpotLightLineIdx.empty() ? nullptr : SpotLightLineIdx.data();
	}
	case EBoundingVolumeType::Sphere:
	{
		return SphereLineIdx;
	}
	default:
		break;
	}

	return nullptr;
}


uint32 UBoundingBoxLines::GetNumIndices(EBoundingVolumeType BoundingVolumeType) const
{
	switch (BoundingVolumeType)
	{
	case EBoundingVolumeType::AABB:
	case EBoundingVolumeType::OBB:
		return 24;
	case EBoundingVolumeType::SpotLight:
		return static_cast<uint32>(SpotLightLineIdx.size());
	case EBoundingVolumeType::Sphere:
		return 360;
	default:
		break;
	}

	return 0;
}
