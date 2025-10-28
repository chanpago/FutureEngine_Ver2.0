// GizmoShader.hlsl

cbuffer constants : register(b0)
{
    row_major float4x4 world;
}

cbuffer PerFrame : register(b1)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    float4 CameraPos;
};

// totalColor
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
    float3 worldNormal : NORMAL;
    float4 color : COLOR;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    float4 worldPos = mul(input.position, world);
    output.position = mul(worldPos, View);
    output.position = mul(output.position, Projection);

    output.worldNormal = normalize(mul(input.normal, (float3x3) world));
    output.color = input.color;

    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float3 N = normalize(input.worldNormal);

    // Directional Light
    float3 lightDir = normalize(float3(0.4, 0.6, -1.0));

    // Lambert
    float NdotL = saturate(dot(N, lightDir));
    float diffuse = 0.4 + 0.6 * NdotL;

    // base color
    float4 baseColor = lerp(input.color, totalColor, totalColor.a);

    // diffuse
    float4 finalColor = baseColor * diffuse;

    return finalColor;
}