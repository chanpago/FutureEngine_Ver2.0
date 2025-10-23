// ============================================================================
// Unreal-like FXAA (HLSL version, using your FXAAParams buffer)
// Based on NVIDIA FXAA 3.11 logic
// CORRECTED VERSION
// ============================================================================

cbuffer FXAAParams : register(b0)
{
    float2 InvResolution;   // 1.0 / resolution (ex: (1/1920, 1/1080))
    float FXAASpanMax;      // Max search span
    float FXAAReduceMul;    // reduce multiplier (ex: 1/8)
    float FXAAReduceMin;    // minimum reduce value (ex: 1/128)
}

Texture2D SceneColor : register(t0);
SamplerState SceneSampler : register(s0);

struct VSInput
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

VSOutput mainVS(VSInput input)
{
    VSOutput output;
    output.Position = float4(input.Position, 0.0f, 1.0f);
    output.TexCoord = input.TexCoord;
    return output;
}

//------------------------------------------------------------------------------
// Utility: luminance calculation
//------------------------------------------------------------------------------
float Luma(float3 color)
{
    /*
     *RGB 각각을 똑같이 보정하면(예: 평균) 색상 변화(채도/색상 회전)에 너무 민감하게 반응해 엣지 감지가 불안정
     *FXAA는 밝기 대비(luminance, 정확히는 감마 공간에서의 Y′ 근사)를 기준으로 “엣지인가?”를 판단하니, 밝기만 대표값으로 뽑아 쓰는 게 좋음
     *중요한 건 “G가 제일 크고, R 다음, B가 제일 작다”는 상대적 비율이 유지되는 것. 사람 눈의 민감도와 일치
     *정리: RGB → 스칼라(Y′)로 압축해 엣지 강도를 안정적으로 본다. 수학적 완벽함보다 속도/안정성이 우선이라 저 상수들이 널리 쓰임
     */
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

//------------------------------------------------------------------------------
// Pixel Shader: Corrected Unreal-style FXAA
//------------------------------------------------------------------------------
float4 mainPS(VSOutput input) : SV_Target
{
    float2 tex = input.TexCoord;
    float2 inv = InvResolution;

    // Fetch 5 samples (center + 4 diagonals)
    // FXAA는 지그재그/사선 엣지에 강하려고 대각 방향의 그라디언트를 
    float3 rgbNW = SceneColor.Sample(SceneSampler, tex + float2(-inv.x, -inv.y)).rgb;
    float3 rgbNE = SceneColor.Sample(SceneSampler, tex + float2( inv.x, -inv.y)).rgb;
    float3 rgbSW = SceneColor.Sample(SceneSampler, tex + float2(-inv.x,  inv.y)).rgb;
    float3 rgbSE = SceneColor.Sample(SceneSampler, tex + float2( inv.x,  inv.y)).rgb;
    float3 rgbM  = SceneColor.Sample(SceneSampler, tex).rgb;

    // Convert to luminance
    float lumaNW = Luma(rgbNW);
    float lumaNE = Luma(rgbNE);
    float lumaSW = Luma(rgbSW);
    float lumaSE = Luma(rgbSE);
    float lumaM  = Luma(rgbM);

    // Compute luminance range in neighborhood
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float lumaRange = lumaMax - lumaMin;

    // Unreal-style FXAA early-exit (skip if low contrast)
    // 주변 대비가 낮으면 엣지가 없다고 판단하고 원본 반환
    float threshold = max(0.0625, lumaMax * 0.125);
    if (lumaRange < threshold)
    {
        return float4(rgbM, 1.0f);
    }

    // Edge direction
    float2 dir;

    /*
    *lumaNW + lumaNE = 윗쪽 평균 밝기 ×2
    *lumaSW + lumaSE = 아랫쪽 평균 밝기 ×2
    *둘의 차이는 “위 vs 아래”의 밝기 차이.
    *
    *lumaNW + lumaSW = 왼쪽 평균 밝기 ×2
    *lumaNE + lumaSE = 오른쪽 평균 밝기 ×2
    *둘의 차이는 “왼 vs 오른”의 밝기 차이.
    */

    // 이 공식은 픽셀의 좌우 보다는 위아래
    // 엘리어싱을 적용할 dir 벡터
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    // Reduce edge direction by local contrast
    // 밝기/대비가 낮거나 노이즈가 많은 곳에서 방향 벡터가 과민하게 커지는 것을 막는 바닥값을 만든다
    // 직관: “엣지 별로 없어 보이면(= 저대비/노이즈), 샘플링 방향의 과도한 증폭을 ‘최소 감쇠 바닥’으로 눌러라.”
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                          (0.25f * FXAAReduceMul), FXAAReduceMin);

    // 엣지 기울기(방향)에 따라 x/y 중 더 작은 쪽을 기준으로 삼는다.
    // 이유: 엣지가 축에 거의 평행하면 한 축 성분이 아주 작아지는데, 그 작은 축을 기준으로 정규화하면
    // **기울기 편향(orientation bias)**을 줄이고, 다양한 기울기에서 비슷한 체감 스텝을 얻을 수 있다
    // 여기에 dirReduce(바닥 감쇠)를 더해 분모가 절대 0으로 가지 않도록 하고, 과민 반응을 막는다.
    float rcpDirMin = 1.0f / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    // *** THIS IS THE CRITICAL FIX ***
    // 앞에서 만든 스케일 팩터로 방향 벡터의 크기를 정규화
    float2 dirScaled = dir * rcpDirMin;

    // ★ 핵심
    // 양방향(±) 으로 **최대 탐색 길이(= pixel 단위)**를 제한
    dirScaled = clamp(dirScaled, -FXAASpanMax, FXAASpanMax);
    dir = dirScaled * InvResolution;
    
    // Sample along the corrected edge direction
    float3 rgbA = 0.5f * (
        SceneColor.Sample(SceneSampler, tex + dir * (1.0 / 3.0 - 0.5)).rgb +
        SceneColor.Sample(SceneSampler, tex + dir * (2.0 / 3.0 - 0.5)).rgb
    );

    float3 rgbB = rgbA * 0.5f + 0.25f * (
        SceneColor.Sample(SceneSampler, tex + dir * -0.5).rgb +
        SceneColor.Sample(SceneSampler, tex + dir * 0.5).rgb
    );

    // Compute luminance of blended sample
    float lumaB = Luma(rgbB);

    /*
     * Use guard band to avoid over-blur
     * rgbB의 휘도가 주변 관측 범위 [lumaMin, lumaMax]를 벗어나면,
     * 경계를 넘어 다른 면/배경을 끌어온 것으로 보고 A로 되돌림 → 후광·디테일 손실 방지.
     */
     
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        rgbB = rgbA;
    }
        
    // Subpixel blending factor
    // subpix가 클수록 더 부드러워지지만 더 뭉개짐
    const float subpix = 0.99f; // Using a slightly less aggressive blend
    float3 finalColor = lerp(rgbM, rgbB, subpix);

    return float4(finalColor, 1.0f);
}
