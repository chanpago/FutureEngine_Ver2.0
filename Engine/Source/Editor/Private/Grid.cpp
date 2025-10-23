#include "pch.h"
#include "Editor/Public/Grid.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Manager/Config/Public/ConfigManager.h"


UGrid::UGrid()
	: Vertices(TArray<FVector>())
	, NumLines(250)
	, CellSize(0) // 아래 UpdateVerticesBy에 넣어주는 값과 달라야 함
{
	NumVertices = NumLines * 4;
	Vertices.reserve(NumVertices);
	UpdateVerticesBy(UConfigManager::GetInstance().GetCellSize());
}
UGrid::~UGrid()
{
	UConfigManager::GetInstance().SetCellSize(CellSize);
}

void UGrid::UpdateVerticesBy(float NewCellSize)
{
	// 중복 삽입 방지
	if (CellSize == NewCellSize)
	{
		return;
	}

	CellSize = NewCellSize; // 필요하다면 멤버 변수도 갱신

	float LineLength = NewCellSize * static_cast<float>(NumLines) / 2.f;

	if (Vertices.size() < NumVertices)
	{
		Vertices.resize(NumVertices);
	}

	uint32 vertexIndex = 0;
	// z축 라인 업데이트
	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount)
	{
		if (LineCount == 0)
		{
			Vertices[vertexIndex++] = { static_cast<float>(LineCount) * NewCellSize, -LineLength, 0.0f };
			Vertices[vertexIndex++] = { static_cast<float>(LineCount) * NewCellSize, 0.f, 0.f };
		}
		else
		{
			Vertices[vertexIndex++] = { static_cast<float>(LineCount) * NewCellSize, -LineLength, 0.0f };
			Vertices[vertexIndex++] = { static_cast<float>(LineCount) * NewCellSize, LineLength, 0.0f };
		}
	}

	// x축 라인 업데이트
	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount)
	{
		if (LineCount == 0)
		{
			Vertices[vertexIndex++] = { -LineLength, static_cast<float>(LineCount) * NewCellSize, 0.0f };
			Vertices[vertexIndex++] = { 0.f, static_cast<float>(LineCount) * NewCellSize, 0.0f };
		}
		else
		{
			Vertices[vertexIndex++] = { -LineLength, static_cast<float>(LineCount) * NewCellSize, 0.0f };
			Vertices[vertexIndex++] = { LineLength, static_cast<float>(LineCount) * NewCellSize, 0.0f };
		}
	}
}


void UGrid::MergeVerticesAt(TArray<FVector>& DestVertices, size_t InsertStartIndex)
{
	// 인덱스 범위 보정
	InsertStartIndex = std::min(DestVertices.size(), InsertStartIndex);
	
	// 미리 메모리 확보
	DestVertices.reserve(DestVertices.size() + std::distance(Vertices.begin(), Vertices.end()));

	// 덮어쓸 수 있는 개수 계산
	size_t overwriteCount = std::min(
		Vertices.size(),
		DestVertices.size() - InsertStartIndex
	);

	// 기존 요소 덮어쓰기
	std::copy(
		Vertices.begin(),
		Vertices.begin() + overwriteCount,
		DestVertices.begin() + InsertStartIndex
	);

	// 원하는 위치에 삽입
	/*destVertices.insert(
		destVertices.begin() + insertStartIndex,
		Vertices.begin(),
		Vertices.end()
	);*/
}

//void UGrid::RenderGrid()
//{
//	URenderer& Renderer = URenderer::GetInstance();
//
//	Renderer.RenderPrimitive(Primitive, Primitive.RenderState);
//
//}
//void UGrid::SetGridProperty(float InCellSize, int32 InNumLines)
//{
//	this->CellSize = InCellSize;
//	this->NumLines = InNumLines;
//}
//
//void UGrid::SetLineVertices()
//{
//	float LineLength = CellSize * static_cast<float>(NumLines) / 2.f;
//
//	for (int32 LineCount = -NumLines/2; LineCount < NumLines/2; ++LineCount) // z축 라인
//	{
//		if (LineCount == 0)
//		{
//			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize,0.f , -LineLength}, Primitive.Color });
//			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize,0.f , 0.f}, Primitive.Color });
//		}
//		else
//		{
//			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize,0.f , -LineLength}, Primitive.Color });
//			LineVertices.push_back({ {static_cast<float>(LineCount) * CellSize,0.f , LineLength}, Primitive.Color });
//		}
//	}
//	for (int32 LineCount = -NumLines / 2; LineCount < NumLines / 2; ++LineCount) // x축 라인
//	{
//		if (LineCount == 0)
//		{
//			LineVertices.push_back({ {-LineLength, 0.f, static_cast<float>(LineCount) * CellSize}, Primitive.Color });
//			LineVertices.push_back({ {0.f, 0.f, static_cast<float>(LineCount) * CellSize}, Primitive.Color });
//		}
//		else
//		{
//			LineVertices.push_back({ {-LineLength, 0.f, static_cast<float>(LineCount) * CellSize}, Primitive.Color });
//			LineVertices.push_back({ {LineLength, 0.f, static_cast<float>(LineCount) * CellSize}, Primitive.Color });
//		}
//	}
//}
