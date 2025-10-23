#include "ClusteredRenderingCS.hlsli"

RWStructuredBuffer<FAABB> ClusterAABB : register(u0);

float GetZ(uint ZIdx)
{
    return ZNear * pow((ZFar / ZNear), (float) ZIdx / ClusterSliceNumZ);
}
float3 NDCToView(float3 NDC)
{
    float4 View = mul(float4(NDC, 1), ProjectionInv);
    return View.xyz / View.w;
}

//dir not Normalized
//eye = (0,0,0)
float3 LinearIntersectionToZPlane(float3 dir, float zDis)
{
    if(Orthographic)
    {
        return float3(dir.xy, zDis);
    }
    else
    {
        float t = zDis / dir.z;
        return t * dir;
    }
}

//GroupID : Dispatch 에 넣어주는 쓰레드 그룹의 ID
//GroupThreadID : 쓰레드 그룹 내의 쓰레드 ID 
[numthreads(THREAD_NUM, 1, 1)]
void main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
    uint ThreadIdx = GetThreadIdx(GroupID.x, GroupThreadID.x);
    if (ThreadIdx >= ClusterSliceNumX * ClusterSliceNumY * ClusterSliceNumZ)
    {
        return;
    }
    uint3 ClusterID = GetClusterID(ThreadIdx);
    uint2 ScreenIdx = ClusterID.xy;
    float2 ScreenSlideNumRCP = (float) 1 / uint2(ClusterSliceNumX, ClusterSliceNumY);
    float2 ScreenMin = ScreenIdx * ScreenSlideNumRCP;
    float2 ScreenMax = (ScreenIdx + int2(1, 1)) * ScreenSlideNumRCP;
    float2 NearNDCMin = ScreenMin * 2 - float2(1, 1);
    float2 NearNDCMax = ScreenMax * 2 - float2(1, 1);
    
    //NearPlane In NDC = 0
    float3 ViewMin = NDCToView(float3(NearNDCMin, 0));
    float3 ViewMax = NDCToView(float3(NearNDCMax, 0));
    
    float MinZ = GetZ(ClusterID.z);
    float MaxZ = GetZ(ClusterID.z + 1);
    
    float3 NearViewMin = LinearIntersectionToZPlane(ViewMin, MinZ);
    float3 NearViewMax = LinearIntersectionToZPlane(ViewMax, MinZ);
    float3 FarViewMin = LinearIntersectionToZPlane(ViewMin, MaxZ);
    float3 FarViewMax = LinearIntersectionToZPlane(ViewMax, MaxZ);
    
    float3 AABBMin = min(min(NearViewMin, NearViewMax), min(FarViewMin, FarViewMax));
    float3 AABBMax = max(max(NearViewMin, NearViewMax), max(FarViewMin, FarViewMax));
    
    ClusterAABB[ThreadIdx].Min = AABBMin;
    ClusterAABB[ThreadIdx].Max = AABBMax;
}