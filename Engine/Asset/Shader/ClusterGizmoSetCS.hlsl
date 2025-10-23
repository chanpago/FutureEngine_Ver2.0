#include "ClusteredRenderingCS.hlsli"


RWStructuredBuffer<FGizmoVertex> ClusterGizmoVertex : register(u0);
StructuredBuffer<FAABB> ClusterAABB : register(t0);
StructuredBuffer<FPointLightInfo> PointLightInfos : register(t1);
StructuredBuffer<FSpotLightInfo> SpotLightInfos : register(t2);
StructuredBuffer<int> PointLightIndices : register(t3);
StructuredBuffer<int> SpotLightIndices : register(t4);

[numthreads(THREAD_NUM, 1, 1)]
void main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
    uint ThreadIdx = GetThreadIdx(GroupID.x, GroupThreadID.x);
    if (ThreadIdx >= ClusterSliceNumX * ClusterSliceNumY * ClusterSliceNumZ)
    {
        return;
    }
    uint3 ClusterID = GetClusterID(ThreadIdx);
    float4 Color = float4(0, 0, 0, 0);
    uint LightIndicesOffset = ThreadIdx * LightMaxCountPerCluster;
    for (int i = 0; i < LightMaxCountPerCluster;i++)
    {
        uint LightIdx = PointLightIndices[LightIndicesOffset + i];
        if (LightIdx >= 0)
        {
            Color += PointLightInfos[LightIdx].Color;
        }
    }
    for (int i = 0; i < LightMaxCountPerCluster; i++)
    {
        uint LightIdx = SpotLightIndices[LightIndicesOffset + i];
        if (LightIdx >= 0)
        {
            Color += SpotLightInfos[LightIdx].Color;
        }
    }
    
    FAABB CurAABB = ClusterAABB[ThreadIdx];
    float3 ViewMin = CurAABB.Min;
    float3 ViewMax = CurAABB.Max;
    
    float3 ViewPos[8];
    ViewPos[0] = float3(ViewMin.x, ViewMin.y, ViewMin.z);
    ViewPos[1] = float3(ViewMin.x, ViewMax.y, ViewMin.z);
    ViewPos[2] = float3(ViewMax.x, ViewMax.y, ViewMin.z);
    ViewPos[3] = float3(ViewMax.x, ViewMin.y, ViewMin.z);
    ViewPos[4] = float3(ViewMin.x, ViewMin.y, ViewMax.z);
    ViewPos[5] = float3(ViewMin.x, ViewMax.y, ViewMax.z);
    ViewPos[6] = float3(ViewMax.x, ViewMax.y, ViewMax.z);
    ViewPos[7] = float3(ViewMax.x, ViewMin.y, ViewMax.z);

    Color.w = 1;
    uint VertexOffset = 8 * ThreadIdx + 8;
    ClusterGizmoVertex[0 + VertexOffset].Pos = mul(float4(ViewPos[0], 1), ViewInv);
    ClusterGizmoVertex[1 + VertexOffset].Pos = mul(float4(ViewPos[1], 1), ViewInv);
    ClusterGizmoVertex[2 + VertexOffset].Pos = mul(float4(ViewPos[2], 1), ViewInv);
    ClusterGizmoVertex[3 + VertexOffset].Pos = mul(float4(ViewPos[3], 1), ViewInv);
    ClusterGizmoVertex[4 + VertexOffset].Pos = mul(float4(ViewPos[4], 1), ViewInv);
    ClusterGizmoVertex[5 + VertexOffset].Pos = mul(float4(ViewPos[5], 1), ViewInv);
    ClusterGizmoVertex[6 + VertexOffset].Pos = mul(float4(ViewPos[6], 1), ViewInv);
    ClusterGizmoVertex[7 + VertexOffset].Pos = mul(float4(ViewPos[7], 1), ViewInv);
    ClusterGizmoVertex[0 + VertexOffset].Color = Color;
    ClusterGizmoVertex[1 + VertexOffset].Color = Color;
    ClusterGizmoVertex[2 + VertexOffset].Color = Color;
    ClusterGizmoVertex[3 + VertexOffset].Color = Color;
    ClusterGizmoVertex[4 + VertexOffset].Color = Color;
    ClusterGizmoVertex[5 + VertexOffset].Color = Color;
    ClusterGizmoVertex[6 + VertexOffset].Color = Color;
    ClusterGizmoVertex[7 + VertexOffset].Color = Color;

    if (ThreadIdx == 0)
    {
        //SetSpotLightGizmo(0);
        float tanhalffov = tan(fov * 0.5f * 3.141592f / 180.0f);
        float3 ViewFrustumPos[8];
        ViewFrustumPos[0] = float3(-tanhalffov * ZNear * Aspect, -tanhalffov * ZNear, ZNear);
        ViewFrustumPos[1] = float3(tanhalffov * ZNear * Aspect, -tanhalffov * ZNear, ZNear);
        ViewFrustumPos[2] = float3(tanhalffov * ZNear * Aspect, tanhalffov * ZNear, ZNear);
        ViewFrustumPos[3] = float3(-tanhalffov * ZNear * Aspect, tanhalffov * ZNear, ZNear);
        ViewFrustumPos[4] = float3(-tanhalffov * ZFar * Aspect, -tanhalffov * ZFar, ZFar);
        ViewFrustumPos[5] = float3(tanhalffov * ZFar * Aspect, -tanhalffov * ZFar, ZFar);
        ViewFrustumPos[6] = float3(tanhalffov * ZFar * Aspect, tanhalffov * ZFar, ZFar);
        ViewFrustumPos[7] = float3(-tanhalffov * ZFar * Aspect, tanhalffov * ZFar, ZFar);
        for (int i = 0; i < 8; i++)
        {
            ClusterGizmoVertex[i].Pos = mul(float4(ViewFrustumPos[i], 1), ViewInv);
            ClusterGizmoVertex[i].Color = float4(1, 1, 1, 1);
        }
      
    }

}