cbuffer PreviewCB : register(b0)
{
    float MinDepth;  // remap input min
    float MaxDepth;  // remap input max
    float Gamma;     // gamma for visibility
    float Invert;    // 0 or 1
}

Texture2D DepthTex : register(t0);
SamplerState LinearClamp : register(s0);

struct VSOut
{
    float4 Pos : SV_Position;
    float2 UV  : TEXCOORD0;
};

VSOut DepthPreviewVS(uint id : SV_VertexID)
{
    // Fullscreen triangle
    float2 pos = float2((id == 2) ? 3.0 : -1.0, (id == 1) ? 3.0 : -1.0);
    float2 uv  = float2((id == 2) ? 2.0 : 0.0, (id == 1) ? 2.0 : 0.0);
    VSOut o;
    o.Pos = float4(pos, 0.0, 1.0);
    // Flip Y to match DirectX texture addressing used elsewhere (shadow sampling flips V)
    o.UV  = float2(uv.x, 1.0 - uv.y);
    return o;
}

float4 DepthPreviewPS(VSOut input) : SV_Target
{
    float d = DepthTex.Sample(LinearClamp, input.UV).r;
    // remap
    float x = saturate((d - MinDepth) / max(1e-6, (MaxDepth - MinDepth)));
    // invert if requested
    x = lerp(x, 1.0 - x, saturate(Invert));
    // gamma
    x = pow(saturate(x), max(1e-3, Gamma));
    return float4(x, 0.0, 0.0, 1.0);
}
