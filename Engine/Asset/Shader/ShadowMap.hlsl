// ShadowMap.hlsl
// Shadow Map 생성을 위한 Vertex/Pixel Shader
// Light의 관점에서 장면을 렌더링하여 깊이 정보를 저장합니다.

#define MAX_CASCADES 8

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
    
    row_major float4x4 LightViewP[MAX_CASCADES]; // V_L'
    row_major float4x4 LightProjP[MAX_CASCADES]; // P_L'
    row_major float4x4 LightViewPInv[MAX_CASCADES]; // (V'_L)^(-1)
    
    float4 CascadeSplits[MAX_CASCADES / 4];
    uint NumCascades;
    float3 pad0;

    row_major float4x4 CameraClipToLightClip;
    
    float4 ShadowParams;               // // x=a(ShadowBias), y=b(ShadowSlopeBias), z=Sharpen(옵션), w=Reserved
    float4 NotUsedCascadeSplits;
    float3 LightDirWS;                   // 월드공간 "표면→광원" 단위벡터
    uint   bInvertedLight;

    // Ortho 경계와 해상도 (텍셀 길이 계산용)
    float4 LightOrthoParams;             // (l, r, b, t)
    float2 ShadowMapSize;                // (Sx, Sy)
    
    uint bUsePSM; // 0: Simple Ortho, 1: PSM
    uint bUseVSM;
    uint bUsePCF;
    uint bUseCSM;
    float2 pad1;
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
    float Depth : TEXCOORD0;  // Linear depth for VSM
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
    float4 lv      = mul(ndcPos, LightViewP[0]);

    // 라이트-뷰에서 한 텍셀 크기
    float dx_lv, dy_lv;
    ComputeLightViewTexelSize(dx_lv, dy_lv);

    // X쪽 1 texel 이동
    float4 lv_dx   = lv + float4(dx_lv, 0, 0, 0);
    float4 ndc_dx4 = mul(lv_dx, LightViewPInv[0]);
    float3 ndc_dx  = ndc_dx4.xyz / max(1e-6, ndc_dx4.w);
    float4 cam_dx  = float4(ndc_dx * w_cam, w_cam);    // w는 원래 카메라 w 유지
    float4 w_dx    = mul(cam_dx, EyeViewProjInv);
    w_dx          /= max(1e-6, w_dx.w);

    // Y쪽 1 texel 이동
    float4 lv_dy   = lv + float4(0, dy_lv, 0, 0);
    float4 ndc_dy4 = mul(lv_dy, LightViewPInv[0]);
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
        // --- PSM: lightView * warpPSM * spotPerspective * crop ---
        // C++에서 PSMMatrix = lightView * spotPerspective * crop로 계산됨
        // 셀이더에서는 World → PSMMatrix 변환만 수행
        
        // 바이어스 적용: 표면을 광원 방향으로 약간 이동
        float  a = ShadowParams.x;  // ShadowBias
        float3 biasDir = normalize(LightDirWS);  // "표면 → 광원" 방향
        float3 biasedPos = worldPos.xyz + biasDir * a;
        
        // PSM 변환: World → Light View → Spot Perspective → Crop
        // LightViewP[0] = lightView, LightProjP[0] = spotPerspective * crop
        o.Position = mul(mul(float4(biasedPos, 1.0f), LightViewP[0]), LightProjP[0]);
    }
    else
    {
        // Standard SpotLight Shadow: World→Light 직접 변환 (bias 적용)
        o.Position = mul(mul(worldPos, LightViewP[0]), LightProjP[0]);
    }
    
    o.Depth = o.Position.z / o.Position.w;
    return o;
}

// Pixel Shader
// 기본 Depth Shadow Map
#if VSM_ENABLED
float2 mainPS(VS_OUTPUT input) : SV_Target
{
    float Depth = saturate(input.Depth);
    float m1 = Depth;
    float m2 = Depth * Depth + 1e-6f;

    return float2(m1, m2);
}
#else
void mainPS(VS_OUTPUT input)
{
}
#endif