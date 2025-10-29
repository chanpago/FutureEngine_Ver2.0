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
#define MAX_CASCADES 8

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

    float ResolutionScale;
    float Bias;
    float SlopeBias;
    float Sharpen;
};

struct FPointLightInfo
{
    float4 Color;
    float3 Position;
    float Intensity;
    float Range;
    float DistanceFalloffExponent;

    float ResolutionScale;
    float Bias;
    float SlopeBias;
    float Sharpen;
    
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

    float ResolutionScale;
    float Bias;
    float SlopeBias;
    float Sharpen;
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
    
    float4 CascadeSplits[MAX_CASCADES / 4];
    uint NumCascades;
    float3 pad0;

    row_major float4x4 CameraClipToLightClip;
    
    float4 ShadowParams;               // x=bias, y=slopeBias, z=sharpen, w=reserved
    float4 NotUsedCascadeSplits;
    float3 LightDirWS;                 // 월드공간 광원 방향
    uint   bInvertedLight;             // 0: normal, 1: inverted

    float4 LightOrthoParams;           // (l, r, b, t)
    float2 ShadowMapSize;              // (Sx, Sy)

    uint bUsePSM; // 0: Simple Ortho, 1: PSM
    uint bUseVSM;
    uint bUsePCF;
    uint bUseCSM;
    float2 pad1;
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

// Point light shadow 3-Tier system
struct FPointShadowTierMapping
{
    uint Tier;           // 0=Low(512), 1=Mid(1024), 2=High(2048), 0xFFFFFFFF=no shadow
    uint TierLocalIndex; // 0-7 within the tier's cube array
};
StructuredBuffer<FPointShadowTierMapping> PointShadowTierMappings : register(t14);

// Low Tier (512x512) - Slots 15-17
TextureCubeArray PointShadowLowTierCubes : register(t15);
Texture2DArray PointShadowLowTier2DArray : register(t16);
Texture2DArray PointShadowLowTierMoments : register(t17);


// Mid Tier (1024x1024) - Slots 18-20
TextureCubeArray PointShadowMidTierCubes : register(t18);
Texture2DArray PointShadowMidTier2DArray : register(t19);
Texture2DArray PointShadowMidTierMoments : register(t20);

// High Tier (2048x2048) - Slots 21-23
TextureCubeArray PointShadowHighTierCubes : register(t21);
Texture2DArray PointShadowHighTier2DArray : register(t22);
Texture2DArray PointShadowHighTierMoments : register(t23);

float3 SafeNormalize3(float3 v);

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

float CalculateVSM(float2 Moments, float CurrentDepth, float Bias, float Sharpen)
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

    float sharpenPower = 1.0f + Sharpen * 15.0f; // Sharpen 0~1 -> Power 1~16
    pMax = pow(saturate(pMax), sharpenPower);
    
    // Light bleeding reduction
    float visibility = (z <= m1) ? 1.0f : saturate((pMax - VSM_BleedReduction) / (1.0f - VSM_BleedReduction));
    return visibility;
}

// -----------------------------------------------------------------------------
// Generic shadow helpers to minimize duplication across light types
// -----------------------------------------------------------------------------
// 2D PCF using hardware comparison sampler (3x3)
inline float ShadowPCF2D(Texture2D DepthTex, SamplerComparisonState Comp, float2 uv, float currentDepth)
{
    uint w, h; DepthTex.GetDimensions(w, h);
    float2 texel = 1.0 / float2(w, h);
    float sum = 0.0;
    [unroll] for (int dy = -1; dy <= 1; ++dy)
    [unroll] for (int dx = -1; dx <= 1; ++dx)
    {
        float2 o = float2(dx, dy) * texel;
        sum += DepthTex.SampleCmpLevelZero(Comp, uv + o, currentDepth);
    }
    return sum / 9.0f;
}

// 2D VSM from moments texture
inline float ShadowVSM2D(Texture2D MomentsTex, SamplerState Samp, float2 uv, float currentDepth, float mipBias, float Sharpen)
{
    float2 mom = MomentsTex.SampleBias(Samp, uv, mipBias).rg;
    return CalculateVSM(mom, currentDepth, mipBias, Sharpen);
}

// 2D array VSM (e.g., cascades)
inline float ShadowVSM2DArray(Texture2DArray MomentsTex, SamplerState Samp, float3 uvLayer, float currentDepth, float mipBias, float Sharpen)
{
    // Prefer clamp sampling and explicit LOD for stability across cascades
    static const float VSM_LOD = 1.0f; // tweakable softness via lower-res moments
    float2 mom = MomentsTex.SampleLevel(Samp, uvLayer, VSM_LOD).rg;
    return CalculateVSM(mom, currentDepth, mipBias, Sharpen);
}

// 2D array PCF (3x3)
inline float ShadowPCF2DArray(Texture2DArray DepthTex, SamplerComparisonState Comp, float3 uvLayer, float currentDepth)
{
    uint w, h, layers; DepthTex.GetDimensions(w, h, layers);
    float2 texel = 1.0 / float2(max(1u,w), max(1u,h));
    float sum = 0.0;
    [unroll] for (int dy = -1; dy <= 1; ++dy)
    [unroll] for (int dx = -1; dx <= 1; ++dx)
    {
        float2 o = float2(dx, dy) * texel;
        sum += DepthTex.SampleCmpLevelZero(Comp, float3(uvLayer.xy + o, uvLayer.z), currentDepth - 0.0001f);
    }
    return sum / 9.0f;
}

float SampleCascadeShadowValue(float3 WorldPosition, int Cascade)
{
    float4 LightPosition = mul(float4(WorldPosition, 1.0f), LightViewP[Cascade]);
    LightPosition = mul(LightPosition, LightProjP[Cascade]);
    LightPosition.xyz /= LightPosition.w;
    float2 ShadowTextureUv = LightPosition.xy * 0.5f + 0.5f;
    ShadowTextureUv.y = 1.0f - ShadowTextureUv.y;
    if (ShadowTextureUv.x < 0.0f || ShadowTextureUv.x > 1.0f || ShadowTextureUv.y < 0.0f || ShadowTextureUv.y > 1.0f)
        return 1.0f;
    float CurrentDepth = LightPosition.z;

    float Bias = Directional.Bias;
    if (bUsePCF)
    {
        uint Width, Height, Layers; CascadedShadowMapTexture.GetDimensions(Width, Height, Layers);
        float2 TexelSize = 1.0 / float2(max(1u,Width), max(1u,Height));
        float FilteredSum = 0.0f;
        [unroll] for (int dy = -1; dy <= 1; ++dy)
            [unroll] for (int dx = -1; dx <= 1; ++dx)
            {
                float2 Offset = float2(dx, dy) * TexelSize;
                FilteredSum += CascadedShadowMapTexture.SampleCmpLevelZero(SamplerPCF,
                    float3(ShadowTextureUv + Offset, Cascade), CurrentDepth - Bias);
            }
        return FilteredSum / 9.0f;
    }
    else if (bUseVSM)
    {
        static const float VSMMipBias = 1.25f;
        return ShadowVSM2DArray(CascadedShadowMapTexture, SamplerLinearClamp, float3(ShadowTextureUv, Cascade),
            CurrentDepth, VSMMipBias, Directional.Sharpen);
    }
    else
    {
        float ShadowMapDepth = CascadedShadowMapTexture.SampleLevel(SamplerLinearClamp, float3(ShadowTextureUv, Cascade), 0).r;
        return (CurrentDepth - Bias) <= ShadowMapDepth ? 1.0f : 0.0f;
    }
}

float SampleShadowCSM(float3 WorldPosition, float ViewDepth)
{
    int CascadeIndex = 0;
    [unroll] for (int i = 0; i < MAX_CASCADES; ++i)
    {
        float splitValue = CascadeSplits[i / 4][i % 4];
        if (ViewDepth > splitValue)
        {
            CascadeIndex++;
        }
    }
       
    // Cascade fade to hide transitions
    float NearSplit = 0.0f;
    float FarSplit = 0.0f;
    if (CascadeIndex == 0) { NearSplit = 0.0f;               FarSplit = CascadeSplits[0][0]; }
    if (CascadeIndex == 1) { NearSplit = CascadeSplits[0][0];    FarSplit = CascadeSplits[0][1]; }
    if (CascadeIndex == 2) { NearSplit = CascadeSplits[0][1];    FarSplit = CascadeSplits[0][2]; }
    if (CascadeIndex == 3) { NearSplit = CascadeSplits[0][2];    FarSplit = CascadeSplits[0][3]; }
    if (CascadeIndex == 4) { NearSplit = CascadeSplits[0][3];    FarSplit = CascadeSplits[1][0]; }
    if (CascadeIndex == 5) { NearSplit = CascadeSplits[1][0];    FarSplit = CascadeSplits[1][1]; }
    if (CascadeIndex == 6) { NearSplit = CascadeSplits[1][1];    FarSplit = CascadeSplits[1][2]; }
    if (CascadeIndex == 7) { NearSplit = CascadeSplits[1][2];    FarSplit = CascadeSplits[1][3]; }
    

    // Blend width is a fraction of the cascade depth range
    const float CascadeBlendFraction = 0.15f;
    float BlendWidth = max(1e-4f, (FarSplit - NearSplit) * CascadeBlendFraction);
    float BlendFactor = saturate((FarSplit - ViewDepth) / BlendWidth); // 1 → inside cascade, 0 → at far boundary

    float CurrentCascadeShadow = SampleCascadeShadowValue(WorldPosition, CascadeIndex);
    if (CascadeIndex < 7)
    {
        float NextCascadeShadow = SampleCascadeShadowValue(WorldPosition, CascadeIndex + 1);
        return lerp(NextCascadeShadow, CurrentCascadeShadow, BlendFactor);
    }
    return CurrentCascadeShadow;
}

float PSM_Visibility(float3 worldPos)
{
    if (bUseCSM != 0)
    {
        float ViewDepth = mul(float4(worldPos, 1.0f), View).z;
        return SampleShadowCSM(worldPos, ViewDepth);
    }
    
    // 픽셀의 View 공간 깊이를 미리 계산 (CSM 인덱스 판별용)
    float ViewDepth = mul(float4(worldPos, 1.0f), View).z;
    
    float4 sh;
    float2 uv;
    if (bUsePSM == 1) {
        // LiSPSM: Clip → LightClip using precomputed warp
        float4 clipPos = mul(float4(worldPos, 1.0), View);
        clipPos = mul(clipPos, Projection);
        sh = mul(clipPos, CameraClipToLightClip);
        // Derive UV/depth from sh (LiSPSM uses UV=(ndc.y, ndc.z), depth=ndc.z)
        float invW = rcp(max(sh.w, 1e-8f));
        float3 ndc = sh.xyz * invW;
        uv = ndc.yz * 0.5f + 0.5f;
        uv.y = 1.0f - uv.y;   // DX UV flip
        float  z  = saturate(ndc.z);
    }
    else {
        // Simple Ortho: World→Light directly
        sh = mul(mul(float4(worldPos, 1.0), LightViewP[0]), LightProjP[0]);
        uv.x = sh.x / sh.w * 0.5 + 0.5;     // NDC X: [-1,1] → UV U: [0,1]
        uv.y = 0.5 - sh.y / sh.w * 0.5;     // NDC Y: [-1(bottom),+1(top)] → UV V: [1(bottom),0(top)]
        float  z  = sh.z  / sh.w;
    }

    // Derive UV/depth from sh (LiSPSM uses UV=(ndc.y, ndc.z), depth=ndc.z)


    if (any(uv < 0.0) || any(uv > 1.0)) return 1.0;
    
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

    if (((bUseVSM == 0) && (bUsePCF == 0)) || ((bUseVSM != 0) && (bUsePCF != 0)))
    {
        // Classic depth compare
        float sd = ShadowMapTexture.SampleLevel(SamplerWrap, ShadowUV, 0).r;
        return (CurrentDepth - Directional.Bias <= sd) ? 1.0f : 0.0f;
    }
    else if (bUsePCF != 0)
    {
        return ShadowPCF2D(ShadowMapTexture, SamplerPCF, ShadowUV, CurrentDepth - Directional.Bias);
    }
    else if (bUseVSM != 0)
    {
        static const float VSM_MipBias = 1.25f; // tweakable softness
        return ShadowVSM2D(ShadowMapTexture, SamplerLinearClamp, ShadowUV, CurrentDepth, VSM_MipBias, Directional.Sharpen);
    }
    return 1.0f;
}

// Variant that applies slope-scaled bias using world normal
float PSM_Visibility_WithNormal(float3 worldPos, float3 worldNormal)
{
    float ViewDepth = mul(float4(worldPos, 1.0f), View).z;
    float2 ShadowUV;
    float CurrentDepth;
    
    if (bUseCSM != 0)
    {
        return SampleShadowCSM(worldPos, ViewDepth);
    }
    if (bUsePSM > 0.5f)
    {
        // LiSPSM Path: World -> EyeSpace -> LightSpace transformation
        float4 eyeSpace = mul(float4(worldPos, 1.0f), View);
        
        // Apply LiSPSM transformation (EyeSpace -> LightSpace)
        float4 lightClip = mul(eyeSpace, CameraClipToLightClip);
        
        // Perspective divide
        float invW = rcp(max(abs(lightClip.w), 1e-8f));
        float3 ndc = lightClip.xyz * invW;
        
        // NDC to UV
        ShadowUV = ndc.xy * 0.5f + 0.5f;
        ShadowUV.y = 1.0f - ShadowUV.y;  // DX UV flip
        
        // LiSPSM depth - DO NOT saturate, perspective depth can exceed [0,1]
        CurrentDepth = ndc.z;
    }
    else
    {
        // Standard Ortho Path: World -> LightView -> LightProj
        float4 LightSpacePos = mul(float4(worldPos, 1.0f), LightViewP[0]);
        LightSpacePos = mul(LightSpacePos, LightProjP[0]);
        
        float invW = rcp(max(abs(LightSpacePos.w), 1e-8f));
        ShadowUV = LightSpacePos.xy * invW * 0.5f + 0.5f;
        ShadowUV.y = 1.0f - ShadowUV.y;
        
        CurrentDepth = LightSpacePos.z * invW;
    }
    
    // Out-of-bounds check
    if (any(ShadowUV < 0.0f) || any(ShadowUV > 1.0f))
        return 1.0f;
    
    // Slope-scaled bias
    float3 Ldir = SafeNormalize3(LightDirWS);
    float ndotl_abs = abs(dot(worldNormal, Ldir));
    float combinedBias = Directional.Bias + Directional.SlopeBias * (1.0f - ndotl_abs);
    
    if (((bUseVSM == 0) && (bUsePCF == 0)) || ((bUseVSM != 0) && (bUsePCF != 0)))
    {
        // Classic depth compare
        float sd = ShadowMapTexture.SampleLevel(SamplerWrap, ShadowUV, 0).r;
        return (CurrentDepth - combinedBias <= sd) ? 1.0f : 0.0f;
    }
    else if (bUsePCF > 0.5f)
    {
        return ShadowPCF2D(ShadowMapTexture, SamplerPCF, ShadowUV, CurrentDepth - combinedBias);
    }
    else if (bUseVSM > 0.5f)
    {
        static const float VSM_MipBias = 1.25f;
        return ShadowVSM2D(ShadowMapTexture, SamplerLinearClamp, ShadowUV, CurrentDepth - combinedBias, VSM_MipBias, Directional.Sharpen);
    }
    
    return 1.0f;
}

// Maintain original helper (no normal) and a normal-aware variant
inline float CalculateDirectionalShadowFactor(float3 WorldPosition)
{
    return PSM_Visibility(WorldPosition);
}
inline float CalculateDirectionalShadowFactorWithNormal(float3 WorldPosition, float3 WorldNormal)
{
    return PSM_Visibility_WithNormal(WorldPosition, WorldNormal);
}

// Compute spotlight shadow factor from single-spot constants (b7/t12)
float CalculateSpotShadowFactorIndexed(uint spotIndex, float3 worldPos, float3 worldNormal)
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
    
    // Slope-scaled bias: base + slopeScale * (1 - |N·L|)
    FSpotLightInfo spotInfo = SpotLightInfos[spotIndex];
    float3 L = SafeNormalize3(spotInfo.Position - worldPos);
    float ndotl_abs = abs(dot(worldNormal, L));
    float bias = spotInfo.Bias + spotInfo.SlopeBias * (1.0f - ndotl_abs);

    // PCF path (3x3) using hardware comparison sampler
    if (bUsePCF != 0)
    {
        return ShadowPCF2D(SpotShadowMapTexture, SamplerPCF, uvAtlas, currentDepth - bias);
    }

    // VSM path using precomputed moments (R32G32_FLOAT)
    if (bUseVSM != 0)
    {
        static const float VSM_MipBias = 0.0f; // could increase for softer
        return ShadowVSM2D(SpotShadowMapTexture, SamplerLinearClamp, uvAtlas, currentDepth, VSM_MipBias, spotInfo.Sharpen);
    }

    // Default: binary comparison using regular sampler
    float sd = SpotShadowMapTexture.SampleLevel(SamplerShadow, uvAtlas, 0).r;
    return (currentDepth - bias) > sd ? 0.0f : 1.0f;
}

// Compute point light shadow factor from cube array (no PCF/VSM, no bias)
float CalculatePointShadowFactorIndexed(uint pointIndex, FPointLightInfo info, float3 worldPos, float3 worldNormal)
{
    // Read tier mapping; 0xFFFFFFFF tier means not shadowed
    FPointShadowTierMapping mapping = PointShadowTierMappings[pointIndex];
    if (mapping.Tier == 0xFFFFFFFFu)
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
    int faceIndex;
    float2 uv;
    if (a.x >= a.y && a.x >= a.z)
    {
        if (dir.x > 0.0f) { F = float3(1,0,0);  uv = float2(-dir.z,  dir.y) / a.x; faceIndex = 0; }
        else               { F = float3(-1,0,0); uv = float2( dir.z,  dir.y) / a.x; faceIndex = 1; }
    }
    else if (a.y >= a.z)
    {
        if (dir.y > 0.0f) { F = float3(0,1,0);  uv = float2( dir.x, -dir.z) / a.y; faceIndex = 2; }
        else               { F = float3(0,-1,0); uv = float2( dir.x,  dir.z) / a.y; faceIndex = 3; }
    }
    else
    {
        if (dir.z > 0.0f) { F = float3(0,0,1);  uv = float2( dir.x,  dir.y) / a.z; faceIndex = 4; }
        else               { F = float3(0,0,-1); uv = float2(-dir.x,  dir.y) / a.z; faceIndex = 5; }
    }
    uv = uv * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y; // DirectX UV flip

    // Reconstruct depth value used by the depth buffer for a 90° LH perspective with zn=0.1 and zf=info.Range
    const float zn = 0.1f;
    const float zf = max(zn + 1e-3f, info.Range);
    float z_eye = max(dot(V, F), 1e-4f);
    float C = zf / (zf - zn);
    float D = -zn * zf / (zf - zn);
    float currentDepth = C + D / z_eye;

    // Tier-local cube index and layer index
    uint cubeIdx = mapping.TierLocalIndex;
    uint layer = cubeIdx * 6 + (uint)faceIndex;

    // slope-scaled bias for point lights as well
    float3 Lp = SafeNormalize3(info.Position - worldPos);
    float bias = info.Bias + info.SlopeBias * (1.0f - abs(dot(worldNormal, Lp)));

    // VSM path over moments 2D array
    if (bUseVSM != 0)
    {
        static const float VSM_MipBias = 0.0f;
        if (mapping.Tier == 0) // Low Tier
            return ShadowVSM2DArray(PointShadowLowTierMoments, SamplerLinearClamp, float3(uv, layer), currentDepth - bias, VSM_MipBias, info.Sharpen);
        else if (mapping.Tier == 1) // Mid Tier
            return ShadowVSM2DArray(PointShadowMidTierMoments, SamplerLinearClamp, float3(uv, layer), currentDepth - bias, VSM_MipBias, info.Sharpen);
        else // High Tier
            return ShadowVSM2DArray(PointShadowHighTierMoments, SamplerLinearClamp, float3(uv, layer), currentDepth - bias, VSM_MipBias, info.Sharpen);
    }

    // PCF path over 2D array SRV (uses hardware comparison sampler)
    if (bUsePCF != 0)
    {
        if (mapping.Tier == 0) // Low Tier
            return ShadowPCF2DArray(PointShadowLowTier2DArray, SamplerPCF, float3(uv, layer), currentDepth - bias);
        else if (mapping.Tier == 1) // Mid Tier
            return ShadowPCF2DArray(PointShadowMidTier2DArray, SamplerPCF, float3(uv, layer), currentDepth - bias);
        else // High Tier
            return ShadowPCF2DArray(PointShadowHighTier2DArray, SamplerPCF, float3(uv, layer), currentDepth - bias);
    }

    // Default: binary compare from cube SRV
    float sd;
    if (mapping.Tier == 0) // Low Tier
        sd = PointShadowLowTierCubes.SampleLevel(SamplerWrap, float4(dir, cubeIdx), 0).r;
    else if (mapping.Tier == 1) // Mid Tier
        sd = PointShadowMidTierCubes.SampleLevel(SamplerWrap, float4(dir, cubeIdx), 0).r;
    else // High Tier
        sd = PointShadowHighTierCubes.SampleLevel(SamplerWrap, float4(dir, cubeIdx), 0).r;

    return (currentDepth - bias <= sd) ? 1.0f : 0.0f;
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
    float ShadowFactor = CalculateDirectionalShadowFactorWithNormal(Input.WorldPosition, N);
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
    float ShadowFactor = CalculateDirectionalShadowFactorWithNormal(Input.WorldPosition, N);
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
        float pf = CalculatePointShadowFactorIndexed(PointIndex, PointLight, Input.WorldPosition, N);
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
        
        float sf = CalculateSpotShadowFactorIndexed(SpotIndex, Input.WorldPosition, N);
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
