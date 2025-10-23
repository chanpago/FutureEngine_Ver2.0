# Week7 Team3 (김호민, 신동민, 정찬호, 최진혁) 기술문서

## 1. 개요

본 문서는 KTL 엔진에 구현된 조명(Lighting) 시스템의 기술적 사양과 아키텍처를 설명합니다.

조명 시스템은 씬(Scene) 내의 물체에 현실적인 명암을 부여하여 깊이감과 입체감을 표현하는 핵심 기능입니다. 본 엔진의 조명 시스템은 다양한 셰이딩 모델(Gouraud, Lambert, Blinn-Phong)을 지원하며, Tile-Based Light Culling을 통해 효율적인 다중 광원 처리가 가능합니다.

---
## 2. 핵심 아키텍처

조명 시스템은 아래의 주요 클래스들 간의 상호작용을 통해 동작합니다.

-   **Light Components**: 씬에 배치되는 다양한 광원 타입을 정의합니다.
    -   `UAmbientLightComponent`: 전역 환경광
    -   `UDirectionalLightComponent`: 방향성 광원 (태양광)
    -   `UPointLightComponent`: 점 광원
    -   `USpotLightComponent`: 스포트라이트
    
-   **Light Actors**: Light Component를 포함하는 배치 가능한 액터입니다.
    -   `AAmbientLight`, `ADirectionalLight`, `APointLight`, `ASpotLight`
    
-   **`FLightPass`**: 렌더링 파이프라인의 한 단계로, Tile-Based Light Culling을 수행하고 조명 정보를 GPU에 전달합니다.

-   **`UberLit.hlsl`**: 다양한 셰이딩 모델을 하나의 셰이더에서 처리하는 Uber Shader입니다.

---
## 3. Light Component 및 Actor 구현

### 3.1. Light Component 계층 구조

```
ULightComponentBase (추상 베이스)
  ├─ ULightComponent (공통 기능)
  │   ├─ UAmbientLightComponent
  │   ├─ UDirectionalLightComponent
  │   ├─ UPointLightComponent
  │   │   └─ USpotLightComponent
```

#### 3.1.1. `ULightComponentBase`
- 모든 광원의 공통 속성을 정의하는 추상 베이스 클래스입니다.
- 주요 속성:
  - `Intensity`: 광원의 세기 (0.0 ~ 20.0)
  - `LightColor`: 광원의 색상
  - `bVisible`: 가시성 플래그
  - `bLightEnabled`: 조명 계산 포함 여부

#### 3.1.2. `ULightComponent`
- 시각화를 위한 `UBillboardComponent`를 관리합니다.
- `UpdateVisualizationBillboardTint()`: 광원의 색상과 강도를 빌보드에 반영하여 에디터에서 직관적으로 확인할 수 있습니다.

#### 3.1.3. `UDirectionalLightComponent`
- 무한히 먼 곳에서 일정한 방향으로 빛을 발산하는 광원입니다.
- 태양광 표현에 적합합니다.
- 주요 기능:
  - `GetForwardVector()`: 광원의 방향 벡터 반환
  - `GetDirectionalLightInfo()`: 셰이더 전달용 구조체 반환

#### 3.1.4. `UPointLightComponent`
- 한 점에서 모든 방향으로 빛을 발산하는 광원입니다.
- 주요 속성:
  - `AttenuationRadius`: 광원의 유효 범위
  - `DistanceFalloffExponent`: 거리에 따른 감쇠 지수 (2.0 ~ 16.0)
- 감쇠 공식: `(1 - distance/range)^exponent`

#### 3.1.5. `USpotLightComponent`
- `UPointLightComponent`를 상속받아 원뿔 형태로 빛을 발산합니다.
- 주요 속성:
  - `InnerConeAngle`: 내부 원뿔 각도 (완전한 조명)
  - `OuterConeAngle`: 외부 원뿔 각도 (조명 시작)
  - `AngleFalloffExponent`: 각도에 따른 감쇠 지수

### 3.2. Light Actor

각 Light Component에 대응하는 Actor 클래스가 존재하며, `GetDefaultRootComponent()`를 통해 해당 컴포넌트를 루트로 설정합니다.

```cpp
UClass* APointLight::GetDefaultRootComponent()
{
    return UPointLightComponent::StaticClass();
}
```

---
## 4. 셰이딩 모델 구현 (`UberLit.hlsl`)

### 4.1. Uber Shader 개념

`UberLit.hlsl`은 컴파일 타임 매크로를 사용하여 하나의 셰이더 파일에서 여러 셰이딩 모델을 지원합니다.

#### 셰이딩 모델 매크로:
- `LIGHTING_MODEL_GOURAUD`: Gouraud Shading (정점 셰이더에서 조명 계산)
- `LIGHTING_MODEL_LAMBERT`: Lambert Shading (픽셀 셰이더, Diffuse만)
- `LIGHTING_MODEL_BLINNPHONG`: Blinn-Phong Shading (픽셀 셰이더, Diffuse + Specular)
- `LIGHTING_MODEL_NORMAL`: World Normal 시각화 모드

### 4.2. 법선 변환 (Normal Transformation)

비균일 스케일에서도 올바른 법선 변환을 위해 `(M^-1)^T` 공식을 사용합니다.

```hlsl
float3x3 World3x3 = (float3x3) World;
float3x3 InvTransWorld = transpose(Inverse3x3(World3x3));
Output.WorldNormal = SafeNormalize3(mul(Input.Normal, InvTransWorld));
```

#### Determinant 검사:
스케일이 0인 경우 (행렬식이 0) 메시가 납작해지므로 정점을 discard합니다:

```hlsl
if (abs(GetDeterminant3x3(WorldMatrix3x3)) < 1e-8)
{
    discard;
}
```

### 4.3. 조명 계산

#### 4.3.1. Ambient Light
```hlsl
float4 CalculateAmbientLight(FAmbientLightInfo info)
{
    return info.Color * info.Intensity;
}
```

#### 4.3.2. Directional Light (Blinn-Phong)
```hlsl
FIllumination CalculateDirectionalLight(...)
{
    float3 LightDir = SafeNormalize3(-Info.Direction);
    float NdotL = saturate(dot(WorldNormal, LightDir));
    
    // Diffuse
    Result.Diffuse = Info.Color * Info.Intensity * NdotL;
    
    // Specular (Blinn-Phong)
    float3 H = SafeNormalize3(LightDir + ViewDir);
    float CosTheta = saturate(dot(WorldNormal, H));
    float Spec = ((Ns + 8.0f) / (8.0f * PI)) * pow(CosTheta, Ns);
    Result.Specular = Info.Color * Info.Intensity * Spec;
}
```

정규화 상수 `(Ns + 8) / (8π)`는 에너지 보존을 위한 항입니다.

#### 4.3.3. Point Light
Directional Light와 동일한 계산에 거리 감쇠를 추가합니다:

```hlsl
float Attenuation = pow(saturate(1.0f - Distance / Info.Range), Info.DistanceFalloffExponent);
Result.Diffuse = Info.Color * Info.Intensity * NdotL * Attenuation;
```

#### 4.3.4. Spot Light
Point Light에 각도 감쇠를 추가합니다:

```hlsl
float CosAngle = dot(-LightDir, SpotDir);
float AttenuationAngle = pow(saturate((CosAngle - CosOuter) / (CosInner - CosOuter)), AngleFalloffExponent);
Result.Diffuse = ... * AttenuationDistance * AttenuationAngle;
```

### 4.4. Gouraud vs Per-Pixel Shading

#### Gouraud Shading:
- **Vertex Shader**에서 조명을 계산하고 보간하여 전달합니다.
- 성능은 좋지만 정점이 적으면 품질 저하가 발생합니다.

```hlsl
#if LIGHTING_MODEL_GOURAUD
    // VS에서 모든 조명 계산
    Output.AmbientLight = Illumination.Ambient;
    Output.DiffuseLight = Illumination.Diffuse;
    Output.SpecularLight = Illumination.Specular;
#endif
```

#### Per-Pixel Shading (Lambert, Blinn-Phong):
- **Pixel Shader**에서 조명을 계산합니다.
- 픽셀 단위로 계산하여 높은 품질을 보장합니다.

```hlsl
#elif LIGHTING_MODEL_LAMBERT || LIGHTING_MODEL_BLINNPHONG
    FIllumination Illumination = (FIllumination)0;
    float3 N = ShadedWorldNormal;
    
    Illumination.Ambient = CalculateAmbientLight(Ambient);
    ADD_ILLUM(Illumination, CalculateDirectionalLight(...));
    // Point & Spot lights...
#endif
```

### 4.5. Safe Normalization

HLSL의 `normalize()` 함수는 영벡터 입력 시 NaN을 발생시킬 수 있으므로, 안전한 정규화 함수를 사용합니다:

```hlsl
float3 SafeNormalize3(float3 v)
{
    float Len2 = dot(v, v);
    return Len2 > 1e-12f ? v / sqrt(Len2) : float3(0.0f, 0.0f, 0.0f);
}
```

---
## 5. Normal Mapping

### 5.1. Tangent Space 계산

OBJ 로더에서 각 정점의 Tangent 벡터를 계산합니다 (`ComputeTangents` 함수):

1. **면 단위 Tangent 계산**: 삼각형의 UV 좌표와 위치 벡터로부터 Tangent와 Bitangent를 계산합니다.
   
2. **정점 단위 누적**: 각 정점이 공유하는 모든 면의 Tangent를 누적합니다.

3. **Gram-Schmidt 직교화**: 법선 벡터와 Tangent를 직교화합니다.
   ```cpp
   Tangent = Tangent - Normal * Dot(Normal, Tangent);
   ```

4. **Handedness 계산**: TBN 좌표계의 좌우손성을 결정합니다.
   ```cpp
   float Handedness = (Dot(Cross(Normal, Tangent), Bitangent) < 0.0f) ? -1.0f : 1.0f;
   Vertices[V].Tangent = FVector4(Tangent.X, Tangent.Y, Tangent.Z, Handedness);
   ```

### 5.2. Normal Map 적용 (Pixel Shader)

```hlsl
float3 ComputeNormalMappedWorldNormal(float2 UV, float3 WorldNormal, float4 WorldTangent)
{
    float3 BaseNormal = SafeNormalize3(WorldNormal);
    
    // Tangent가 유효하지 않으면 메시 노말 사용
    if (dot(WorldTangent.xyz, WorldTangent.xyz) <= 1e-8f)
        return BaseNormal;
    
    // 노말 맵 샘플링 [0,1] -> [-1,1]
    float3 TangentSpaceNormal = SafeNormalize3(NormalTexture.Sample(...).xyz * 2.0f - 1.0f);
    
    // TBN 행렬 구성
    float3 T = SafeNormalize3(WorldTangent.xyz);
    float3 B = SafeNormalize3(cross(BaseNormal, T) * WorldTangent.w);
    float3x3 TBN = float3x3(T, B, BaseNormal);
    
    // 탄젠트 공간 -> 월드 공간
    return SafeNormalize3(mul(TangentSpaceNormal, TBN));
}
```

### 5.3. World Normal View Mode

디버깅 목적으로 월드 공간 법선 벡터를 RGB 색상으로 시각화합니다:

```hlsl
#elif LIGHTING_MODEL_NORMAL
    float3 EncodedWorldNormal = ShadedWorldNormal * 0.5f + 0.5f;
    finalPixel.rgb = EncodedWorldNormal;
#endif
```

- `[-1, 1]` 범위의 법선을 `[0, 1]` 범위로 인코딩
- 결과: 빨강(+X), 초록(+Y), 파랑(+Z) 방향으로 표현

---
## 6. Clustered Light Culling

다수의 광원을 효율적으로 처리하기 위해 Clustered Rendering을 구현했습니다.

### 6.1. 개요

화면을 작은 타일(Cluster)로 분할하고, 각 타일과 교차하는 광원만 선별하여 처리함으로써 조명 계산 비용을 크게 절감합니다.

### 6.2. Cluster 구성

#### 6.2.1. 화면 분할
- **X/Y 축**: 화면 공간을 균등하게 분할 (예: 24×16)
- **Z 축**: View 공간의 깊이를 로그 스케일로 분할 (32 슬라이스)

```cpp
uint GetDepthSliceIdx(float ViewZ)
{
    float BottomValue = 1 / log(FarClip / NearClip);
    ViewZ = clamp(ViewZ, NearClip, FarClip);
    return uint(floor(log(ViewZ) * ClusterSliceNumZ * BottomValue 
                      - ClusterSliceNumZ * log(NearClip) * BottomValue));
}
```

로그 스케일 사용 이유:
- 가까운 곳에서는 세밀하게 분할 (높은 정밀도)
- 먼 곳에서는 성글게 분할 (효율성)

#### 6.2.2. Cluster AABB 생성 (`ViewClusterCS.hlsl`)

각 Cluster의 AABB(Axis-Aligned Bounding Box)를 View 공간에서 계산합니다:

```hlsl
[numthreads(THREAD_NUM, 1, 1)]
void main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
    uint ThreadIdx = GetThreadIdx(GroupID.x, GroupThreadID.x);
    uint3 ClusterID = GetClusterID(ThreadIdx);
    
    // NDC 좌표 계산
    float2 NDCMin = float2(ClusterID.xy) / float2(ClusterXSliceNum, ClusterYSliceNum) * 2.0f - 1.0f;
    float2 NDCMax = float2(ClusterID.xy + 1) / float2(ClusterXSliceNum, ClusterYSliceNum) * 2.0f - 1.0f;
    
    // 깊이 범위
    float ZNear = GetSliceDepth(ClusterID.z);
    float ZFar = GetSliceDepth(ClusterID.z + 1);
    
    // 8개 코너를 View 공간으로 변환
    // ... AABB 계산
    
    ClusterAABB[ThreadIdx] = ResultAABB;
}
```

### 6.3. Light Culling (`ClusteredLightCullingCS.hlsl`)

각 Cluster와 교차하는 광원을 찾아 인덱스 버퍼에 기록합니다.

#### 6.3.1. Point Light Culling (구-AABB 교차 검사)

```hlsl
bool IntersectAABBSphere(float3 AABBMin, float3 AABBMax, 
                         float3 SphereCenter, float SphereRadius)
{
    float3 BoxCenter = (AABBMin + AABBMax) * 0.5f;
    float3 BoxHalfSize = (AABBMax - AABBMin) * 0.5f;
    float3 ExtensionSize = BoxHalfSize + float3(SphereRadius, SphereRadius, SphereRadius);
    float3 B2L = SphereCenter - BoxCenter;
    float3 AbsB2L = abs(B2L);
    
    // 1. AABB 확장 테스트
    if (AbsB2L.x > ExtensionSize.x || 
        AbsB2L.y > ExtensionSize.y || 
        AbsB2L.z > ExtensionSize.z)
        return false;
    
    // 2. Over Axis 계산
    int3 OverAxis = int3(AbsB2L.x > BoxHalfSize.x ? 1 : 0, 
                         AbsB2L.y > BoxHalfSize.y ? 1 : 0, 
                         AbsB2L.z > BoxHalfSize.z ? 1 : 0);
    int OverCount = OverAxis.x + OverAxis.y + OverAxis.z;
    
    if (OverCount < 2)
        return true;
    
    // 3. 가장 가까운 AABB 모서리 점과의 거리 검사
    float3 NearBoxPoint = sign(B2L) * OverAxis * BoxHalfSize + BoxCenter;
    NearBoxPoint.x = OverAxis.x == 0 ? SphereCenter.x : NearBoxPoint.x;
    NearBoxPoint.y = OverAxis.y == 0 ? SphereCenter.y : NearBoxPoint.y;
    NearBoxPoint.z = OverAxis.z == 0 ? SphereCenter.z : NearBoxPoint.z;
    
    float3 P2L = SphereCenter - NearBoxPoint;
    return dot(P2L, P2L) < SphereRadius * SphereRadius;
}
```

#### 6.3.2. Spot Light Culling

**옵션 1**: Bounding Sphere로 근사 (빠르지만 부정확)
- Point Light와 동일한 방법 사용

**옵션 2**: 원뿔 AABB (정확하지만 느림)
- 원뿔을 감싸는 AABB를 계산하여 교차 검사
- Rodrigues 회전 공식 사용:

```hlsl
float3 RodriguesRotation(float3 N, float3 V, float Radian, float CosCache, float SinCache)
{
    return dot(V, N) * N * (1 - CosCache) + CosCache * V + cross(N, V) * SinCache;
}
```

#### 6.3.3. Light Index Buffer 작성

```hlsl
int LightIndicesOffset = LightMaxCountPerCluster * ThreadIdx;
uint IncludeLightCount = 0;

for (int i = 0; i < PointLightCount && IncludeLightCount < LightMaxCountPerCluster; i++)
{
    if (IntersectAABBSphere(...))
    {
        PointLightIndices[LightIndicesOffset + IncludeLightCount] = i;
        IncludeLightCount++;
    }
}

// 나머지는 -1로 채움 (끝 마커)
for (uint i = IncludeLightCount; i < LightMaxCountPerCluster; i++)
{
    PointLightIndices[LightIndicesOffset + i] = -1;
}
```

### 6.4. 픽셀 셰이더에서의 활용

각 픽셀에서 자신이 속한 Cluster를 찾고, 해당 Cluster의 광원 인덱스만 순회하며 조명을 계산합니다:

```hlsl
uint GetLightIndicesOffset(float3 WorldPos)
{
    float4 ViewPos = mul(float4(WorldPos, 1), View);
    float4 NDC = mul(ViewPos, Projection);
    NDC.xy /= NDC.w;
    
    float2 ScreenNorm = saturate(NDC.xy * 0.5f + 0.5f);
    uint2 ClusterXY = uint2(floor(ScreenNorm * float2(ClusterSliceNumX, ClusterSliceNumY)));
    uint ClusterZ = GetDepthSliceIdx(ViewPos.z);
    
    uint ClusterIdx = ClusterXY.x + ClusterXY.y * ClusterSliceNumX 
                      + ClusterSliceNumX * ClusterSliceNumY * ClusterZ;
    
    return LightMaxCountPerCluster * ClusterIdx;
}

// 픽셀 셰이더
uint LightIndicesOffset = GetLightIndicesOffset(Input.WorldPosition);
uint PointLightCount = GetPointLightCount(LightIndicesOffset);

for (uint i = 0; i < PointLightCount; i++)
{
    FPointLightInfo PointLight = GetPointLight(LightIndicesOffset + i);
    ADD_ILLUM(Illumination, CalculatePointLight(PointLight, ...));
}
```

### 6.5. 성능 이점

- **기존 방식**: 모든 픽셀에서 모든 광원을 검사 → O(픽셀 수 × 광원 수)
- **Tiled 방식**: 각 픽셀에서 Cluster 내 광원만 검사 → O(픽셀 수 × 평균 Cluster당 광원 수)

예시:
- 광원 100개, 픽셀 1920×1080, Cluster당 평균 광원 5개
- **기존**: 207,360,000 검사
- **Tiled**: 10,368,000 검사 (**약 20배 감소**)

---
## 7. Cluster 시각화

디버깅을 위해 각 Cluster를 와이어프레임으로 시각화하는 기능을 제공합니다.

### 7.1. Cluster Gizmo 생성 (`ClusterGizmoSetCS.hlsl`)

각 Cluster의 8개 코너를 정점으로 하는 와이어프레임을 생성합니다:

```hlsl
[numthreads(THREAD_NUM, 1, 1)]
void main(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
    uint ThreadIdx = GetThreadIdx(GroupID.x, GroupThreadID.x);
    FAABB CurAABB = ClusterAABB[ThreadIdx];
    
    // 8개 코너 계산
    float3 ViewPos[8];
    ViewPos[0] = float3(ViewMin.x, ViewMin.y, ViewMin.z);
    ViewPos[1] = float3(ViewMin.x, ViewMax.y, ViewMin.z);
    // ... 나머지 6개
    
    // Cluster에 포함된 광원 수에 따라 색상 결정
    float4 Color = float4(0, 0, 0, 0);
    for (int i = 0; i < LightMaxCountPerCluster; i++)
    {
        if (PointLightIndices[LightIndicesOffset + i] >= 0)
            Color += PointLightInfos[...].Color;
        if (SpotLightIndices[LightIndicesOffset + i] >= 0)
            Color += SpotLightInfos[...].Color;
    }
    
    // 정점 버퍼에 기록
    for (int i = 0; i < 8; i++)
    {
        ClusterGizmoVertex[VertexOffset + i].Pos = mul(float4(ViewPos[i], 1), ViewInv);
        ClusterGizmoVertex[VertexOffset + i].Color = Color;
    }
}
```

### 7.2. Light Density Heatmap

`ClusteredRenderingGrid.hlsl`은 화면 오버레이로 각 픽셀이 포함된 Cluster의 광원 밀도를 히트맵으로 표시합니다:

```hlsl
float4 mainPS(PS_INPUT input) : SV_Target
{
    float2 uv = input.TexCoord;
    uint2 ClusterXY = uint2(floor(uv * float2(ClusterSliceNumX, ClusterSliceNumY)));
    uint ClusterIdxNear = ClusterXY.x + ClusterXY.y * ClusterSliceNumX;
    
    uint LightCount = 0;
    for (int i = 0; i < ClusterSliceNumZ; i++)
    {
        uint CurClusterIdx = ClusterIdxNear + i * ClusterSliceNumX * ClusterSliceNumY;
        for (int j = 0; j < LightMaxCountPerCluster; j++)
        {
            if (PointLightIndices[CurClusterIdx * LightMaxCountPerCluster + j] >= 0)
                LightCount++;
            if (SpotLightIndices[CurClusterIdx * LightMaxCountPerCluster + j] >= 0)
                LightCount++;
        }
    }
    
    // 초록색으로, 광원이 많을수록 불투명
    return float4(0, 1, 0, (float)LightCount / (LightMaxCountPerCluster * ClusterSliceNumZ) * 8);
}
```

---
## 8. 렌더링 파이프라인 통합

### 8.1. FLightPass

`FLightPass`는 매 프레임 다음 작업을 수행합니다:

1. **옵션 변경 검사**: Cluster 설정이 변경되었는지 확인하고, 변경되었다면 버퍼를 재생성합니다.

2. **광원 정보 수집**: 현재 씬의 모든 광원을 순회하며 GPU 버퍼에 업로드합니다.
   ```cpp
   FRenderResourceFactory::UpdateConstantBufferData(GlobalLightConstantBuffer, GlobalLightData);
   FRenderResourceFactory::UpdateStructuredBuffer(PointLightStructuredBuffer, PointLightDatas);
   FRenderResourceFactory::UpdateStructuredBuffer(SpotLightStructuredBuffer, SpotLightDatas);
   ```

3. **Cluster AABB 계산**: `ViewClusterCS` Compute Shader를 디스패치합니다.
   ```cpp
   Pipeline->SetUnorderedAccessView(0, ClusterAABBRWStructuredBufferUAV);
   Pipeline->DispatchCS(ViewClusterCS, ThreadGroupCount, 1, 1);
   ```

4. **Light Culling**: `ClusteredLightCullingCS` Compute Shader를 디스패치합니다.
   ```cpp
   Pipeline->SetUnorderedAccessView(0, PointLightIndicesRWStructuredBufferUAV);
   Pipeline->SetUnorderedAccessView(1, SpotLightIndicesRWStructuredBufferUAV);
   Pipeline->DispatchCS(ClusteredLightCullingCS, ThreadGroupCount, 1, 1);
   ```

5. **셰이더에 바인딩**: 조명 정보와 인덱스 버퍼를 셰이더 슬롯에 바인딩합니다.
   ```cpp
   Pipeline->SetConstantBuffer(3, EShaderType::VS | EShaderType::PS, GlobalLightConstantBuffer);
   Pipeline->SetShaderResourceView(6, EShaderType::VS | EShaderType::PS, PointLightIndicesRWStructuredBufferSRV);
   Pipeline->SetShaderResourceView(8, EShaderType::VS | EShaderType::PS, PointLightStructuredBufferSRV);
   // ...
   ```

### 8.2. FStaticMeshPass

`FStaticMeshPass`는 메시를 렌더링할 때 View Mode에 따라 적절한 셰이더를 선택합니다:

```cpp
void FStaticMeshPass::Execute(FRenderingContext& Context)
{
    if (Context.ViewMode == EViewModeIndex::VMI_Wireframe)
    {
        RenderState.FillMode = EFillMode::WireFrame;
    }
    else
    {
        VS = Renderer.GetVertexShader(Context.ViewMode);
        PS = Renderer.GetPixelShader(Context.ViewMode);
    }
    
    // ... 메시 렌더링
}
```

View Mode 종류:
- `VMI_Gouraud`: Gouraud Shading
- `VMI_Lambert`: Lambert Shading
- `VMI_BlinnPhong`: Blinn-Phong Shading
- `VMI_WorldNormal`: 법선 시각화
- `VMI_Unlit`: 조명 없음
- `VMI_Wireframe`: 와이어프레임

---
## 9. 에디터 통합

### 9.1. View Mode 전환

메인 메뉴바의 "보기" 메뉴에서 셰이딩 모델을 실시간으로 전환할 수 있습니다:

```cpp
void UMainBarWidget::RenderViewMenu()
{
    if (ImGui::BeginMenu("보기"))
    {
        if (ImGui::MenuItem("고로 셰이딩 적용(Gouraud)", nullptr, bIsGouraud))
        {
            ViewportMgr.SetActiveViewportViewMode(EViewModeIndex::VMI_Gouraud);
        }
        if (ImGui::MenuItem("램버트 셰이딩 적용(Lambert)", nullptr, bIsLambert))
        {
            ViewportMgr.SetActiveViewportViewMode(EViewModeIndex::VMI_Lambert);
        }
        if (ImGui::MenuItem("블린-퐁 셰이딩 적용(Blinn-Phong)", nullptr, bIsBlinnPhong))
        {
            ViewportMgr.SetActiveViewportViewMode(EViewModeIndex::VMI_BlinnPhong);
        }
        if (ImGui::MenuItem("월드 노멀(WorldNormal)", nullptr, bIsWorldNormal))
        {
            ViewportMgr.SetActiveViewportViewMode(EViewModeIndex::VMI_WorldNormal);
        }
        // ...
    }
}
```

### 9.2. 광원 시각화

각 광원은 `UBillboardComponent`를 통해 에디터에서 시각적으로 표시됩니다:
- 광원의 색상과 강도에 따라 빌보드 색상이 자동으로 업데이트됩니다.
- `UpdateVisualizationBillboardTint()` 함수가 이를 담당합니다.

```cpp
void ULightComponent::UpdateVisualizationBillboardTint()
{
    if (!VisualizationBillboard)
        return;
    
    FVector ClampedColor = GetLightColor();
    ClampedColor.X = std::clamp(ClampedColor.X, 0.0f, 1.0f);
    ClampedColor.Y = std::clamp(ClampedColor.Y, 0.0f, 1.0f);
    ClampedColor.Z = std::clamp(ClampedColor.Z, 0.0f, 1.0f);
    
    FVector4 Tint(ClampedColor.X, ClampedColor.Y, ClampedColor.Z, 1.0f);
    VisualizationBillboard->SetSpriteTint(Tint);
}
```

---
## 10. 성능 최적화 기법

### 10.1. NaN 방지

모든 벡터 정규화에서 `SafeNormalize` 함수를 사용하여 NaN 발생을 방지합니다:
- 영벡터 입력 시 영벡터 반환
- `1e-12f` 임계값으로 매우 작은 벡터도 안전하게 처리

### 10.2. 조기 종료 (Early Exit)

조명 계산에서 기여도가 없는 경우 즉시 반환:
```hlsl
if (dot(Info.Direction, Info.Direction) < 1e-12 || 
    dot(WorldNormal, WorldNormal) < 1e-12)
    return Result;
```

### 10.3. 동적 버퍼 크기 조정

광원 수가 증가하면 버퍼를 2배씩 확장하여 재할당 빈도를 줄입니다:

```cpp
if (PointLightBufferCount < PointLightCount)
{
    while (PointLightBufferCount < PointLightCount)
    {
        PointLightBufferCount = PointLightBufferCount << 1;
    }
    SafeRelease(PointLightStructuredBuffer);
    PointLightStructuredBuffer = FRenderResourceFactory::CreateStructuredBuffer<FPointLightInfo>(PointLightBufferCount);
    // ...
}
```

### 10.4. Compute Shader 병렬화

모든 Cluster 계산을 병렬로 수행:
```cpp
int ThreadGroupCount = (ClusterSliceNumX * ClusterSliceNumY * ClusterSliceNumZ + CSNumThread - 1) / CSNumThread;
Pipeline->DispatchCS(ViewClusterCS, ThreadGroupCount, 1, 1);
```

---
## 11. 한계 및 향후 개선 방향

### 11.1. 현재 한계

1. **그림자 미지원**: 광원이 물체에 가려져도 뒤편이 밝게 표현됩니다.
2. **고정 Cluster 크기**: 씬의 복잡도에 따라 동적으로 조정되지 않습니다.
3. **간접광 없음**: 한 번 반사된 빛(Indirect Lighting)은 계산되지 않습니다.

### 11.2. 향후 개선 방향

1. **Shadow Mapping**: 그림자 구현으로 현실감 향상
2. **Adaptive Clustering**: 광원 밀도에 따라 Cluster 크기를 동적 조정
3. **Deferred Rendering**: G-Buffer 기반 렌더링으로 더 많은 광원 지원
4. **PBR (Physically Based Rendering)**: 물리 기반 재질 시스템
5. **Global Illumination**: 간접광 시뮬레이션 (Screen Space GI, Voxel GI 등)
---
## 부록: 주요 상수 및 설정값

| 항목 | 기본값 | 범위 | 설명 |
|------|--------|------|------|
| Light Intensity | 1.0 | 0.0 ~ 20.0 | 광원 세기 (UE 호환) |
| Point Light Range | 10.0 | > 0.0 | 점 광원 유효 범위 |
| Distance Falloff Exponent | 2.0 | 0.0 ~ 16.0 | 거리 감쇠 지수 |
| Spot Inner Cone Angle | 0.0 | 0.0 ~ π/2 | 스포트라이트 내부 각도 |
| Spot Outer Cone Angle | π/4 | 0.0 ~ π/2 | 스포트라이트 외부 각도 |
| Cluster X Slices | 24 | > 0 | 화면 X축 분할 수 |
| Cluster Y Slices | 16 | > 0 | 화면 Y축 분할 수 |
| Cluster Z Slices | 32 | > 0 | 깊이 분할 수 (로그 스케일) |
| Max Lights Per Cluster | 32 | > 0 | Cluster당 최대 광원 수 |
