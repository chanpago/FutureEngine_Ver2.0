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
    float3 lightDir = normalize(float3(0.4f, 0.6f, -0.8f));

    // Lambert
    float NdotL = saturate(dot(N, lightDir));
    
    // base color
    float4 baseColor = lerp(input.color, totalColor, totalColor.a);
    
    // Temporarily hardcode a soft shadow for the directional arrow
    float finalAmbient;
    if (distance(baseColor.rgb, float3(0.95, 0.95, 0.95)) < 0.01)
    {
        finalAmbient = 0.7f;
    }
    else
    {
        finalAmbient = 0.4f;
    }
    
    // diffuse
    float diffuse = finalAmbient + (1.0f - finalAmbient) * NdotL;
    float4 finalColor = baseColor * diffuse;

    return finalColor;
}