cbuffer ClusterSliceInfo : register(b4)
{
    uint ClusterSliceNumX;
    uint ClusterSliceNumY;
    uint ClusterSliceNumZ;
    uint LightMaxCountPerCluster;
};
StructuredBuffer<int> PointLightIndices : register(t6);
StructuredBuffer<int> SpotLightIndices : register(t7);


struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};



PS_INPUT mainVS(uint vertexID : SV_VertexID)
{
    PS_INPUT output;

	// SV_VertexID를 사용하여 화면을 덮는 큰 삼각형의 클립 공간 좌표를 생성
	// ID가 0일 때: (-1, -1)
	// ID가 1일 때: ( 3, -1)
	// ID가 2일 때: (-1,  3)
    float2 Pos;
    switch (vertexID)
    {
        case 0:
            Pos = float2(-1.f, -1.f);
            break;
        case 1:
            Pos = float2(3.f, -1.f);
            break;
        case 2:
            Pos = float2(-1.f, 3.f);
            break;
        default:
            Pos = float2(0.f, 0.f);
            break;
    }
    output.Position = float4(Pos, 0.0f, 1.0f);
    output.TexCoord = Pos * 0.5f + 0.5f;
    return output;
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    float2 uv = input.TexCoord;
    uint2 ClusterXY = uint2(floor(uv * float2(ClusterSliceNumX, ClusterSliceNumY)));
    uint ClusterIdxNear = ClusterXY.x + ClusterXY.y * ClusterSliceNumX;
    uint LightCount = 0;
    for (int i = 0; i < ClusterSliceNumZ;i++)
    {
        uint CurClusterIdx = ClusterIdxNear + i * ClusterSliceNumX * ClusterSliceNumY;
        for (int j = 0; j < LightMaxCountPerCluster; j++)
        {
            if(PointLightIndices[CurClusterIdx * LightMaxCountPerCluster + j] >= 0)
            {
                LightCount++;
            }
            if (SpotLightIndices[CurClusterIdx * LightMaxCountPerCluster + j] >= 0)
            {
                LightCount++;
            }
        }
    }
    return float4(0, 1, 0, (float) LightCount / (LightMaxCountPerCluster * ClusterSliceNumZ) * 8);
}
