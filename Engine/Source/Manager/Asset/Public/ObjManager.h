#pragma once
#include <Global/CoreTypes.h>
#include <Global/Types.h>
#include <Component/Mesh/Public/StaticMesh.h>
#include <Manager/Asset/Public/ObjImporter.h>
#include <memory>

class FObjManager
{
public:
	static FStaticMesh* LoadObjStaticMeshAsset(const FName& PathFileName, const FObjImporter::Configuration& Config = {});
	static UStaticMesh* LoadObjStaticMesh(const FName& PathFileName, const FObjImporter::Configuration& Config = {});
	static void CreateMaterialsFromMTL(UStaticMesh* StaticMesh, FStaticMesh* StaticMeshAsset, const FName& ObjFilePath);
	static void Release();

	static constexpr size_t INVALID_INDEX = SIZE_MAX;
	
private:
	static TMap<FName, std::unique_ptr<FStaticMesh>> ObjFStaticMeshMap;
	static UMaterial* CachedDefaultMaterial;
};
