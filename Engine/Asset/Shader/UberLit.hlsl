// UberLit.hlsl - Uber Shader with Multiple Lighting Models
// Supports: Gouraud, Lambert, Phong lighting models

// =============================================================================
// <주의사항>
// normalize 대신 SafeNormalize 함수를 사용하세요.
// normalize에는 영벡터 입력시 NaN이 발생할 수 있습니다. (div by zero 가드가 없음)
// =============================================================================


#define NUM_POINT_LIGHT 8
#define NUM_SPOT_LIGHT 8
#define ADD_ILLUM(a, b) { (a).Ambient += (b).Ambient; (a).Diffuse += (b).Diffuse; (a).Specular += (b).Specular; }
#define MAX_CASCADES 4

static const float PI = 3.14159265f;

// Light Structure Definitions
struct FAmbientLightInfo
{
    float4 Color;
    float Intensity;
    float3 Padding;
};

struct FDirectionalLightInfo
{
    float4 Color;
    float3 Direction;
    float Intensity;
};

struct FPointLightInfo
{
    float4 Color;
    float3 Position;
    float Intensity;
    float Range;
    float DistanceFalloffExponent;
    float2 Padding;
};

struct FSpotLightInfo
{
    float4 Color;
    float3 Position;
    float Intensity;
    float Range;
    float DistanceFalloffExponent;
    float InnerConeAngle;
    float OuterConeAngle;
    float AngleFalloffExponent;
    float3 Direction;
};

// reflectance와 곱해지기 전
// 표면에 도달한 빛의 조명 기여량
struct FIllumination
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
};


// Constant Buffers
cbuffer Model : register(b0)
{
    row_major float4x4 World;
}

cbuffer Camera : register(b1)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    float3 ViewWorldLocation;
    float NearClip;
    float FarClip;
}

cbuffer GlobalLightConstant : register(b3)
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
};
cbuffer ClusterSliceInfo : register(b4)
{
    uint ClusterSliceNumX;
    uint ClusterSliceNumY;
    uint ClusterSliceNumZ;
    uint LightMaxCountPerCluster;
    uint SpotLightIntersectOption;
    uint Orthographic;
    uint2 padding;
};
cbuffer LightCountInfo : register(b5)
{
    uint PointLightCount;
    uint SpotLightCount;
    float2 Padding;
};

cbuffer ShadowMapConstants : register(b6)
{
    row_major float4x4 EyeView;        // V_e
    row_major float4x4 EyeProj;        // P_e
    row_major float4x4 EyeViewProjInv; // (P_e * V_e)^(-1)

    row_major float4x4 LightViewP[MAX_CASCADES]; // V_L'
    row_major float4x4 LightProjP[MAX_CASCADES]; // P_L'
    row_major float4x4 LightViewPInv[MAX_CASCADES]; // (V'_L)^(-1)

    row_major float4x4 CameraClipToLightClip;
    
    float4 ShadowParams;               // x=bias, y=slopeBias, z=sharpen, w=reserved
    float4 CascadeSplits;
    float3 LightDirWS;                 // 월드공간 광원 방향
    uint   bInvertedLight;             // 0: normal, 1: inverted

    float4 LightOrthoParams;           // (l, r, b, t)
    float2 ShadowMapSize;              // (Sx, Sy)

    uint bUsePSM; // 0: Simple Ortho, 1: PSM
    uint bUseVSM;
    uint bUsePCF;
    uint bUseCSM;
    float2 pad;
};

StructuredBuffer<int> PointLightIndices : register(t6);
StructuredBuffer<int> SpotLightIndices : register(t7);
StructuredBuffer<FPointLightInfo> PointLightInfos : register(t8);
StructuredBuffer<FSpotLightInfo> SpotLightInfos : register(t9);
Texture2D ShadowMapTexture : register(t10);
Texture2DArray CascadedShadowMapTexture : register(t11);
Texture2D SpotShadowMapTexture : register(t12);
// Per-spot atlas entry buffer (index aligns with SpotLightInfos index)
struct FSpotShadowAtlasEntry
{
    row_major float4x4 View;
    row_major float4x4 Proj;
    float2 AtlasScale;
    float2 AtlasOffset;
    row_major float4x4 PSMMatrix;  // PSM: World → Light Clip
    uint bUsePSM;                   // 1 if PSM enabled
    float3 Padding;
};
StructuredBuffer<FSpotShadowAtlasEntry> SpotShadowAtlasEntries : register(t13);

// Point light shadow cube array and mapping (multiple point lights)
TextureCubeArray PointShadowCubes : register(t14);
StructuredBuffer<uint> PointShadowCubeIndices : register(t15);


uint GetDepthSliceIdx(float ViewZ)
{
    float BottomValue = 1 / log(FarClip / NearClip);
    ViewZ = clamp(ViewZ, NearClip, FarClip);
    return uint(floor(log(ViewZ) * ClusterSliceNumZ * BottomValue - ClusterSliceNumZ * log(NearClip) * BottomValue));
}

uint GetLightIndicesOffset(float3 WorldPos)
{
    float4 ViewPos = mul(float4(WorldPos, 1), View);
    float4 NDC = mul(ViewPos, Projection);
    NDC.xy /= NDC.w;
    //-1 ~ 1 =>0 ~ ScreenXSlideNum
    //-1 ~ 1 =>0 ~ ScreenYSlideNum
    //Near ~ Far => 0 ~ ZSlideNum
    float2 ScreenNorm = saturate(NDC.xy * 0.5f + 0.5f);
    uint2 ClusterXY = uint2(floor(ScreenNorm * float2(ClusterSliceNumX, ClusterSliceNumY)));
    uint ClusterZ = GetDepthSliceIdx(ViewPos.z);
    
    uint ClusterIdx = ClusterXY.x + ClusterXY.y * ClusterSliceNumX + ClusterSliceNumX * ClusterSliceNumY * ClusterZ;
    
    return LightMaxCountPerCluster * ClusterIdx;
}


uint GetPointLightCount(uint LightIndicesOffset)
{
    uint Count = 0;
    for (uint i = 0; i < LightMaxCountPerCluster; i++)
    {
        if (PointLightIndices[LightIndicesOffset + i] >= 0)
        {
            Count++;
        }
    }
    return Count;
}

FPointLightInfo GetPointLight(uint LightIdx)
{
    uint LightInfoIdx = PointLightIndices[LightIdx];
    return PointLightInfos[LightInfoIdx];
}

uint GetSpotLightCount(uint LightIndicesOffset)
{
    uint Count = 0;
    for (uint i = 0; i < LightMaxCountPerCluster; i++)
    {
        if (SpotLightIndices[LightIndicesOffset + i] >= 0)
        {
            Count++;
        }
    }
    return Count;
}
FSpotLightInfo GetSpotLight(uint LightIdx)
{
    uint LightInfoIdx = SpotLightIndices[LightIdx];
    return SpotLightInfos[LightInfoIdx];
}


cbuffer MaterialConstants : register(b2)
{
    float4 Ka; // Ambient color
    float4 Kd; // Diffuse color
    float4 Ks; // Specular color
    float Ns;  // Specular exponent
    float Ni;  // Index of refraction
    float D;   // Dissolve factor
    uint MaterialFlags; // Which textures are available (bitfield)
    float Time;
}


// Textures
Texture2D DiffuseTexture : register(t0);
Texture2D AmbientTexture : register(t1);
Texture2D SpecularTexture : register(t2);
Texture2D NormalTexture : register(t3);
Texture2D AlphaTexture : register(t4);
Texture2D BumpTexture : register(t5);

// 섬도우맵 텍셀 크기(PCF 오프셋용).
static const float2 gShadowTexel = float2(1.0/2048.0, 1.0/2048.0);
SamplerState SamplerWrap : register(s0);
SamplerState SamplerLinearClamp : register(s1);
SamplerState SamplerShadow : register(s2);
SamplerComparisonState SamplerPCF : register(s10);

// Material flags
#define HAS_DIFFUSE_MAP  (1 << 0) // map_Kd
#define HAS_AMBIENT_MAP  (1 << 1) // map_Ka
#define HAS_SPECULAR_MAP (1 << 2) // map_Ks
#define HAS_NORMAL_MAP   (1 << 3) // map_normal
#define HAS_ALPHA_MAP    (1 << 4) // map_d
#define HAS_BUMP_MAP     (1 << 5) // map_Bump

// Vertex Shader Input/Output
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD0;
    float4 Tangent : TANGENT;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float2 Tex : TEXCOORD2;
    float4 WorldTangent : TEXCOORD3;
#if LIGHTING_MODEL_GOURAUD
    float4 AmbientLight : COLOR0;
    float4 DiffuseLight : COLOR1;
    float4 SpecularLight : COLOR2;
#endif
};

struct PS_OUTPUT
{
    float4 SceneColor : SV_Target0;
    float4 NormalData : SV_Target1;
};

float CalculateVSM(float2 Moments, float CurrentDepth, float Bias)
{
    // VSM: configurable smoothing via mip bias
    static const float VSM_MinVariance = 1e-5f; // Floors variance to reduce hard edges
    static const float VSM_BleedReduction = 0.2f; // 0..1, higher reduces light bleeding

    // Clamp depth into [0,1] and apply small bias
    float z = saturate(CurrentDepth - ShadowParams.x);
    float m1 = Moments.x;
    float m2 = Moments.y;

    // Variance and Chebyshev upper bound
    float variance = max(m2 - m1 * m1, VSM_MinVariance);
    float d = z - m1;
    float pMax = saturate(variance / (variance + d * d));

    // Light bleeding reduction
    float visibility = (z <= m1) ? 1.0f : saturate((pMax - VSM_BleedReduction) / (1.0f - VSM_BleedReduction));
    return visibility;
}

// 반환: 가시도(1=조명 통과, 0=완전 그림자)
// Shadow Map 샘플링 함수
/*float CalculateShadowFactor(float3 WorldPos)
{
    // 안전 검사: Light View/Projection Matrix가 Identity면 Shadow 없음
    // (이는 Shadow Map이 아직 렌더링되지 않았음을 의미)
    if (abs(LightViewP[0][0][0] - 1.0f) < 0.001f && abs(LightViewP[0][1][1] - 1.0f) < 0.001f)
    {
        return 1.0f; // Shadow 없음
    }
    
    // 픽셀의 View 공간 깊이를 미리 계산 (CSM 인덱스 판별용)
    float ViewDepth = mul(float4(WorldPos, 1.0f), View).z;
    
    // 최종 그림자 값 (1.0 = 빛, 0.0 = 그림자)
    float Shadow = 1.0f;
    
    if (bUseCSM != 0)  // CSM + VSM(?)
    {
        int CascadeIndex = 0;
        if (ViewDepth > CascadeSplits.x)
            CascadeIndex = 1;
        if (ViewDepth > CascadeSplits.y)
            CascadeIndex = 2;
        if (ViewDepth > CascadeSplits.z)
            CascadeIndex = 3;
        
        float4 LightSpacePos = mul(float4(WorldPos, 1.0f), LightViewP[CascadeIndex]);
        LightSpacePos = mul(LightSpacePos, LightProjP[CascadeIndex]);
        LightSpacePos.xyz /= LightSpacePos.w;
        
        float2 ShadowUV = LightSpacePos.xy * 0.5f + 0.5f;
        ShadowUV.y = 1.0f - ShadowUV.y;
        
        if (ShadowUV.x < 0.0f || ShadowUV.x > 1.0f || ShadowUV.y < 0.0f || ShadowUV.y > 1.0f)
            return 1.0f;
        
        float CurrentDepth = LightSpacePos.z;
        // float ShadowMapDepth = CascadedShadowMapTexture.Sample(SamplerWrap, float3(ShadowUV, CascadeIndex)).r;
        // Shadow = (CurrentDepth - ShadowBias) > ShadowMapDepth ? 0.0f : 1.0f;
        
        static const float VSM_MipBias = 1.25f; // Increase for softer shadows
        float2 Moments = CascadedShadowMapTexture.Sample(SamplerWrap, float3(ShadowUV, CascadeIndex)).rg;
        Shadow = CalculateVSM(Moments, CurrentDepth, VSM_MipBias);
    }
    else // only VSM
    {
        // World Position을 Light 공간으로 변환
        float4 LightSpacePos = mul(float4(WorldPos, 1.0f), LightViewP[0]);
        LightSpacePos = mul(LightSpacePos, LightProjP[0]);
    
        // Perspective Division (Orthographic이면 w=1이지만 일관성을 위해 수행)
        LightSpacePos.xyz /= LightSpacePos.w;
    
        // NDC [-1,1] -> Texture UV [0,1] 변환
        float2 ShadowUV = LightSpacePos.xy * 0.5f + 0.5f;
        ShadowUV.y = 1.0f - ShadowUV.y; // Y축 반전 (DirectX UV 좌표계)
    
        // Shadow Map 범위 밖이면 그림자 없음 (1.0 = 밝음)
        if (ShadowUV.x < 0.0f || ShadowUV.x > 1.0f || ShadowUV.y < 0.0f || ShadowUV.y > 1.0f)
            return 1.0f;
        
        // 현재 픽셀의 Light 공간 Depth
        float CurrentDepth = LightSpacePos.z;
        
        if (bUseVSM == 0)
        {
            // Classic depth compare
            float ShadowMapDepth = ShadowMapTexture.Sample(SamplerWrap, ShadowUV).r;
            Shadow = (CurrentDepth - ShadowParams.x) > ShadowMapDepth ? 0.0f : 1.0f;
        }
        else
        {
            // VSM: configurable smoothing via mip bias
            static const float VSM_MipBias = 1.25f; // Increase for softer shadows

            // Variance Shadow Mapping using precomputed moments (R32G32_FLOAT)
            float2 Moments = ShadowMapTexture.SampleBias(SamplerLinearClamp, ShadowUV, VSM_MipBias).rg;
            Shadow = CalculateVSM(Moments, CurrentDepth, VSM_MipBias);
        }
    }
    
    return Shadow;
}*/
float PSM_Visibility(float3 worldPos)
{
    // 픽셀의 View 공간 깊이를 미리 계산 (CSM 인덱스 판별용)
    float ViewDepth = mul(float4(worldPos, 1.0f), View).z;
    
    float4 sh;

    if (bUsePSM == 1) {
        // PSM: World→Eye NDC→Light
        float4 eyeClip = mul(mul(float4(worldPos, 1.0), EyeView), EyeProj);
        float3 ndc = eyeClip.xyz / max(eyeClip.w, 1e-8f);
        sh = mul(mul(float4(ndc, 1.0), LightViewP[0]), LightProjP[0]);
    }
    else {
        // Simple Ortho: World→Light 직접 변환
        sh = mul(mul(float4(worldPos, 1.0), LightViewP[0]), LightProjP[0]);
    }

    // Clip→NDC→UV, 깊이 (DirectX UV: Y축 반전 필요)
    float2 uv;
    uv.x = sh.x / sh.w * 0.5 + 0.5;     // NDC X: [-1,1] → UV U: [0,1]
    uv.y = 0.5 - sh.y / sh.w * 0.5;     // NDC Y: [-1(bottom),+1(top)] → UV V: [1(bottom),0(top)]
    float  z  = sh.z  / sh.w;

    // 경계 밖이면 취향에 따라 1(밝게) 또는 0(그림자) 처리. 보통 1이 안전.
    if (any(uv < 0.0) || any(uv > 1.0)) return 1.0;

    // 2) 수동 PCF (3x3). 필요시 5x5로 확장 가능.
    //const int R = 1;
    //float sum = 0.0;
    //[unroll] for (int dy=-R; dy<=R; ++dy)
    //    [unroll] for (int dx=-R; dx<=R; ++dx)
    //    {
    //        float2 o  = float2(dx, dy) * gShadowTexel;
    //        float  dz = ShadowMapTexture.SampleLevel(SamplerShadow, uv + o, 0).r;
//
    //        // 비교방향: normal (<) vs inverted (>)
    //        // PSM: 베이킹 시 이미 바이어스 적용 → 직접 비교
    //        // LVP: 샘플링 시 바이어스 적용
    //        bool lit;
    //        if (bUsePSM == 1)
    //        {
    //            // PSM: 월드 공간 바이어스가 ShadowMap.hlsl에 이미 적용됨
    //            lit = (bInvertedLight == 0) ? (z <= dz) : (z >= dz);
    //        }
    //        else
    //        {
    //            // LVP: 샘플링 시 바이어스 적용
    //            lit = (bInvertedLight == 0)
    //                ? ((z - ShadowParams.x) <= dz)      // normal depth (LESS)
    //                : ((z + ShadowParams.x) >= dz);     // reversed depth (GREATER)
    //        }
    //        sum += lit ? 1.0 : 0.0;
    //    }
/*
pass를 변수명으로 쓰지 말자 bool lit 을 bool pass로 썼었다: “식별자 이름” 문제. HLSL(특히 FX 문법 인식)에서 pass는 예약어로 취급되는 경우가 있어서 변수 이름으로 쓰면 파서가 에러
우연찮게 vs로 한번보자는 생각이들어서 봤었는데, vs는 여기에 빨간줄 뜨더라.. 갓 vs..
이거 때문에 3시간 날렸다.. 찾기도 어려운 HLSL 조심 또 조심....
 */

    
    // World Position을 Light 공간으로 변환
    float4 LightSpacePos = mul(float4(worldPos, 1.0f), LightViewP[0]);
    LightSpacePos = mul(LightSpacePos, LightProjP[0]);
    
    // Perspective Division (Orthographic이면 w=1이지만 일관성을 위해 수행)
    LightSpacePos.xyz /= LightSpacePos.w;
    
    // NDC [-1,1] -> Texture UV [0,1] 변환
    float2 ShadowUV = LightSpacePos.xy * 0.5f + 0.5f;
    ShadowUV.y = 1.0f - ShadowUV.y;  // Y축 반전 (DirectX UV 좌표계)
    
    // Shadow Map 범위 밖이면 그림자 없음 (1.0 = 밝음)
    if (ShadowUV.x < 0.0f || ShadowUV.x > 1.0f || ShadowUV.y < 0.0f || ShadowUV.y > 1.0f)
        return 1.0f;
    
    // 현재 픽셀의 Light 공간 Depth
    float CurrentDepth = LightSpacePos.z;
    
    if (bUseCSM != 0)
    {
        int CascadeIndex = 0;
        if (ViewDepth > CascadeSplits.x)    CascadeIndex = 1;
        if (ViewDepth > CascadeSplits.y)    CascadeIndex = 2;
        if (ViewDepth > CascadeSplits.z)    CascadeIndex = 3;
        
        float4 LightSpacePos = mul(float4(worldPos, 1.0f), LightViewP[CascadeIndex]);
        LightSpacePos = mul(LightSpacePos, LightProjP[CascadeIndex]);
        LightSpacePos.xyz /= LightSpacePos.w;
        
        float2 ShadowUV = LightSpacePos.xy * 0.5f + 0.5f;
        ShadowUV.y = 1.0f - ShadowUV.y;
        
        if (ShadowUV.x < 0.0f || ShadowUV.x > 1.0f || ShadowUV.y < 0.0f || ShadowUV.y > 1.0f)
            return 1.0f;
        
        float CurrentDepth = LightSpacePos.z;
        // float ShadowMapDepth = CascadedShadowMapTexture.Sample(SamplerWrap, float3(ShadowUV, CascadeIndex)).r;
        // Shadow = (CurrentDepth - ShadowBias) > ShadowMapDepth ? 0.0f : 1.0f;
        
        static const float VSM_MipBias = 1.25f; // Increase for softer shadows
        float2 Moments = CascadedShadowMapTexture.Sample(SamplerWrap, float3(ShadowUV, CascadeIndex)).rg;
        return CalculateVSM(Moments, CurrentDepth, VSM_MipBias);
    }
    else if (((bUseVSM == 0) && (bUsePCF == 0)) || ((bUseVSM != 0) && (bUsePCF != 0)))
    {
        // Classic depth compare
        float ShadowMapDepth = ShadowMapTexture.Sample(SamplerWrap, ShadowUV).r;
        float Shadow = (CurrentDepth - ShadowParams[0]) > ShadowMapDepth ? 0.0f : 1.0f;
        //float Shadow = (CurrentDepth - 0) > ShadowMapDepth ? 0.0f : 1.0f;
        return Shadow;
    }
    else if (bUsePCF != 0)
    {
        // 3x3 PCF (Percentage-Closer Filtering)
        float Shadow = 0.0f;
        float2 TexelSize;
        uint Width, Height;
        ShadowMapTexture.GetDimensions(Width, Height); // 텍스처의 가로, 세로 가져오기
        TexelSize = 1.0f / float2(Width, Height); // 한 텍셀의 사이즈
    
        // 3x3 커널 순회
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float2 Offset = float2(x, y) * TexelSize;
                // CurrentDepth - bias <= ShadowMapDepth
                // ShadowUV + Offset에서 읽은 depth와 세번째 인자랑 비교
                // 세번째 인자가 더 작으면 true -> 1.0 반환 (빛 받음)
                // 아니라면 false -> 0.0 반환 (그림자)
     
                Shadow += ShadowMapTexture.SampleCmpLevelZero(SamplerPCF, ShadowUV + Offset, CurrentDepth - ShadowParams[0]);
                //Shadow += ShadowMapTexture.SampleCmpLevelZero(SamplerPCF, ShadowUV + Offset, CurrentDepth - 0.0f);
            }
        }
        // 9개 평균 계산하여 부드러운 그림자 값
        Shadow /= 9.0f;
        return Shadow;
    }
    else if(bUseVSM != 0)
    {
        // VSM: configurable smoothing via mip bias
        static const float VSM_MipBias = 1.25f; // Increase for softer shadows
        static const float VSM_MinVariance = 1e-5f; // Floors variance to reduce hard edges
        static const float VSM_BleedReduction = 0.2f; // 0..1, higher reduces light bleeding

        // Variance Shadow Mapping using precomputed moments (R32G32_FLOAT)
        float2 Moments = ShadowMapTexture.SampleBias(SamplerLinearClamp, ShadowUV, VSM_MipBias).rg;

        // Clamp depth into [0,1] and apply small bias
        float z = saturate(CurrentDepth - ShadowParams[0]);
        //float z = saturate(CurrentDepth - 0);
        float m1 = Moments.x;
        float m2 = Moments.y;

        // Variance and Chebyshev upper bound
        float variance = max(m2 - m1 * m1, VSM_MinVariance);
        float d = z - m1;
        float pMax = saturate(variance / (variance + d * d));

        // Light bleeding reduction
        float visibility = (z <= m1) ? 1.0f : saturate((pMax - VSM_BleedReduction) / (1.0f - VSM_BleedReduction));
        return visibility;
    }
    return 1.0f;
}

// 기존 코드가 호출하는 CalculateShadowFactor를 PSM으로 매핑
inline float CalculateShadowFactor(float3 WorldPosition)
{
    return PSM_Visibility(WorldPosition);
}

// Compute spotlight shadow factor from single-spot constants (b7/t12)
float CalculateSpotShadowFactorIndexed(uint spotIndex, float3 worldPos)
{
    // Fetch per-spot view/proj and atlas transform
    FSpotShadowAtlasEntry entry = SpotShadowAtlasEntries[spotIndex];

    // Transform world position to spot light clip space
    float4 clip;
    if (entry.bUsePSM == 1)
    {
        // PSM: World → Light View → Spot Perspective → Crop
        // entry.PSMMatrix = lightView * spotPerspective * crop (전체 변환)
        clip = mul(float4(worldPos, 1.0f), entry.PSMMatrix);
    }
    else
    {
        // Standard: World → View → Proj
        clip = mul(float4(worldPos, 1.0f), entry.View);
        clip = mul(clip, entry.Proj);
    }
    
    if (clip.w <= 0.0f)
        return 1.0f; // behind the light frustum

    float invW = rcp(clip.w);
    float2 uv = float2(clip.x * invW, clip.y * invW) * 0.5f + 0.5f;
    // DirectX UV: flip Y
    uv.y = 1.0f - uv.y;

    // Guard against bleeding by shrinking within the tile by ~1 texel
    uint atlasW, atlasH;
    SpotShadowMapTexture.GetDimensions(atlasW, atlasH);
    float2 atlasTexel = 1.0 / float2(atlasW, atlasH);
    float2 pad = atlasTexel * 1.5f; // tweakable inner padding in atlas UVs
    float2 safeScale  = max(entry.AtlasScale - 2.0f * pad, float2(0.0f, 0.0f));
    float2 safeOffset = entry.AtlasOffset + pad;
    // Map local [0,1] UV into padded atlas rect
    float2 uvAtlas = safeOffset + uv * safeScale;
    if (uvAtlas.x < 0.0f || uvAtlas.x > 1.0f || uvAtlas.y < 0.0f || uvAtlas.y > 1.0f)
        return 1.0f;

    float currentDepth = saturate(clip.z * invW);
    
    // Use proper bias value (matching directional light)
    float bias = 0.00005f;  // Shadow acne 방지

    // PCF path (3x3) using hardware comparison sampler
    if (bUsePCF != 0)
    {
        float shadow = 0.0f;
        // One texel in atlas UV space
        float2 texel = atlasTexel;
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            [unroll]
            for (int y = -1; y <= 1; ++y)
            {
                float2 o = float2(x, y) * texel;
                shadow += SpotShadowMapTexture.SampleCmpLevelZero(SamplerPCF, uvAtlas + o, currentDepth - bias);
            }
        }
        return shadow / 9.0f;
    }

    // VSM path using precomputed moments (R32G32_FLOAT)
    if (bUseVSM != 0)
    {
        // Sample moments without derivatives to allow dynamic loops
        float2 Moments = SpotShadowMapTexture.SampleLevel(SamplerLinearClamp, uvAtlas, 0).rg;

        // Chebyshev upper bound
        static const float VSM_MinVariance = 1e-5f;
        static const float VSM_BleedReduction = 0.2f;

        float z = saturate(currentDepth - bias);
        float m1 = Moments.x;
        float m2 = Moments.y;
        float variance = max(m2 - m1 * m1, VSM_MinVariance);
        float d = z - m1;
        float pMax = saturate(variance / (variance + d * d));
        float visibility = (z <= m1) ? 1.0f : saturate((pMax - VSM_BleedReduction) / (1.0f - VSM_BleedReduction));
        return visibility;
    }

    // Default: binary comparison using regular sampler
    float sd = SpotShadowMapTexture.SampleLevel(SamplerShadow, uvAtlas, 0).r;
    return (currentDepth - bias) > sd ? 0.0f : 1.0f;
}

// Compute point light shadow factor from cube array (no PCF/VSM, no bias)
float CalculatePointShadowFactorIndexed(uint pointIndex, FPointLightInfo info, float3 worldPos)
{
    // Map global point light index to cube array index; 0xFFFFFFFF means not shadowed
    uint cubeIdx = PointShadowCubeIndices[pointIndex];
    if (cubeIdx == 0xFFFFFFFFu)
        return 1.0f;

    float3 V = worldPos - info.Position;
    float dist = length(V);
    if (dist <= 1e-6f)
        return 1.0f; // self
    if (dist >= info.Range)
        return 1.0f; // outside range → treat as lit (no shadow attenuation beyond range)

    float3 dir = V / dist;
    float3 a = abs(dir);
    float3 F;
    if (a.x >= a.y && a.x >= a.z) F = (dir.x > 0.0f) ? float3(1,0,0) : float3(-1,0,0);
    else if (a.y >= a.z)          F = (dir.y > 0.0f) ? float3(0,1,0) : float3(0,-1,0);
    else                          F = (dir.z > 0.0f) ? float3(0,0,1) : float3(0,0,-1);

    // Reconstruct depth value used by the depth buffer for a 90° LH perspective with zn=0.1 and zf=info.Range
    const float zn = 0.1f;
    const float zf = max(zn + 1e-3f, info.Range);
    float z_eye = max(dot(V, F), 1e-4f);
    float C = zf / (zf - zn);
    float D = -zn * zf / (zf - zn);
    float currentDepth = C + D / z_eye;

    float sd = PointShadowCubes.SampleLevel(SamplerWrap, float4(dir, cubeIdx), 0).r;
    return (currentDepth - 0.0001f <= sd) ? 1.0f : 0.0f;
}

// Safe Normalize Util Functions
float2 SafeNormalize2(float2 v)
{
    float Len2 = dot(v, v);
    return Len2 > 1e-12f ? v / sqrt(Len2) : float2(0.0f, 0.0f);
}

float3 SafeNormalize3(float3 v)
{
    float Len2 = dot(v, v);
    return Len2 > 1e-12f ? v / sqrt(Len2) : float3(0.0f, 0.0f, 0.0f);
}

float4 SafeNormalize4(float4 v)
{
    float Len2 = dot(v, v);
    return Len2 > 1e-12f ? v / sqrt(Len2) : float4(0.0f, 0.0f, 0.0f, 0.0f);
}

float GetDeterminant3x3(float3x3 M)
{
    return M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1])
         - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0])
         + M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
}

// 3x3 Matrix Inverse
// Returns identity matrix if determinant is near zero
float3x3 Inverse3x3(float3x3 M)
{
    float det = GetDeterminant3x3(M);
    
    // Singular matrix guard
    if (abs(det) < 1e-8)
        return float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1); // Identity matrix
    
    float invDet = 1.0f / det;
    
    float3x3 inv;
    inv[0][0] = (M[1][1] * M[2][2] - M[1][2] * M[2][1]) * invDet;
    inv[0][1] = (M[0][2] * M[2][1] - M[0][1] * M[2][2]) * invDet;
    inv[0][2] = (M[0][1] * M[1][2] - M[0][2] * M[1][1]) * invDet;
    
    inv[1][0] = (M[1][2] * M[2][0] - M[1][0] * M[2][2]) * invDet;
    inv[1][1] = (M[0][0] * M[2][2] - M[0][2] * M[2][0]) * invDet;
    inv[1][2] = (M[0][2] * M[1][0] - M[0][0] * M[1][2]) * invDet;
    
    inv[2][0] = (M[1][0] * M[2][1] - M[1][1] * M[2][0]) * invDet;
    inv[2][1] = (M[0][1] * M[2][0] - M[0][0] * M[2][1]) * invDet;
    inv[2][2] = (M[0][0] * M[1][1] - M[0][1] * M[1][0]) * invDet;
    
    return inv;
}



// Lighting Calculation Functions
float4 CalculateAmbientLight(FAmbientLightInfo info)
{
    return info.Color * info.Intensity;
}

FIllumination CalculateDirectionalLight(FDirectionalLightInfo Info, float3 WorldNormal, float3 WorldPos, float3 ViewPos)
{
    FIllumination Result = (FIllumination) 0;
    
    // LightDir 또는 WorldNormal이 영벡터면 결과도 전부 영벡터가 되므로 계산 종료 (Nan 방어 코드도 겸함)
    if (dot(Info.Direction, Info.Direction) < 1e-12 || dot(WorldNormal, WorldNormal) < 1e-12)
        return Result;
    
    float3 LightDir = SafeNormalize3(-Info.Direction);
    float NdotL = saturate(dot(WorldNormal, LightDir));
    
    // diffuse illumination
    Result.Diffuse = Info.Color * Info.Intensity * NdotL;
    
#if LIGHTING_MODEL_BLINNPHONG || LIGHTING_MODEL_GOURAUD
    // Specular (Blinn-Phong)
    float3 WorldToCameraVector = SafeNormalize3(ViewPos - WorldPos); // 영벡터면 결과적으로 LightDir와 같은 셈이 됨
    float3 WorldToLightVector = LightDir;
    
    float3 H = SafeNormalize3(WorldToLightVector + WorldToCameraVector); // H가 영벡터면 Specular도 영벡터
    float CosTheta = saturate(dot(WorldNormal, H));
    float Spec = CosTheta < 1e-6 ? 0.0f : ((Ns + 8.0f) / (8.0f * PI)) * pow(CosTheta, Ns); // 0^0 방지를 위해 이렇게 계산함
    Result.Specular = Info.Color * Info.Intensity * Spec;
#endif
    
    return Result;
}

FIllumination CalculatePointLight(FPointLightInfo Info, float3 WorldNormal, float3 WorldPos, float3 ViewPos)
{
    FIllumination Result = (FIllumination) 0;
    
    float3 LightDir = Info.Position - WorldPos;
    float Distance = length(LightDir);
    
    // 거리나 범위가 너무 작거나, 거리가 범위 밖이면 조명 기여 없음 (Nan 방어 코드도 겸함)
    if (Distance < 1e-6 || Info.Range < 1e-6 || Distance > Info.Range)
        return Result;
    
    LightDir = SafeNormalize3(LightDir);
    float NdotL = saturate(dot(WorldNormal, LightDir));
    // attenuation based on distance: (1 - d / R)^n
    float Attenuation = pow(saturate(1.0f - Distance / Info.Range), Info.DistanceFalloffExponent);
    
    // diffuse illumination
    Result.Diffuse = Info.Color * Info.Intensity * NdotL * Attenuation;
    
#if LIGHTING_MODEL_BLINNPHONG || LIGHTING_MODEL_GOURAUD
    // Specular (Blinn-Phong)
    float3 WorldToCameraVector = SafeNormalize3(ViewPos - WorldPos); // 영벡터면 결과적으로 LightDir와 같은 셈이 됨
    float3 WorldToLightVector = LightDir;
    
    float3 H = SafeNormalize3(WorldToLightVector + WorldToCameraVector); // H가 영벡터면 Specular도 영벡터
    float CosTheta = saturate(dot(WorldNormal, H));
    float Spec = CosTheta < 1e-6 ? 0.0f : ((Ns + 8.0f) / (8.0f * PI)) * pow(CosTheta, Ns); // 0^0 방지를 위해 이렇게 계산함
    Result.Specular = Info.Color * Info.Intensity * Spec * Attenuation;
#endif
    
    return Result;
}

FIllumination CalculateSpotLight(FSpotLightInfo Info, float3 WorldNormal, float3 WorldPos, float3 ViewPos)
{
    FIllumination Result = (FIllumination) 0;
    
    float3 LightDir = Info.Position - WorldPos;
    float Distance = length(LightDir);
    
    // 거리나 범위가 너무 작거나, 거리가 범위 밖이면 조명 기여 없음 (Nan 방어 코드도 겸함)
    if (Distance < 1e-6 || Info.Range < 1e-6 || Distance > Info.Range)
        return Result;
    
    LightDir = SafeNormalize3(LightDir);
    float3 SpotDir = SafeNormalize3(Info.Direction); // SpotDIr이 영벡터면 (CosAngle < CosOuter)에 걸려 0벡터 반환
    
    float CosAngle = dot(-LightDir, SpotDir);
    float CosOuter = cos(Info.OuterConeAngle);
    float CosInner = cos(Info.InnerConeAngle);
    if (CosAngle - CosOuter <= 1e-6)
        return Result;
    
    float NdotL = saturate(dot(WorldNormal, LightDir));

    // attenuation based on distance: (1 - d / R)^n
    float AttenuationDistance = pow(saturate(1.0f - Distance / Info.Range), Info.DistanceFalloffExponent);
    
    float AttenuationAngle = 0.0f;
    if (CosAngle >= CosInner || CosInner - CosOuter <= 1e-6)
    {
        AttenuationAngle = 1.0f;
    }
    else
    {
        AttenuationAngle = pow(saturate((CosAngle - CosOuter) / (CosInner - CosOuter)), Info.AngleFalloffExponent);
    }
    
    Result.Diffuse = Info.Color * Info.Intensity * NdotL * AttenuationDistance * AttenuationAngle;
    
#if LIGHTING_MODEL_BLINNPHONG || LIGHTING_MODEL_GOURAUD
    // Specular (Blinn-Phong)
    float3 WorldToCameraVector = SafeNormalize3(ViewPos - WorldPos);
    float3 WorldToLightVector = LightDir;
    
    float3 H = SafeNormalize3(WorldToLightVector + WorldToCameraVector); // H가 영벡터면 Specular도 영벡터
    float CosTheta = saturate(dot(WorldNormal, H));
    float Spec = CosTheta < 1e-6 ? 0.0f : ((Ns + 8.0f) / (8.0f * PI)) * pow(CosTheta, Ns); // 0^0 방지를 위해 이렇게 계산함
    Result.Specular = Info.Color * Info.Intensity * Spec * AttenuationDistance * AttenuationAngle;
#endif
    
    return Result;
}


float3 ComputeNormalMappedWorldNormal(float2 UV, float3 WorldNormal, float4 WorldTangent)
{
    float3 BaseNormal = SafeNormalize3(WorldNormal);

    // Tangent가 비정상(0 길이)이면 노말 맵 적용 포기하고 메시 노말 사용
    float TangentLen2 = dot(WorldTangent.xyz, WorldTangent.xyz);
    if (TangentLen2 <= 1e-8f)
    {
        return BaseNormal;
    }
    // 노말 맵 텍셀 샘플링 [0,1]
    float3 Encoded = NormalTexture.Sample(SamplerWrap, UV).xyz;
    // [0,1] -> [-1,1]로 매핑해서 탄젠트 공간 노말을 복원한다.
    float3 TangentSpaceNormal = SafeNormalize3(Encoded * 2.0f - 1.0f);

    // VS로 넘어온 월드 탄젠트를 정규화
    float3 T = WorldTangent.xyz / sqrt(TangentLen2);
    // TBN이 올바른 방향이 되도록 저장해둔 좌우손성으로 B 복원
    float Handedness = WorldTangent.w;
    float3 B = SafeNormalize3(cross(BaseNormal, T) * Handedness);

    float3x3 TBN = float3x3(T, B, BaseNormal);
    // 로컬 공간의 탄젠트를 월드 공간으로 보냄
    return SafeNormalize3(mul(TangentSpaceNormal, TBN));

}
// Vertex Shader
PS_INPUT Uber_VS(VS_INPUT Input)
{
    PS_INPUT Output;
    
    Output.WorldPosition = mul(float4(Input.Position, 1.0f), World).xyz;
    Output.Position = mul(mul(mul(float4(Input.Position, 1.0f), World), View), Projection);
    float3x3 World3x3 = (float3x3) World;
    Output.WorldNormal = SafeNormalize3(mul(Input.Normal, transpose(Inverse3x3(World3x3))));
    float3 WorldTangent = SafeNormalize3(mul(Input.Tangent.xyz, (float3x3) World));
    Output.WorldTangent = float4(WorldTangent, Input.Tangent.w);
    Output.Tex = Input.Tex;
    
#if LIGHTING_MODEL_GOURAUD
    // Calculate lighting in vertex shader (Gouraud)
    // Accumulate light only; material and textures are applied in pixel stage
    FIllumination Illumination = (FIllumination)0;
    
    // 1. Ambient Light
    Illumination.Ambient = CalculateAmbientLight(Ambient);
    
    // 2. Directional Light
    ADD_ILLUM(Illumination, CalculateDirectionalLight(Directional, Output.WorldNormal, Output.WorldPosition, ViewWorldLocation))

    // 3. Point Lights
    uint LightIndicesOffset = GetLightIndicesOffset(Output.WorldPosition);
    uint PointLightCount = GetPointLightCount(LightIndicesOffset);
    for (uint i = 0; i < PointLightCount; i++)
    {        
        FPointLightInfo PointLight = GetPointLight(LightIndicesOffset + i);
        ADD_ILLUM(Illumination, CalculatePointLight(PointLight, Output.WorldNormal, Output.WorldPosition, ViewWorldLocation));
    }

    // 4.Spot Lights 
    uint SpotLightCount = GetSpotLightCount(LightIndicesOffset);
     for (uint j = 0; j < SpotLightCount; j++)
    {        
        FSpotLightInfo SpotLight = GetSpotLight(LightIndicesOffset + j);
        ADD_ILLUM(Illumination, CalculateSpotLight(SpotLight, Output.WorldNormal, Output.WorldPosition, ViewWorldLocation));
    }
    
    // Assign to output
    Output.AmbientLight = Illumination.Ambient;
    Output.DiffuseLight = Illumination.Diffuse;
    Output.SpecularLight = Illumination.Specular;
#endif
    
    return Output;
}

// Pixel Shader
PS_OUTPUT Uber_PS(PS_INPUT Input)
{
    PS_OUTPUT Output;
    float4 finalPixel = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float2 UV = Input.Tex;
    float3 ShadedWorldNormal = SafeNormalize3(Input.WorldNormal);
    
    if (MaterialFlags & HAS_NORMAL_MAP)
    {
        ShadedWorldNormal = ComputeNormalMappedWorldNormal(UV, Input.WorldNormal, Input.WorldTangent);
        // else: Tangent가 유효하지 않으면 NormalBase 유지
    }
    // Sample textures
    float4 ambientColor = Ka;
    if (MaterialFlags & HAS_AMBIENT_MAP)
    {
        ambientColor *= AmbientTexture.Sample(SamplerWrap, UV);
    }
    else if (MaterialFlags & HAS_DIFFUSE_MAP)
    {
        // If no ambient map, but diffuse map exists, use diffuse map for ambient color
        ambientColor *= DiffuseTexture.Sample(SamplerWrap, UV);
    }
    
    float4 diffuseColor = Kd;
    if (MaterialFlags & HAS_DIFFUSE_MAP)
    {
        diffuseColor *= DiffuseTexture.Sample(SamplerWrap, UV);
    }
    
    float4 specularColor = Ks;
    if (MaterialFlags & HAS_SPECULAR_MAP)
    {
        specularColor *= SpecularTexture.Sample(SamplerWrap, UV);
    }
    
    
#if LIGHTING_MODEL_GOURAUD
    // Use pre-calculated vertex lighting; apply diffuse material/texture per-pixel
    //finalPixel.rgb = Input.AmbientLight.rgb * ambientColor.rgb + Input.DiffuseLight.rgb * diffuseColor.rgb + Input.SpecularLight.rgb * specularColor.rgb;

    // Shadow Map 적용 (Pixel Shader에서 그림자 계산)
    float ShadowFactor = CalculateShadowFactor(Input.WorldPosition);
    float3 shadedDiffuse = Input.DiffuseLight.rgb * ShadowFactor;
    float3 shadedSpecular = Input.SpecularLight.rgb * ShadowFactor;
    finalPixel.rgb = Input.AmbientLight.rgb * ambientColor.rgb + shadedDiffuse * diffuseColor.rgb + shadedSpecular * specularColor.rgb;
    
#elif LIGHTING_MODEL_LAMBERT || LIGHTING_MODEL_BLINNPHONG
    // Calculate lighting in pixel shader
    FIllumination Illumination = (FIllumination)0;
    float3 N = ShadedWorldNormal;
    
    // 1. Ambient Light
    Illumination.Ambient = CalculateAmbientLight(Ambient);


    //ADD_ILLUM(Illumination, CalculateDirectionalLight(Directional, N, Input.WorldPosition, ViewWorldLocation));
    // 2. Directional Light (Shadow Map 적용)
    float ShadowFactor = CalculateShadowFactor(Input.WorldPosition);
    //float ShadowFactor = 1.0;  // 강제 밝게
    FIllumination DirectionalIllum = CalculateDirectionalLight(Directional, N, Input.WorldPosition, ViewWorldLocation);
    DirectionalIllum.Diffuse *= ShadowFactor;
    DirectionalIllum.Specular *= ShadowFactor;
    ADD_ILLUM(Illumination, DirectionalIllum);
    

    
    // 3. Point Lights
    uint LightIndicesOffset = GetLightIndicesOffset(Input.WorldPosition);
    uint PointLightCount = GetPointLightCount(LightIndicesOffset);
    [loop] for (uint i = 0; i < PointLightCount ; i++)
    {
        uint PointIndex = PointLightIndices[LightIndicesOffset + i];
        FPointLightInfo PointLight = PointLightInfos[PointIndex];
        FIllumination P = CalculatePointLight(PointLight, N, Input.WorldPosition, ViewWorldLocation);
        float pf = CalculatePointShadowFactorIndexed(PointIndex, PointLight, Input.WorldPosition);
        P.Diffuse *= pf;
        P.Specular *= pf;
        ADD_ILLUM(Illumination, P);
    }
    
    // 4. Spot Lights
    uint SpotLightCount = GetSpotLightCount(LightIndicesOffset);
    [loop] for (uint j = 0; j < SpotLightCount ; j++)
    {
        // Fetch global index and info
        uint SpotIndex = SpotLightIndices[LightIndicesOffset + j];
        FSpotLightInfo SpotLight = SpotLightInfos[SpotIndex];
        FIllumination S = CalculateSpotLight(SpotLight, N, Input.WorldPosition, ViewWorldLocation);
        
        float sf = CalculateSpotShadowFactorIndexed(SpotIndex, Input.WorldPosition);
        S.Diffuse *= sf;
        S.Specular *= sf;
        ADD_ILLUM(Illumination, S);
    }
    
    finalPixel.rgb = Illumination.Ambient.rgb * ambientColor.rgb + Illumination.Diffuse.rgb * diffuseColor.rgb + Illumination.Specular.rgb * specularColor.rgb;
    
#elif LIGHTING_MODEL_NORMAL
    float3 EncodedWorldNormal = ShadedWorldNormal * 0.5f + 0.5f;
    finalPixel.rgb = EncodedWorldNormal;
    
#else
    // Fallback: simple textured rendering (like current TexturePS.hlsl)
    finalPixel.rgb = diffuseColor.rgb + ambientColor.rgb;
#endif
    
    // Alpha handling
#if LIGHTING_MODEL_NORMAL
    finalPixel.a = 1.0f;
#else
    // 1. Diffuse Map 있으면 그 alpha 사용, 없으면 1.0
    float alpha = (MaterialFlags & HAS_DIFFUSE_MAP) ? diffuseColor.a : 1.0f;
    
    // 2. Alpha Map 따로 있으면 곱해줌.
    if (MaterialFlags & HAS_ALPHA_MAP)
    {
        alpha *= AlphaTexture.Sample(SamplerWrap, UV).r;
    }
    
    // 3. D 곱해서 최종 alpha 결정
    finalPixel.a = D * alpha;
#endif
    Output.SceneColor = finalPixel;
    
    // Encode normal for deferred rendering
    float3 encodedNormal = SafeNormalize3(ShadedWorldNormal) * 0.5f + 0.5f;
    Output.NormalData = float4(encodedNormal, 1.0f);
    
    return Output;
}
