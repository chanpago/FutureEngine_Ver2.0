cbuffer constants : register(b0)
{
	row_major float4x4 world;
}

cbuffer PerFrame : register(b1)
{
	row_major float4x4 View;		// View Matrix Calculation of MVP Matrix
	row_major float4x4 Projection;	// Projection Matrix Calculation of MVP Matrix
};

cbuffer MaterialConstants : register(b2)
{
	float4 Ka;		// Ambient color
	float4 Kd;		// Diffuse color
	float4 Ks;		// Specular color
	float Ns;		// Specular exponent
	float Ni;		// Index of refraction
	float D;		// Dissolve factor
	uint MaterialFlags;	// Which textures are available (bitfield)
	float Time;
};

Texture2D DiffuseTexture : register(t0);	// map_Kd
Texture2D AmbientTexture : register(t1);	// map_Ka
Texture2D SpecularTexture : register(t2);	// map_Ks
Texture2D NormalTexture : register(t3);		// map_Ns
Texture2D AlphaTexture : register(t4);		// map_d
Texture2D BumpTexture : register(t5);		// map_bump

SamplerState SamplerWrap : register(s0);

// Material flags
#define HAS_DIFFUSE_MAP	 (1 << 0)
#define HAS_AMBIENT_MAP	 (1 << 1)
#define HAS_SPECULAR_MAP (1 << 2)
#define HAS_NORMAL_MAP	 (1 << 3)
#define HAS_ALPHA_MAP	 (1 << 4)
#define HAS_BUMP_MAP	 (1 << 5)

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;	// Transformed position to pass to the pixel shader
	float3 normal : TEXCOORD0;
	float2 tex : TEXCOORD1;
};

PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output;

	float4 tmp = float4(input.position, 1.0f);
	tmp = mul(tmp, world);
	tmp = mul(tmp, View);
	tmp = mul(tmp, Projection);
	output.position = tmp;
	output.normal = normalize(mul(float4(input.normal, 0.0f), world).xyz);
	output.tex = input.tex;

	return output;
}

struct PS_OUTPUT
{
	float4 SceneColor : SV_Target0;
	float4 NormalData : SV_Target1;
};

PS_OUTPUT mainPS(PS_INPUT input) : SV_TARGET
{
	PS_OUTPUT output;
	float4 finalColor = float4(0.f, 0.f, 0.f, 1.f);
	float2 UV = input.tex;

	// Base diffuse color
	float4 diffuseColor = Kd;
	if (MaterialFlags & HAS_DIFFUSE_MAP)
	{
		diffuseColor *= DiffuseTexture.Sample(SamplerWrap, UV);
		finalColor.a = diffuseColor.a;
	}

	// Ambient contribution
	float4 ambientColor = Ka;
	if (MaterialFlags & HAS_AMBIENT_MAP)
	{
		ambientColor *= AmbientTexture.Sample(SamplerWrap, UV);
	}

	finalColor.rgb = diffuseColor.rgb + ambientColor.rgb;

	// Alpha handling
	if (MaterialFlags & HAS_ALPHA_MAP)
	{
		float alpha = AlphaTexture.Sample(SamplerWrap, UV).r;
		finalColor.a = D;
		finalColor.a *= alpha;
	}

	output.SceneColor = finalColor;
	float3 encodedNormal = normalize(input.normal) * 0.5f + 0.5f; // [-1,1] → [0,1]
	output.NormalData = float4(encodedNormal, 1.0f);
	
	return output;
}
