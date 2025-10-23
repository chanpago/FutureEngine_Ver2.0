#pragma once

// C++ Standard Libraries
#include <filesystem>
#include <fstream>
#include <sstream>

// Engine Headers
#include "Core/Public/Archive.h"
#include "Global/Macro.h"
#include "Global/Types.h"
#include "Global/Vector.h"

// Forward Declarations
struct FObjectInfo;
struct FObjectMaterialInfo;

/** @brief Holds all the data parsed from a single .obj file, including all objects, materials, and global vertex data. */
struct FObjInfo
{
	TArray<FObjectInfo> ObjectInfoList;
	TArray<FObjectMaterialInfo> ObjectMaterialInfoList;

	TArray<FVector> VertexList;
	TArray<FVector> NormalList;
	TArray<FVector2> TexCoordList;
};

inline FArchive& operator<<(FArchive& Ar, FObjInfo& ObjInfo)
{
	Ar << ObjInfo.ObjectInfoList;
	Ar << ObjInfo.ObjectMaterialInfoList;

	Ar << ObjInfo.VertexList;
	Ar << ObjInfo.NormalList;
	Ar << ObjInfo.TexCoordList;

	return Ar;
}

/**
 * @brief Represents a single object within a .obj file, defined by the 'o' tag.
 * Contains face indices that refer to the global vertex lists in FObjInfo.
 */
struct FObjectInfo
{
	FString Name;

	/** Face Information */
	TArray<size_t> VertexIndexList;
	TArray<size_t> NormalIndexList;
	TArray<size_t> TexCoordIndexList;

	/** Group Information */
	TArray<FString> GroupNameList;
	/** Stores the starting face index for each group defined by the 'g' tag. */
	TArray<size_t> GroupIndexList; 

	/** Material Information */
	TArray<FString> MaterialNameList;
	/** Stores the starting face index for each material defined by the 'usemtl' tag. */
	TArray<size_t> MaterialIndexList; 
};

inline FArchive& operator<<(FArchive& Ar, FObjectInfo& ObjectInfo)
{
	Ar << ObjectInfo.Name;

	Ar << ObjectInfo.VertexIndexList;
	Ar << ObjectInfo.NormalIndexList;
	Ar << ObjectInfo.TexCoordIndexList;

	Ar << ObjectInfo.GroupNameList;
	Ar << ObjectInfo.GroupIndexList;

	Ar << ObjectInfo.MaterialNameList;
	Ar << ObjectInfo.MaterialIndexList;

	return Ar;
}

struct FObjectMaterialInfo
{
	FString Name;

	/** Ambient color (Ka). */
	FVector Ka;

	/** Diffuse color (Kd). */
	FVector Kd{1, 1, 1};

	/** Specular color (Ks). */
	FVector Ks;

	/** Emissive color (Ke) */
	FVector Ke;

	/** Specular exponent (Ns). Defines the size of the specular highlight. */
	float Ns;

	/** Optical density or index of refraction (Ni). */
	float Ni;

	/** Dissolve factor (d). 1.0 is fully opaque. */
	float D;

	/** Illumination model (illum). */
	int32 Illumination;

	/** Ambient texture map (map_Ka). */
	FString KaMap;

	/** Diffuse texture map (map_Kd). */
	FString KdMap;

	/** Specular texture map (map_Ks). */
	FString KsMap;

	/** Specular highlight map (map_Ns). */
	FString NsMap;

	/** Alpha texture map (map_d). */
	FString DMap;

	/** Bump map (map_Bump or bump). */
	FString BumpMap;
};

inline FArchive& operator<<(FArchive& Ar, FObjectMaterialInfo& ObjectMaterialInfo)
{
	Ar << ObjectMaterialInfo.Name;
	Ar << ObjectMaterialInfo.Ka;
	Ar << ObjectMaterialInfo.Kd;
	Ar << ObjectMaterialInfo.Ks;
	Ar << ObjectMaterialInfo.Ke;
	Ar << ObjectMaterialInfo.Ns;
	Ar << ObjectMaterialInfo.Ni;
	Ar << ObjectMaterialInfo.D;
	Ar << ObjectMaterialInfo.Illumination;
	Ar << ObjectMaterialInfo.KaMap;
	Ar << ObjectMaterialInfo.KdMap;
	Ar << ObjectMaterialInfo.KsMap;
	Ar << ObjectMaterialInfo.NsMap;
	Ar << ObjectMaterialInfo.DMap;
	Ar << ObjectMaterialInfo.BumpMap;
	return Ar;
}

struct FObjImporter
{
	/** @todo: Implement configuration to manage behaviors of LoadObj */
	struct Configuration
	{
		FString DefaultName = "DefaultObject";
		bool bIsObjectEnabled = false;
		bool bIsBinaryEnabled = false;
		bool bFlipWindingOrder = false;
		bool bPositionToUEBasis = true;
		bool bNormalToUEBasis = true;
		bool bUVToUEBasis = true;
		// ...
	};

	/**
	 * @brief Loads and parses a .obj file from the given path.
	 * @param FilePath The absolute or relative path to the .obj file.
	 * @param OutObjInfo A pointer to an FObjInfo struct that will be populated with the file's data.
	 * @param Config Configuration options for the import process.
	 * @return True if the file was loaded and parsed successfully, false otherwise.
	 */
	static bool LoadObj(const std::filesystem::path& FilePath, FObjInfo* OutObjInfo, Configuration Config = {});

	/**
	 * @brief Loads and parses a .mtl material library file.
	 * @param FilePath The path to the .mtl file.
	 * @param OutObjInfo A pointer to the parent FObjInfo struct where materials will be added.
	 * @return True if the material library was loaded successfully, false otherwise.
	 */
	static bool LoadMaterial(const std::filesystem::path& FilePath, FObjInfo* OutObjInfo);

private:
	/**
	 * @brief Parses a single face component string (e.g., "v/vt/vn").
	 * @param FaceBuffer The string chunk representing one vertex of a face.
	 * @param OutObjectInfo The object info struct to populate with the parsed indices.
	 * @return True on success, false on failure.
	 * @note This function assumes a consistent face format within a single object.
	 *       Mixing formats (e.g., 'f 1/1' and 'f 1//1') may lead to incorrect parsing.
	 */
	static bool ParseFaceBuffer(const FString& FaceBuffer, FObjectInfo* OutObjectInfo);

	static FVector PositionToUEBasis(const FVector& InVector)
	{
		return FVector(InVector.X, -InVector.Y, InVector.Z);
	}

	static FVector NormalToUEBasis(const FVector& InVector)
	{
		return FVector(InVector.X, -InVector.Y, InVector.Z);
	}

	static FVector2 UVToUEBasis(const FVector2& InVector)
	{
		return FVector2(InVector.X, 1.0f - InVector.Y);
	}
};
