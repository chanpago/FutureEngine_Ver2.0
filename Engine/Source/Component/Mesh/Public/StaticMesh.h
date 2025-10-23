#pragma once

#include "Core/Public/Object.h"       // UObject 기반 클래스 및 매크로
#include "Global/CoreTypes.h"        // TArray 등
#include "Global/BVH.h"

// 전방 선언: FStaticMesh의 전체 정의를 포함할 필요 없이 포인터만 사용
struct FMeshSection
{
	uint32 StartIndex;
	uint32 IndexCount;
	uint32 MaterialSlot;
};

/**
* @brief 스태틱 메시 Cooked Data.
* @note 엔진 내부 관점에서 Static Mesh Asset은 이 구조체를 의미합니다.
*/
struct FStaticMesh
{
	FName PathFileName;

	TArray<FNormalVertex> Vertices;
	TArray<uint32> Indices;
	FBVH BVH; // 메시의 가속 구조

	// --- 2. 재질 정보 (Materials) ---
	// 이 메시에 사용되는 모든 고유 재질의 목록 (페인트 팔레트)
	//TArray<UMaterial> Materials;

	TArray<FMaterial> MaterialInfo;

	// --- 3. 연결 정보 (Sections) ---
	// 각 재질을 어떤 기하 구간에 칠할지에 대한 지시서
	TArray<FMeshSection> Sections;
};


/**
 * @brief FStaticMesh(Cooked Data)를 엔진 오브젝트 시스템에 통합하는 래퍼 클래스.
 * 가비지 컬렉션, 리플렉션, 애셋 참조 관리의 대상이 됩니다.
 * @note 엔진 외부 관점에서 Static Mesh Asset은 이 클래스를 의미합니다.
 */
UCLASS()
class UStaticMesh : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMesh, UObject)

public:
	UStaticMesh();
	virtual ~UStaticMesh(); // 가상 소멸자 권장

	// --- 데이터 연결 ---

	/**
	 * @brief AssetManager가 이 UStaticMesh 객체와 실제 데이터(FStaticMesh)를 연결합니다.
	 * @param InStaticMeshAsset AssetManager가 소유하고 있는 FStaticMesh 데이터에 대한 포인터
	 */
	FStaticMesh* GetStaticMeshAsset() { return StaticMeshAsset; }
	void SetStaticMeshAsset(FStaticMesh* InStaticMeshAsset);

	// --- 데이터 접근자 (Getters) ---
	// 이 UStaticMesh가 감싸고 있는 FStaticMesh의 데이터에 대한 접근을 제공합니다.
	// 렌더러나 컴포넌트는 이 함수들을 통해 실제 데이터에 접근합니다.

	const FName& GetAssetPathFileName() const;

	// Geometry Data
	const TArray<FNormalVertex>& GetVertices() const;
	TArray<FNormalVertex>& GetVertices();
	const TArray<uint32>& GetIndices() const;

	// Material Data
	UMaterial* GetMaterial(int32 MaterialIndex) const;
	void SetMaterial(int32 MaterialIndex, UMaterial* Material);
	int32 GetNumMaterials() const;
	const TArray<FMeshSection>& GetSections() const;

	// 유효성 검사
	bool IsValid() const { return StaticMeshAsset != nullptr; }

private:
	// 실제 데이터 본체(FStaticMesh)에 대한 비소유(non-owning) 포인터.
	// 이 데이터의 실제 소유권 및 생명주기는 AssetManager가 책임집니다.
	FStaticMesh* StaticMeshAsset;

	TArray<UMaterial*> Materials;
};
