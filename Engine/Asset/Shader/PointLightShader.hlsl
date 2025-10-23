// ================================================================
// PointLightShader.hlsl
// - Deferred/Hybrid Post Lighting Pass
// - Input : SceneColor, NormalData, Depth
// - Output : LitScene (SceneColor + Point Light contribution)
// ================================================================

// ------------------------------------------------
// Constant Buffers
// ------------------------------------------------
cbuffer PerFrameConstants : register(b0)
{
    row_major float4x4 InvView;        // Inverse of View matrix
    row_major float4x4 InvProjection;  // Inverse of Projection matrix
    float3 CameraPosition;             // For specular calculation
    float  Padding0;
    float4 Viewport;                   // Viewport rectangle (x, y, width, height)
    float2 RenderTargetSize;           // Full size of the render target
};

cbuffer PointLightConstants : register(b1)
{
    float3 LightPosition;      // World-space position of light
    float  LightIntensity;     // Strength (brightness)
    float3 LightColor;         // RGB color
    float  AttenuationRadius;        // Max effective range
    float  DistanceFalloffExponent; // Falloff sharpness
    float3 Padding1;
};

// ------------------------------------------------
// Textures and Sampler
// ------------------------------------------------
Texture2D SceneColorTex : register(t0);
Texture2D NormalTex     : register(t1);
Texture2D DepthTex      : register(t2);

SamplerState LinearSampler : register(s0);

// ------------------------------------------------
// Fullscreen Quad Vertex Shader
// ------------------------------------------------
struct VS_INPUT
{
    float3 Position : POSITION;  // fullscreen quad position (-1~1)
};

struct PS_INPUT
{
    float4 Position : SV_POSITION; // SV_Position provides screen-space coordinates
};

PS_INPUT mainVS(VS_INPUT Input)
{
    PS_INPUT Output;

    // Fullscreen quad input position is already in clip space (-1~1)
    Output.Position = float4(Input.Position, 1.0f);

    return Output;
}

// ------------------------------------------------
// Helper : Reconstruct world-space position from depth
// ------------------------------------------------
float3 ReconstructWorldPosition(float2 ClipPositionXY, float depth)
{
    // depth : [0,1] from DepthTex
    float4 ClipPosition = float4(ClipPositionXY, depth, 1.0f);

    // View-space reconstruction
    float4 ViewPosition = mul(ClipPosition, InvProjection);
    ViewPosition /= ViewPosition.w;

    // World-space conversion
    float4 WorldPosition = mul(ViewPosition, InvView);
    return WorldPosition.xyz;
}

// ------------------------------------------------
// Pixel Shader
// ------------------------------------------------
float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    // Use absolute screen coordinates from SV_Position
    float2 ScreenPosition = Input.Position.xy;

    // Calculate UV coordinates based on full render target size
    float2 UV = ScreenPosition / RenderTargetSize;

    // Read G-Buffer data
    float4 BaseColor = SceneColorTex.Sample(LinearSampler, UV);
    float4 EncodedNormal = NormalTex.Sample(LinearSampler, UV);
    float Depth = DepthTex.Sample(LinearSampler, UV).r;

    // Skip background (no depth)
    if (Depth >= 1.0f)
        return BaseColor;

    // Decode normal
    float3 Normal = normalize(EncodedNormal.xyz * 2.0f - 1.0f);

    // Reconstruct world position using viewport-aware clip space coordinates
    float2 ViewportUV = (ScreenPosition - Viewport.xy) / Viewport.zw;
    float2 ViewportClipPosition = ViewportUV * 2.0 - 1.0;
    ViewportClipPosition.y *= -1.0; // Flip Y for clip space
    float3 WorldPosition = ReconstructWorldPosition(ViewportClipPosition, Depth);

    // Lighting vector
    float3 L = LightPosition - WorldPosition;
    float Distance = length(L);
    L /= Distance;

    // Attenuation
    float Attenuation = saturate(1.0f - pow(Distance / AttenuationRadius, DistanceFalloffExponent));
    Attenuation *= LightIntensity;

    // Diffuse
    float NdotL = saturate(dot(Normal, L));
    float3 Diffuse = LightColor * NdotL * Attenuation;

    // Specular (Blinn-Phong)
    float3 V = normalize(CameraPosition - WorldPosition);
    float3 H = normalize(L + V);
    float Spec = pow(saturate(dot(Normal, H)), 64.0f);
    float3 SpecularColor = float3(1.0f, 0.9f, 0.8f);
    float3 Specular = SpecularColor * Spec * Attenuation * 0.5f;

    // Combine lighting
    float3 FinalLighting = Diffuse + Specular;

    return float4(FinalLighting, 1.0f);
}
