// 상수 버퍼 정의
cbuffer WorldMatrixBuffer : register(b0)
{
	row_major matrix WorldMatrix;
};

cbuffer ViewProjectionBuffer : register(b1)
{
	row_major float4x4 View; // View Matrix Calculation of MVP Matrix
	row_major float4x4 Projection; // Projection Matrix Calculation of MVP Matrix
};

cbuffer FontDataBuffer : register(b2)
{
    float2 AtlasSize;      // 512.0, 512.0
    float2 GlyphSize;      // 16.0, 16.0  
    float2 GridSize;       // 32.0, 32.0
    float2 Padding;
};

// 입력 구조체
struct VSInput
{
	float3 position : POSITION;     // FVector (3 floats)
	float2 texCoord : TEXCOORD0;    // FVector2 (2 floats)
	uint charIndex : TEXCOORD1;     // uint32 문자 인덱스
};

struct PSInput
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	uint charIndex : TEXCOORD1;
};

// Texture and Sampler
Texture2D FontAtlas : register(t0);
SamplerState FontSampler : register(s0);

// Vertex shader
PSInput mainVS(VSInput Input)
{
	PSInput Output;

	// 월드 좌표계로 변환
	float4 worldPos = mul(float4(Input.position, 1.0f), WorldMatrix);
	
	// 뷰-프로젝션 변환
	Output.position = mul(worldPos, View);
	Output.position = mul(Output.position, Projection);
	
	// ASCII 문자를 16x16 그리드로 매핑 (범용적 처리)
	// ASCII 코드를 기반으로 그리드 위치 계산
	uint col = Input.charIndex % 16;  // 열 (0-15)
	uint row = Input.charIndex / 16;  // 행 (0-15)
	float2 gridPos = float2(float(col), float(row));
	
	// 16x16 그리드 셀 크기 계산
	float2 cellSize = float2(1.0f / 16.0f, 1.0f / 16.0f);
	
	// 최종 UV 좌표 계산: 그리드 위치 + 셀 내부 오프셋
	float2 atlasUV = (gridPos * cellSize) + (Input.texCoord * cellSize);
	Output.texCoord = atlasUV;
	
	Output.charIndex = Input.charIndex;

	return Output;
}

// Pixel shader
float4 mainPS(PSInput Input) : SV_TARGET
{
	// 폰트 텍스처에서 색상 샘플링
	float4 AtlasColor = FontAtlas.Sample(FontSampler, Input.texCoord);
	
	// 흰색 글자에 알파 블렌딩 적용
	float4 FinalColor = float4(1.0f, 1.0f, 1.0f, AtlasColor.r);
	
	// 투명한 픽셀은 폐기 (선택사항 - 성능 향상)
	if (FinalColor.a < 0.01f)
		discard;
	
	return FinalColor;
}
