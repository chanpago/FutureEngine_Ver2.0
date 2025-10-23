
#define NUM_POINT_LIGHT 8
#define NUM_SPOT_LIGHT 8
#define ADD_ILLUM(a, b) { (a).Ambient += (b).Ambient; (a).Diffuse += (b).Diffuse; (a).Specular += (b).Specular; }

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

cbuffer GlobalLightConstant : register(b20)
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
};
cbuffer LightCountInfo : register(b21)
{
    uint PointLightCount;
    uint SpotLightCount;
    float2 Padding;
};
cbuffer ClusterSliceInfo : register(b22)
{
    uint ClusterXSliceNum;
    uint ClusterYSliceNum;
    uint ClusterZSliceNum;
    uint LightMaxCountPerCluster;
}

StructuredBuffer<int> LightIndices : register(t20);
StructuredBuffer<FPointLightInfo> PointLightInfos : register(t21);
StructuredBuffer<FSpotLightInfo> SpotLightInfos : register(t22);


//uint GetDepthSliceIdx(float ViewZ)
//{
//    float BottomValue = 1 / log(FarClip / NearClip);
//    ViewZ = clamp(ViewZ, NearClip, FarClip);
//    return uint(floor(log(ViewZ) * ClusterZSliceNum * BottomValue - ClusterZSliceNum * log(NearClip) * BottomValue));
//}

//uint GetLightIndicesOffset(float3 WorldPos)
//{
//    float4 ViewPos = mul(float4(WorldPos, 1), View);
//    float4 NDC = mul(ViewPos, Projection);
//    NDC.xy /= NDC.w;
//    //-1 ~ 1 =>0 ~ ScreenXSlideNum
//    //-1 ~ 1 =>0 ~ ScreenYSlideNum
//    //Near ~ Far => 0 ~ ZSlideNum
//    float2 ScreenNorm = saturate(NDC.xy * 0.5f + 0.5f);
//    uint2 ClusterXY = uint2(floor(ScreenNorm * float2(ClusterXSliceNum, ClusterYSliceNum)));
//    uint ClusterZ = GetDepthSliceIdx(ViewPos.z);
    
//    uint ClusterIdx = ClusterXY.x + ClusterXY.y * ClusterXSliceNum + ClusterXSliceNum * ClusterYSliceNum * ClusterZ;
    
//    return LightMaxCountPerCluster * ClusterIdx;
//}


//uint GetPointLightCount(uint LightIndicesOffset)
//{
//    uint Count = 0;
//    for (int i = 0; i < LightMaxCountPerCluster; i++)
//    {
//        if(LightIndices[LightIndicesOffset + i] >= 0)
//        {
//            Count++;
//        }
//    }
//    return Count;
//}

//FPointLightInfo GetPointLight(uint LightIdx)
//{
//    uint LightInfoIdx = LightIndices[LightIdx];
//    return PointLightInfos[LightInfoIdx];
//}