#pragma once
#include "Core/Public/Object.h"

class UTexture;

/**
 * @note: This struct is exactly same as the one defined in ObjImporter.h.
 * This is intentionally introduced for abstractional layer of material object.
 */
struct FMaterial
{
	FString Name;

	/** Ambient color (Ka). */
	FVector Ka;

	/** Diffuse color (Kd). */
	FVector Kd{0.1,0.1,0.1}; // 기본 값을 바꿔줘야 프리미티브들의 색상이 바뀜

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

	/** Bump map (map_bump or bump). */
	FString BumpMap;
};

UCLASS()
class UMaterial : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UMaterial, UObject)

public:
	UMaterial() {}
	~UMaterial() override;

	FVector GetAmbientColor() const { return MaterialData.Ka; }
	FVector GetDiffuseColor() const { return MaterialData.Kd; }
	FVector GetSpecularColor() const { return MaterialData.Ks; }
	float GetSpecularExponent() const { return MaterialData.Ns; }
	float GetRefractionIndex() const { return MaterialData.Ni; }
	float GetDissolveFactor() const { return MaterialData.D; }
	
	// Texture access functions
	UTexture* GetDiffuseTexture() const { return DiffuseTexture; }
	UTexture* GetAmbientTexture() const { return AmbientTexture; }
	UTexture* GetSpecularTexture() const { return SpecularTexture; }
	UTexture* GetNormalTexture() const { return NormalTexture; }
	UTexture* GetAlphaTexture() const { return AlphaTexture; }
	UTexture* GetBumpTexture() const { return BumpTexture; }

	void SetMaterialData(const FMaterial& InMaterialData) { MaterialData = InMaterialData; }

	void SetDiffuseTexture(UTexture* InTexture) { DiffuseTexture = InTexture; }
	void SetAmbientTexture(UTexture* InTexture) { AmbientTexture = InTexture; }
	void SetSpecularTexture(UTexture* InTexture) { SpecularTexture = InTexture; }
	void SetNormalTexture(UTexture* InTexture) { NormalTexture = InTexture; }
	void SetAlphaTexture(UTexture* InTexture) { AlphaTexture = InTexture; }
	void SetBumpTexture(UTexture* InTexture) { BumpTexture = InTexture; }


	void SetAmbientColor(FVector& InColor) { MaterialData.Ka = InColor; }
	void SetDiffuseColor(FVector& InColor) { MaterialData.Kd = InColor; }
	void SetSpecularColor(FVector& InColor) { MaterialData.Ks = InColor; }

private:
	UTexture* DiffuseTexture = nullptr;
	UTexture* AmbientTexture = nullptr;
	UTexture* SpecularTexture = nullptr;
	UTexture* NormalTexture = nullptr;
	UTexture* AlphaTexture = nullptr;
	UTexture* BumpTexture = nullptr;

	FMaterial MaterialData;
};
