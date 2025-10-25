#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include "Global/Types.h"
#include "Core/Public/Name.h"
#include <Texture/Public/Material.h>

//struct BatchLineContants
//{
//	float CellSize;
//	//FMatrix BoundingBoxModel;
//	uint32 ZGridStartIndex; // 인덱스 버퍼에서, z방향쪽 그리드가 시작되는 인덱스
//	uint32 BoundingBoxStartIndex; // 인덱스 버퍼에서, 바운딩박스가 시작되는 인덱스
//};

struct FCameraConstants
{
	FCameraConstants() : NearClip(0), FarClip(0)
	{
		View = FMatrix::Identity();
		Projection = FMatrix::Identity();
	}

	FMatrix View;
	FMatrix Projection;
	FVector ViewWorldLocation;    
	float NearClip;
	float FarClip;
};

#define HAS_DIFFUSE_MAP	 (1 << 0)
#define HAS_AMBIENT_MAP	 (1 << 1)
#define HAS_SPECULAR_MAP (1 << 2)
#define HAS_NORMAL_MAP	 (1 << 3)
#define HAS_ALPHA_MAP	 (1 << 4)
#define HAS_BUMP_MAP	 (1 << 5)

struct FMaterialConstants
{
	FVector4 Ka;
	FVector4 Kd;
	FVector4 Ks;
	float Ns;
	float Ni;
	float D;
	uint32 MaterialFlags;
	float Time; // Time in seconds
};

struct FVertex
{
	FVector Position;
	FVector4 Color;
};

struct FNormalVertex
{
	FVector Position;
	FVector Normal;
	FVector4 Color;
	FVector2 TexCoord;
	FVector4 Tangent;  // XYZ: Tangent, W: Handedness(+1/-1)
};

struct FRay
{
	FVector4 Origin;
	FVector4 Direction;
};

/**
 * @brief Render State Settings for Actor's Component
 */
struct FRenderState
{
	ECullMode CullMode = ECullMode::None;
	EFillMode FillMode = EFillMode::Solid;
};

/**
 * @brief 변환 정보를 담는 구조체
 */
struct FTransform
{
	FVector Location = FVector(0.0f, 0.0f, 0.0f);
	FVector Rotation = FVector(0.0f, 0.0f, 0.0f);
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	FTransform() = default;

	FTransform(const FVector& InLocation, const FVector& InRotation = FVector::ZeroVector(),
		const FVector& InScale = FVector::OneVector())
		: Location(InLocation), Rotation(InRotation), Scale(InScale)
	{
	}
};

/**
 * @brief 2차원 좌표의 정보를 담는 구조체
 */
struct FPoint
{
	INT X = 0;
	INT Y = 0;
	constexpr FPoint(LONG InX, LONG InY) : X(InX), Y(InY)
	{
	}
};

/**
 * @brief 윈도우를 비롯한 2D 화면의 정보를 담는 구조체
 */
struct FRect
{
	LONG Left = 0;
	LONG Top = 0;
	LONG Width = 0;
	LONG Height = 0;

	LONG GetRight() const { return Left + Width; }
	LONG GetBottom() const { return Top + Height; }
};

struct FAmbientLightInfo
{
	FVector4 Color;
	float Intensity;
	FVector Padding;
};

struct FDirectionalLightInfo
{
	FVector4 Color;
	FVector Direction;
	float Intensity;
};

//StructuredBuffer padding 없어도됨
struct FPointLightInfo
{
	FVector4 Color;
	FVector Position;
	float Intensity;
	float Range;
	float DistanceFalloffExponent;
	FVector2 padding;
};

//StructuredBuffer padding 없어도됨
struct FSpotLightInfo
{
	// Point Light와 공유하는 속성 (필드 순서 맞춤)
	FVector4 Color;
	FVector Position;
	float Intensity;
	float Range;
	float DistanceFalloffExponent;

	// SpotLight 고유 속성
	float InnerConeAngle;
	float OuterConeAngle;
	float AngleFalloffExponent;
	FVector Direction;
};

struct FGlobalLightConstant
{
	FAmbientLightInfo Ambient;
	FDirectionalLightInfo Directional;
};

#define MAX_CASCADES 4

struct FShadowMapConstants
{
	FMatrix LightViewMatrix[MAX_CASCADES];
	FMatrix LightProjectionMatrix[MAX_CASCADES];
	FVector4 CascadeSplits;
	float ShadowBias;
	float UseVSM; // 0 = depth compare, 1 = VSM
	float UseCSM; // 0 = no CSM, 1 = enable CSM
	float Padding;
};
