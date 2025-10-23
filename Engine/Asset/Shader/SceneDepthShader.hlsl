// ================================================================
// SceneDepthView.hlsl
// - Renders a linearized depth view for the editor.
// - Input: Depth Texture
// - Output: Grayscale Linear Depth
// ================================================================

// ------------------------------------------------
// Constant Buffers
// ------------------------------------------------
cbuffer PerFrameConstants : register(b0)
{
    float2 RenderTargetSize;                   // Viewport rectangle (x, y, width, height)
    int IsOrthographic;
};

cbuffer Camera : register(b1)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    float3 ViewWorldLocation;    
    float NearClip;
    float FarClip;
};

// ------------------------------------------------
// Textures and Sampler
// ------------------------------------------------
Texture2D DepthTexture : register(t0);
SamplerState PointSampler : register(s0);

// ------------------------------------------------
// Vertex and Pixel Shader I/O
// ------------------------------------------------
struct PS_INPUT
{
    float4 Position : SV_POSITION;
};

// ================================================================
// Vertex Shader
// - Generates a full-screen triangle without needing a vertex buffer.
// ================================================================
PS_INPUT mainVS(uint vertexID : SV_VertexID)
{
    PS_INPUT output;

    // SV_VertexID를 사용하여 화면을 덮는 큰 삼각형의 클립 공간 좌표를 생성
    // ID 0 -> (-1, 1), ID 1 -> (3, 1), ID 2 -> (-1, -3) -- 수정된 좌표계
    // 이 좌표계는 UV가 (0,0)부터 시작하도록 조정합니다.
    float2 pos = float2((vertexID << 1) & 2, vertexID & 2);
    output.Position = float4(pos * 2.0f - 1.0f, 0.0f, 1.0f);
    output.Position.y *= -1.0f;

    return output;
}

// ================================================================
// Pixel Shader
// - Samples non-linear depth and converts it to linear depth.
// ================================================================
float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    float2 ScreenPosition = Input.Position.xy;
    float2 uv = ScreenPosition / RenderTargetSize;
    
    float nonLinearDepth = DepthTexture.Sample(PointSampler, uv).r;
    float linearDepth;

    // IsOrthographic 플래그 값에 따라 분기
    float BandSize = 0.001f;
    if (IsOrthographic > 0)
    {
        // 직교 투영일 경우: 깊이 값은 이미 선형적이므로 그대로 사용
        linearDepth = nonLinearDepth;
    }
    else
    {
        // 원근 투영일 경우: 기존의 선형화 공식을 사용
        float viewSpaceDepth = (FarClip * NearClip) / (nonLinearDepth * (NearClip - FarClip) + FarClip);
        linearDepth = saturate(viewSpaceDepth / FarClip);
        BandSize = 0.02f;
    }
    
    float scaledDepth = linearDepth / BandSize;
    float sawtooth = frac(scaledDepth);
    float bandPattern = 1.0f - sawtooth;
    return float4(bandPattern, bandPattern, bandPattern, 1.0f);
}
