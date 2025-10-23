cbuffer constants : register(b0)
{
	row_major float4x4 World;
	row_major float4x4 WorldInverseTranspose;
}

cbuffer PerFrame : register(b1)
{
	row_major float4x4 View;		// View Matrix Calculation of MVP Matrix
	row_major float4x4 Projection;	// Projection Matrix Calculation of MVP Matrix
};

cbuffer DecalConstants : register(b2)
{
	row_major float4x4 DecalWorld;
    row_major float4x4 DecalViewProjection;
	float FadeProgress;
};

Texture2D DecalTexture : register(t0);
SamplerState DecalSampler : register(s0);

Texture2D FadeTexture : register(t1);
SamplerState FadeSampler : register(s1);

struct VS_INPUT
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float4 Color : COLOR;
	float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float4 WorldPos : TEXCOORD0;
	float4 Normal : TEXCOORD1;
	float2 Tex : TEXCOORD2;
};

PS_INPUT mainVS(VS_INPUT Input)
{
	PS_INPUT Output;

	float4 Pos = mul(float4(Input.Position, 1.0f), World);
	Output.Position = mul(mul(Pos, View), Projection);
	Output.WorldPos = Pos;
	Output.Normal = normalize(mul(float4(Input.Normal, 0.0f), WorldInverseTranspose));
	Output.Tex = Input.Tex;
	return Output;
}

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    float2 DecalUV;
	
	
	// Normal Test
	float4 DecalForward = mul(float4(1.0f, 0.0f, 0.0f, 0.0f), DecalWorld);
	if (dot(DecalForward, Input.Normal) > 0.0f) {
		//discard;
	}

	// Decal Local Transition
    float4 DecalLocalPos = mul(Input.WorldPos, DecalViewProjection);
    DecalLocalPos /= DecalLocalPos.w;

	float EPS = 1e-5f;
    if (DecalLocalPos.x < 0.f - EPS ||
    	DecalLocalPos.x > 1.f + EPS ||
    	abs(DecalLocalPos.y) > 1.f + EPS ||
    	abs(DecalLocalPos.z) > 1.f + EPS)
    {
        discard;
    }
	
	//UV Transition ([-0.5~0.5], [-0.5~0.5]) -> ([0~1.0], [1.0~0])
    DecalUV = ((DecalLocalPos.yz) * float2(0.5f, -0.5f) + 0.5f);
    
	float4 DecalColor = DecalTexture.Sample(DecalSampler, DecalUV);

	float FadeValue = FadeTexture.Sample(FadeSampler, DecalUV).r;
	DecalColor.a *= 1.0f - saturate(FadeProgress / (FadeValue + 1e-6));
	
	if (DecalColor.a < 0.001f) { discard; }
	
	return DecalColor;
}