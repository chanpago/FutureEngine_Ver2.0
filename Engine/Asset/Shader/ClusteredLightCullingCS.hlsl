#include "ClusteredRenderingCS.hlsli"


RWStructuredBuffer<int> PointLightIndices : register(u0);
RWStructuredBuffer<int> SpotLightIndices : register(u1);

StructuredBuffer<FAABB> ClusterAABB : register(t0);
StructuredBuffer<FPointLightInfo> PointLightInfos : register(t1);
StructuredBuffer<FSpotLightInfo> SpotLightInfos : register(t2);


bool IntersectAABBSphere(float3 AABBMin, float3 AABBMax, float3 SphereCenter, float SphereRadius)
{
    float3 BoxCenter = (AABBMin + AABBMax) * 0.5f;
    float3 BoxHalfSize = (AABBMax - AABBMin) * 0.5f;
    float3 ExtensionSize = BoxHalfSize + float3(SphereRadius, SphereRadius, SphereRadius);
    float3 B2L = SphereCenter - BoxCenter; //Box To  Light
    float3 AbsB2L = abs(B2L);
    if(AbsB2L.x > ExtensionSize.x || AbsB2L.y > ExtensionSize.y || AbsB2L.z > ExtensionSize.z)
    {
        return false;
    }
    
    //Over Half Size = 1
    int3 OverAxis = int3(AbsB2L.x > BoxHalfSize.x ? 1 : 0, AbsB2L.y > BoxHalfSize.y ? 1 : 0, AbsB2L.z > BoxHalfSize.z ? 1 : 0);
    int OverCount = OverAxis.x + OverAxis.y + OverAxis.z;
    if(OverCount < 2)
    {
        return true;
    }
    
    float3 SignOverAxis = float3(OverAxis.x * sign(B2L.x), OverAxis.y * sign(B2L.y), OverAxis.z * sign(B2L.z));
    float3 NearBoxPoint = SignOverAxis * BoxHalfSize + BoxCenter;
    NearBoxPoint.x = SignOverAxis.x == 0 ? SphereCenter.x : NearBoxPoint.x;
    NearBoxPoint.y = SignOverAxis.y == 0 ? SphereCenter.y : NearBoxPoint.y;
    NearBoxPoint.z = SignOverAxis.z == 0 ? SphereCenter.z : NearBoxPoint.z;
    
    float3 P2L = SphereCenter - NearBoxPoint; //NearBoxPoint To Light
    float DisSquare = dot(P2L, P2L);
    return DisSquare < SphereRadius * SphereRadius;
}

float3 RodriguesRotation(float3 N, float3 V, float Radian,  float CosCache, float SinCache)
{
    return dot(V, N) * N * (1 - CosCache) + CosCache * V + cross(N, V) * SinCache;
}
bool IntersectAABBAABB(float3 MinA, float3 MaxA, float3 MinB, float3 MaxB)
{
    return (MinA.x <= MaxB.x && MaxA.x >= MinB.x) &&
           (MinA.y <= MaxB.y && MaxA.y >= MinB.y) &&
           (MinA.z <= MaxB.z && MaxA.z >= MinB.z);
}
bool IntersectAABBSpotLight(float3 AABBMin, float3 AABBMax, uint SpotIdx)
{
    FSpotLightInfo SpotLight = SpotLightInfos[SpotIdx];
    float4 LightViewPos = mul(float4(SpotLight.Position, 1), ViewMatrix);
    float3 LightViewDirection = mul(float4(SpotLight.Direction, 0), ViewMatrix);
    float ConeOuterSin = sin(SpotLight.OuterConeAngle);
    float3 SpotLocalAABBMin = float3(-SpotLight.Range * ConeOuterSin, 0, -SpotLight.Range * ConeOuterSin);
    float3 SpotLocalAABBMax = float3(SpotLight.Range * ConeOuterSin, SpotLight.Range, SpotLight.Range * ConeOuterSin);
    float3 Axis = cross(float3(0, 1, 0), LightViewDirection);
    Axis = length(Axis) < 0.0001f ? float3(1, 0, 0) : normalize(Axis);
    float RotAngleRadian = acos(clamp(LightViewDirection.y, -1, 1)); //dot(float3(0,1,0),SpotLight.Direction) = SpotLight.Direction.y
    
    float Sin = sin(RotAngleRadian);
    float Cos = cos(RotAngleRadian);
    
    float3 SpotLocalAABBToWorld[8];
    SpotLocalAABBToWorld[0] = RodriguesRotation(Axis, float3(SpotLocalAABBMin.x, SpotLocalAABBMin.y, SpotLocalAABBMin.z), RotAngleRadian, Cos, Sin);
    SpotLocalAABBToWorld[1] = RodriguesRotation(Axis, float3(SpotLocalAABBMax.x, SpotLocalAABBMin.y, SpotLocalAABBMin.z), RotAngleRadian, Cos, Sin);
    SpotLocalAABBToWorld[2] = RodriguesRotation(Axis, float3(SpotLocalAABBMax.x, SpotLocalAABBMin.y, SpotLocalAABBMax.z), RotAngleRadian, Cos, Sin);
    SpotLocalAABBToWorld[3] = RodriguesRotation(Axis, float3(SpotLocalAABBMin.x, SpotLocalAABBMin.y, SpotLocalAABBMax.z), RotAngleRadian, Cos, Sin);
    SpotLocalAABBToWorld[4] = RodriguesRotation(Axis, float3(SpotLocalAABBMin.x, SpotLocalAABBMax.y, SpotLocalAABBMin.z), RotAngleRadian, Cos, Sin);
    SpotLocalAABBToWorld[5] = RodriguesRotation(Axis, float3(SpotLocalAABBMax.x, SpotLocalAABBMax.y, SpotLocalAABBMin.z), RotAngleRadian, Cos, Sin);
    SpotLocalAABBToWorld[6] = RodriguesRotation(Axis, float3(SpotLocalAABBMax.x, SpotLocalAABBMax.y, SpotLocalAABBMax.z), RotAngleRadian, Cos, Sin);
    SpotLocalAABBToWorld[7] = RodriguesRotation(Axis, float3(SpotLocalAABBMin.x, SpotLocalAABBMax.y, SpotLocalAABBMax.z), RotAngleRadian, Cos, Sin);
    
    float3 SpotWorldAABBMin = SpotLocalAABBToWorld[0];
    float3 SpotWorldAABBMax = SpotLocalAABBToWorld[0];
    for (int i = 1; i < 8;i++)
    {
        SpotWorldAABBMin = float3(min(SpotWorldAABBMin.x, SpotLocalAABBToWorld[i].x), min(SpotWorldAABBMin.y, SpotLocalAABBToWorld[i].y), min(SpotWorldAABBMin.z, SpotLocalAABBToWorld[i].z));
        SpotWorldAABBMax = float3(max(SpotWorldAABBMax.x, SpotLocalAABBToWorld[i].x), max(SpotWorldAABBMax.y, SpotLocalAABBToWorld[i].y), max(SpotWorldAABBMax.z, SpotLocalAABBToWorld[i].z));
    }
    SpotWorldAABBMin += LightViewPos.xyz;
    SpotWorldAABBMax += LightViewPos.xyz;

    return IntersectAABBAABB(AABBMin, AABBMax, SpotWorldAABBMin, SpotWorldAABBMax);
}
[numthreads(THREAD_NUM, 1, 1)]
void main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
    uint ThreadIdx = GetThreadIdx(GroupID.x, GroupThreadID.x);
    if (ThreadIdx >= ClusterSliceNumX * ClusterSliceNumY * ClusterSliceNumZ)
    {
        return;
    }
    uint3 ClusterID = GetClusterID(ThreadIdx);
    FAABB CurAABB = ClusterAABB[ThreadIdx];
    
    int LightIndicesOffset = LightMaxCountPerCluster * ThreadIdx;
    uint IncludeLightCount = 0;
    for (int i = 0; (i < PointLightCount)  && (IncludeLightCount < LightMaxCountPerCluster); i++)
    {
        FPointLightInfo PointLightInfo = PointLightInfos[i];
        float4 LightViewPos = mul(float4(PointLightInfo.Position, 1), ViewMatrix);
        if (IntersectAABBSphere(CurAABB.Min, CurAABB.Max, LightViewPos.xyz, PointLightInfo.Range))
        {
            PointLightIndices[LightIndicesOffset + IncludeLightCount] = i;
            IncludeLightCount++;
        }
    }    
    for (uint i = IncludeLightCount; i < LightMaxCountPerCluster; i++)
    {
        PointLightIndices[LightIndicesOffset + i] = -1;
    }
    IncludeLightCount = 0;
    for (int i = 0; (i < SpotLightCount) && (IncludeLightCount < LightMaxCountPerCluster); i++)
    {
        FSpotLightInfo SpotLightInfo = SpotLightInfos[i];
        if (SpotLightIntersectOption == 1)
        {
            if (IntersectAABBSpotLight(CurAABB.Min, CurAABB.Max, i))
            {
                SpotLightIndices[LightIndicesOffset + IncludeLightCount] = i;
                IncludeLightCount++;
            }
        }
        else
        {
            float4 LightViewPos = mul(float4(SpotLightInfo.Position, 1), ViewMatrix);
            if (IntersectAABBSphere(CurAABB.Min, CurAABB.Max, LightViewPos.xyz, SpotLightInfo.Range))
            {
                SpotLightIndices[LightIndicesOffset + IncludeLightCount] = i;
                IncludeLightCount++;
            }
        }  
    }
    
    for (uint i = IncludeLightCount; i < LightMaxCountPerCluster; i++)
    {
        SpotLightIndices[LightIndicesOffset + i] = -1;
    }
    
    
    //if (ClusterIdx == 0)
    //{
    //    LightIndices[0] = LightIndicesOffset;
    //    LightIndices[1] = IncludeLightCount;
    //    LightIndices[2] = CurAABB.Min.x;
    //    LightIndices[3] = CurAABB.Min.y;
    //    LightIndices[4] = CurAABB.Min.z;
    //    LightIndices[5] = CurAABB.Max.x;
    //    LightIndices[6] = CurAABB.Max.y;
    //    LightIndices[7] = CurAABB.Max.z;
    //    if (PointLightCount > 0)
    //    {
    //        FPointLightInfo PointLightInfo = PointLightInfos[0];
    //        float4 LightViewPos = mul(float4(PointLightInfo.Position, 1), ViewMatrix);
    //        LightIndices[8] = LightViewPos.x;
    //        LightIndices[9] = LightViewPos.y;
    //        LightIndices[10] = LightViewPos.z;
    //        LightIndices[11] = LightViewPos.w;
    //    }

    //}
}