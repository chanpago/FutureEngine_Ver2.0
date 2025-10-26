// ShadowMap.hlsl
// Shadow Map 생성을 위한 Vertex/Pixel Shader
// Light의 관점에서 장면을 렌더링하여 깊이 정보를 저장합니다.

// Constant Buffers
cbuffer Model : register(b0)
{
    row_major float4x4 World;
}

cbuffer PSMConstants  : register(b6)
{
    row_major float4x4 EyeView;        // V_e
    row_major float4x4 EyeProj;        // P_e
    row_major float4x4 EyeViewProjInv; // (P_e * V_e)^(-1)
    
    row_major float4x4 LightViewP;     // V_L'
    row_major float4x4 LightProjP;     // P_L'
    row_major float4x4 LightViewPInv;    // (V'_L)^(-1)
    
    float4 ShadowParams;               // // x=a(ShadowBias), y=b(ShadowSlopeBias), z=Sharpen(옵션), w=Reserved
    float3 LightDirWS;                   // 월드공간 "표면→광원" 단위벡터
    uint   bInvertedLight;

    // Ortho 경계와 해상도 (텍셀 길이 계산용)
    float4 LightOrthoParams;             // (l, r, b, t)
    float2 ShadowMapSize;                // (Sx, Sy)
    
    uint   bUsePSM;                    // 0: Simple Ortho, 1: PSM
    uint   ShadowCasterType;           // 0: Directional, 1: Spot
    uint   SpotShadowCasterIndex;      // index into SpotLightInfos (unused here)
    uint   padA;
    uint   padB0;
    uint   padB1;
}

// Input/Output Structures
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
    float4 Tangent : TANGENT;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
};

//======================================================================
// Helpers
//======================================================================

// NDC 박스에서 1 texel 이동이 라이트-뷰 좌표에서 얼마인지 환산
void ComputeLightViewTexelSize(out float dx_lv, out float dy_lv)
{
    // OrthoRowLH에서 clip.x = 2/(r-l) * lv.x + (l+r)/(l-r)
    // → lv 한 texel = (r-l)/Sx,  lv_y 한 texel = (t-b)/Sy
    float l = LightOrthoParams.x;
    float r = LightOrthoParams.y;
    float b = LightOrthoParams.z;
    float t = LightOrthoParams.w;

    dx_lv = (r - l) / max(1.0, ShadowMapSize.x);
    dy_lv = (t - b) / max(1.0, ShadowMapSize.y);
}

// 월드공간 텍셀 길이 근사(L_texel): 
// world → cam clip → NDC → light-view
// light-view에서 ±1 texel 이동 → (V'_L)^(-1)로 NDC 복귀 → cam clip(w 유지) → (P_e V_e)^(-1)로 월드 복귀 → 길이
float ComputeWorldTexelLength(float4 worldPos, float w_cam)
{
    float4 eyeClip = mul(mul(worldPos, EyeView), EyeProj);
    float3 ndc     = eyeClip.xyz / max(1e-6, w_cam);
    float4 ndcPos  = float4(ndc, 1.0);

    // NDC → light-view
    float4 lv      = mul(ndcPos, LightViewP);

    // 라이트-뷰에서 한 텍셀 크기
    float dx_lv, dy_lv;
    ComputeLightViewTexelSize(dx_lv, dy_lv);

    // X쪽 1 texel 이동
    float4 lv_dx   = lv + float4(dx_lv, 0, 0, 0);
    float4 ndc_dx4 = mul(lv_dx, LightViewPInv);
    float3 ndc_dx  = ndc_dx4.xyz / max(1e-6, ndc_dx4.w);
    float4 cam_dx  = float4(ndc_dx * w_cam, w_cam);    // w는 원래 카메라 w 유지
    float4 w_dx    = mul(cam_dx, EyeViewProjInv);
    w_dx          /= max(1e-6, w_dx.w);

    // Y쪽 1 texel 이동
    float4 lv_dy   = lv + float4(0, dy_lv, 0, 0);
    float4 ndc_dy4 = mul(lv_dy, LightViewPInv);
    float3 ndc_dy  = ndc_dy4.xyz / max(1e-6, ndc_dy4.w);
    float4 cam_dy  = float4(ndc_dy * w_cam, w_cam);
    float4 w_dy    = mul(cam_dy, EyeViewProjInv);
    w_dy          /= max(1e-6, w_dy.w);

    // X/Y 중 더 큰 길이를 사용(보수적)
    float Lx = length(w_dx.xyz - worldPos.xyz);
    float Ly = length(w_dy.xyz - worldPos.xyz);
    return max(Lx, Ly);
}

//======================================================================
// Vertex Shader
//======================================================================
VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT o;
    float4 worldPos = mul(float4(input.Position, 1.0f), World);
    
    if (bUsePSM == 1)
    {
        // --- 월드공간 바이어스 ---
        float  a = ShadowParams.x;                    // ShadowBias
        float  b = ShadowParams.y;                    // ShadowSlopeBias
        float4 camClip_pre = mul(mul(worldPos, EyeView), EyeProj);
        float  w_cam       = max(1e-6, camClip_pre.w);

        // 월드 텍셀 길이 근사
        float L_texel = ComputeWorldTexelLength(worldPos, w_cam);
        float biasDist = a + b * L_texel;

        // 표면→광원 방향으로 전진(PSM 권장)
        float3 PbiasedWS = worldPos.xyz + LightDirWS * biasDist;
        float4 worldBiased = float4(PbiasedWS, 1.0);

        // --- PSM 변환: World → Camera clip → NDC → Light view/proj ---
        float4 eyeClip = mul(mul(worldBiased, EyeView), EyeProj);
        float3 camNDC  = eyeClip.xyz / max(1e-6, eyeClip.w);
        float4 ndcPos  = float4(camNDC, 1.0);

        o.Position = mul(mul(ndcPos, LightViewP), LightProjP);
    }
    else
    {
        // Simple Ortho: World→Light 직접 변환
        o.Position = mul(mul(worldPos, LightViewP), LightProjP);
    }
    
    return o;
}

// Pixel Shader
// 기본 Depth Shadow Map (나중에 VSM 추가 가능)
void mainPS()
{
    // Depth는 자동으로 Depth Buffer에 기록
    // Pixel Shader에서 특별히 할 일이 없음
    // 나중에 VSM을 구현할 때는 float2(depth, depth^2)를 반환
}
