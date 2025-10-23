#define DEGREE_TO_RADIAN (3.141592f / 180.0f)
#define THREAD_NUM 128
struct FPointLightInfo
{
    float4 Color;
    float3 Position;
    float Intensity;
    float Range;
    float DistanceFalloffExponent;
    float2 padding;
};

//StructuredBuffer padding 없어도됨
struct FSpotLightInfo
{
	// Point Light와 공유하는 속성 (필드 순서 맞춤)
    float4 Color;
    float3 Position;
    float Intensity;
    float Range;
    float DistanceFalloffExponent;

	// SpotLight 고유 속성
    float InnerConeAngle;
    float OuterConeAngle;
    float AngleFalloffExponent;
    float3 Direction;
};
struct FGizmoVertex
{
    float3 Pos;
    float4 Color;
};
struct FAABB
{
    float3 Min;
    float3 Max;
};

cbuffer ViewClusterInfo : register(b0)
{
    row_major float4x4 ProjectionInv;
    row_major float4x4 ViewInv;
    row_major float4x4 ViewMatrix;
    float ZNear;
    float ZFar;
    float Aspect;
    float fov;
};
cbuffer ClusterSliceInfo : register(b1)
{
    uint ClusterSliceNumX;
    uint ClusterSliceNumY;
    uint ClusterSliceNumZ;
    uint LightMaxCountPerCluster;
    uint SpotLightIntersectOption;
    uint Orthographic; //0 = Perspective, 1 = Orthographic
    uint2 ClusterSliceInfo_padding;
};
cbuffer LightCountInfo : register(b2)
{
    uint PointLightCount;
    uint SpotLightCount;
    uint2 padding;
}


//Group, Thread 모두 x만 쓴다고 가정
//(GroupY를 쓸려면 GroupSize.x 가 있어야 인덱싱이 가능하다...)
//하지만 실제 데이터의 크기3d공간과 Group이나 Thread 갯수를 매칭 시킨다면 인덱싱 계산을 안해도됨
//Thread와 Group둘다 x만 쓴다고 가정
uint GetThreadIdx(uint GroupX, uint GroupThreadX)
{
    return GroupX * THREAD_NUM + GroupThreadX;
}
uint3 GetClusterID(uint ThreadIdx)
{
    uint3 ClusterID;
    ClusterID.x = ThreadIdx % ClusterSliceNumX;
    ClusterID.y = (ThreadIdx / ClusterSliceNumX) % ClusterSliceNumY;
    ClusterID.z = (ThreadIdx / (ClusterSliceNumX * ClusterSliceNumY));
    return ClusterID;
}