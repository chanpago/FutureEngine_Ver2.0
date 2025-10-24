// ShadowMap.hlsl
// Shadow Map 생성을 위한 Vertex/Pixel Shader
// Light의 관점에서 장면을 렌더링하여 깊이 정보를 저장합니다.

// Constant Buffers
cbuffer Model : register(b0)
{
    row_major float4x4 World;
}

cbuffer LightCamera : register(b1)
{
    row_major float4x4 LightView;
    row_major float4x4 LightProjection;
    float3 LightWorldLocation;
    float NearClip;
    float FarClip;
}

// Input/Output Structures
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
    float4 Tangent : TANGENT;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float Depth : TEXCOORD0;  // Linear depth for VSM
};

// Vertex Shader
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // World Space로 변환
    float4 WorldPos = mul(float4(input.Position, 1.0f), World);
    
    // Light View Space로 변환
    float4 LightViewPos = mul(WorldPos, LightView);
    
    // Light Projection Space로 변환
    output.Position = mul(LightViewPos, LightProjection);
    
    // Linear depth 저장 (VSM용)
    output.Depth = output.Position.z / output.Position.w;
    
    return output;
}

// Pixel Shader
// 기본 Depth Shadow Map (나중에 VSM 추가 가능)
void mainPS(PS_INPUT input)
{
    // Depth는 자동으로 Depth Buffer에 기록
    // Pixel Shader에서 특별히 할 일이 없음
    // 나중에 VSM을 구현할 때는 float2(depth, depth^2)를 반환
}
