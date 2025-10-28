// GizmoShader.hlsl

cbuffer constants : register(b0)
{
    row_major float4x4 world;
}

cbuffer PerFrame : register(b1)
{
    row_major float4x4 View; // View Matrix Calculation of MVP Matrix
    row_major float4x4 Projection; // Projection Matrix Calculation of MVP Matrix
    float4 CameraPos;
};

cbuffer PerFrame : register(b2)
{
    float4 totalColor;
};

struct VS_INPUT
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float3 viewDir : TEXCOORD0;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    float4 worldPos = mul(input.position, world);
    output.position = mul(worldPos, View);
    output.position = mul(output.position, Projection);

    output.normal = mul(input.normal, (float3x3) world);
    output.viewDir = normalize(CameraPos.xyz - worldPos.xyz);
    output.color = input.color;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float ndotv = saturate(dot(normal, input.viewDir));
    
    float4 finalColor = lerp(input.color, totalColor, totalColor.a);
    
    // Apply a simple rim light effect for better visibility
    float rim = 1.0 - ndotv;
    float4 rimColor = float4(1.0, 1.0, 1.0, 1.0) * pow(rim, 2.0);

    finalColor += rimColor * 0.2;

    return finalColor;
}