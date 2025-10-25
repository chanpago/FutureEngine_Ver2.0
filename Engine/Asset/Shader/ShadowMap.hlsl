// ShadowMap.hlsl
// Shadow Map 생성을 위한 Vertex/Pixel Shader
// Light의 관점에서 장면을 렌더링하여 깊이 정보를 저장합니다.

// Constant Buffers
cbuffer Model : register(b0)
{
    row_major float4x4 World;
}

cbuffer PSMConstants  : register(b6)
{
    row_major float4x4 EyeView;        // V_e
    row_major float4x4 EyeProj;        // P_e
    row_major float4x4 LightViewP;     // V_L'
    row_major float4x4 LightProjP;     // P_L'
    float4 ShadowParams;               // x: bias, ...
    uint   bInvertedLight;
    uint   bUsePSM;                    // 0: Simple Ortho, 1: PSM
    uint2  _pad_;
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

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
};

// Vertex Shader (entry: mainVS)
VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT o;
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    
    if (bUsePSM == 1)
    {
        // PSM: World→Camera NDC→Light
        float4 eyeClip = mul(mul(worldPos, EyeView), EyeProj);
        float3 psm = eyeClip.xyz / max(eyeClip.w, 1e-8f);  // PSM 공간(NDC)
        o.Position = mul(mul(float4(psm, 1.0), LightViewP), LightProjP);
    }
    else
    {
        // Simple Ortho: World→Light 직접 변환
        o.Position = mul(mul(worldPos, LightViewP), LightProjP);
    }
    
    return o;
}

// Pixel Shader
// 기본 Depth Shadow Map (나중에 VSM 추가 가능)
void mainPS()
{
    // Depth는 자동으로 Depth Buffer에 기록
    // Pixel Shader에서 특별히 할 일이 없음
    // 나중에 VSM을 구현할 때는 float2(depth, depth^2)를 반환
}
