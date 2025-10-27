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
	FMatrix EyeView;       // V_e
	FMatrix EyeProj;       // P_e
	FMatrix EyeViewProjInv;// (P_e * V_e)^(-1)
	
	FMatrix LightViewP[MAX_CASCADES];    // V_L'
	FMatrix LightProjP[MAX_CASCADES];    // P_L'
	FMatrix LightViewPInv[MAX_CASCADES];  // (V'_L)^(-1)
	
	FVector4 ShadowParams; // x: depthBias, y: (reserved)
	FVector4 CascadeSplits;
	FVector LightDirWS;                   // 월드공간 "표면→광원" 단위벡터
	uint32  bInvertedLight;// 0: normal, 1: inverted (방향광에서는 보통 0)

	FVector4 LightOrthoParams;             // (l, r, b, t)
	FVector2 ShadowMapSize;                // (Sx, Sy)
	uint32	bUsePSM;
	uint32  bUseVSM;	// 0 = depth compare, 1 = VSM                                        
	uint32  bUsePCF;
	uint32  bUseCSM;	// 0 = no CSM, 1 = enable CSM
	float   Padding[2];
};

// Spot Light dedicated shadow constants (single spot caster)
struct FSpotShadowConstants
{
    FMatrix LightView;      // spot view
    FMatrix LightProj;      // spot projection (perspective)
    FVector SpotPosition;   // world pos
    float   SpotRange;      // max range
    FVector SpotDirection;  // world dir (normalized)
    float   OuterCone;      // radians
    float   InnerCone;      // radians
    FVector2 ShadowMapSize; // (Sx, Sy)
    float   ShadowBias;     // depth bias
    uint32  bUseVSM;        // 0/1
    uint32  bUsePCF;        // 0/1
    float   Padding;        // align to 16 bytes
};
