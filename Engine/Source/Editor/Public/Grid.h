#pragma once
#include "Core/Public/Object.h"
#include "Global/CoreTypes.h"
#include "Editor/Public/EditorPrimitive.h"

class UGrid : public UObject
{
public:
	UGrid();
	~UGrid() override;
	//void SetLineVertices();
	//void SetGridProperty(float InCellSize, int InNumLines);

	void UpdateVerticesBy(float NewCellSize);
	void MergeVerticesAt(TArray<FVector>& DestVertices, size_t InsertStartIndex);
	//void RenderGrid();

	uint32 GetNumVertices() const
	{
		return NumVertices;
	}

	float GetCellSize() const
	{
		return CellSize;
	}

	void SetCellSize(const float newCellSize)
	{
		CellSize = newCellSize;
	}

private:
	float CellSize = 1.0f;
	int NumLines = 250;
	//FEditorPrimitive Primitive;
	TArray<FVector> Vertices;
	uint32 NumVertices;
};
