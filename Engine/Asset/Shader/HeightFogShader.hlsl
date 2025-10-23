cbuffer FogConstant : register(b0)
{
	float4 FogColor;
	float FogDensity;
	float FogHeightFalloff;
	float StartDistance;
	float FogCutoffDistance;
	float FogMaxOpacity;
	float FogZ;
}

cbuffer CameraInverse : register(b1)
{
	row_major float4x4 ViewInverse;
	row_major float4x4 ProjectionInverse;
};

cbuffer ViewportInfo : register(b2)
{
	float2 RenderTargetSize;
}

Texture2D DepthTexture : register(t0);
SamplerState DepthSampler : register(s0);

struct PS_INPUT
{
	float4 Position : SV_POSITION;
	float2 NDC : TEXCOORD;
};



PS_INPUT mainVS(uint vertexID : SV_VertexID)
{
	PS_INPUT output;

	// SV_VertexID를 사용하여 화면을 덮는 큰 삼각형의 클립 공간 좌표를 생성
	// ID가 0일 때: (-1, -1)
	// ID가 1일 때: ( 3, -1)
	// ID가 2일 때: (-1,  3)
	switch (vertexID)
	{
		case 0:
			output.NDC = float2(-1.f, -1.f);
			break;
		case 1:
			output.NDC = float2(3.f, -1.f);
			break;
		case 2:
			output.NDC = float2(-1.f, 3.f);
			break;
		default:
			output.NDC = float2(0.f, 0.f);
			break;
	}
	output.Position = float4(output.NDC, 0.0f, 1.0f);
	return output;       
}


float4 mainPS(PS_INPUT Input) : SV_TARGET
{
	//픽셀 좌표 => depth 값 가져오기 위한 UV 좌표 생성
	float2 depthUV = Input.Position.xy / RenderTargetSize;
	float depth = DepthTexture.Sample(DepthSampler, depthUV);

	//NDC 좌표에서 월드 좌표 역산
	float4 clipPos = float4(Input.NDC, depth, 1.f);
	float4 viewPos = mul(clipPos, ProjectionInverse);
	viewPos /= viewPos.w; // 원근 나누기
	float4 worldPos = mul(viewPos, ViewInverse);
	
	//카메라 -> 픽셀 벡터 계산 (뷰 좌표계에서 진행)
	float distanceToPixel = length(viewPos.xyz);
	
	// 안개 농도(Opacity) 계산
	float fogOpacity = 0.0f;
	// 안개는 FogCutoffDistance 안쪽에만 적용
	if (distanceToPixel < FogCutoffDistance)
	{
	    // 1. 높이와 거리를 고려한 기본 지수 안개 계산
	    float heightDensity = FogDensity * exp(-FogHeightFalloff * (worldPos.z - FogZ));
			//높을수록 강도 감소
	    float exponentialFog = 1.0 - exp(-distanceToPixel * heightDensity);
			//멀수록 1에 가까워짐
		
	    // 2. StartDistance까지 선형으로 안개를 보간해주는 페이드인 계수 계산
	    float fadeInFactor = saturate(distanceToPixel / StartDistance);
	
	    // 3. 기본 안개에 페이드인 계수를 곱해 최종 농도 결정
	    fogOpacity = exponentialFog * fadeInFactor;
	}

    //최종 값 보정
    fogOpacity = saturate(fogOpacity) * FogMaxOpacity;

	return float4(FogColor.xyz, fogOpacity);
}