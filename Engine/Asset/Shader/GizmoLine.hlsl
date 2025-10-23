

//inputlayout 만들어 놨으나 안쓴다. 주의 GizmoInputLayout

struct FGizmoVertex
{
    float3 Pos;
    float4 Color;
};
cbuffer PerFrame : register(b1)
{
    row_major float4x4 View; // View Matrix Calculation of MVP Matrix
    row_major float4x4 Projection; // Projection Matrix Calculation of MVP Matrix
};


StructuredBuffer<FGizmoVertex> ClusterGizmoVertex : register(t0);


struct PS_INPUT
{
    float4 Position : SV_POSITION; // Transformed position to pass to the pixel shader
    float4 Color : COLOR; // Transformed position to pass to the pixel shader
};

PS_INPUT mainVS(uint id : SV_VertexID)
{
    int Index = id % 24;
    int ClusterIdx = (id / 24) - 1;
    int VertexIdx = (ClusterIdx + 1) * 8;
    if (Index == 0 || Index == 7 || Index == 8)
    {
        VertexIdx += 0;
    }
    else if (Index == 1 || Index == 2 || Index == 10)
    {
        VertexIdx += 1;
    }
    else if (Index == 3 || Index == 4 || Index == 12)
    {
        VertexIdx += 2;
    }
    else if (Index == 5 || Index == 6 || Index == 14)
    {
        VertexIdx += 3;
    }
    else if (Index == 9 || Index == 16 || Index == 23)
    {
        VertexIdx += 4;
    }
    else if (Index == 11 || Index == 17 || Index == 18)
    {
        VertexIdx += 5;
    }
    else if (Index == 13 || Index == 19 || Index == 20)
    {
        VertexIdx += 6;
    }
    else if (Index == 15 || Index == 21 || Index == 22)
    {
        VertexIdx += 7;
    }
        
    //24, 16, 32
    //if (ClusterIdx >= 0)
    //{
    //    int ClusterX = ClusterIdx % 24;
    //    int ClusterY = (ClusterIdx / 24) % 16;
    //    int ClusterZ = ClusterIdx / (24 * 16);
    //    if (ClusterY != 7)
    //    {
    //        VertexIdx = 0;
    //    }
    //}
    
    //if(id > 20000)
    //{
    //    VertexIdx = 0;
    //}
    FGizmoVertex Vertex = ClusterGizmoVertex[VertexIdx];

    PS_INPUT output;
    float4 tmp = float4(Vertex.Pos, 1);
    tmp = mul(tmp, View);
    tmp = mul(tmp, Projection);
	
    output.Position = tmp;
    output.Color = Vertex.Color;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    return input.Color;
}
