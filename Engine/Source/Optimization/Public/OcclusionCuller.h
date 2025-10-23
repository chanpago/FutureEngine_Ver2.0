#pragma once

/**
 * @brief Occlusion Culling 을 담당하는 클래스
 */
class UPrimitiveComponent;

class COcclusionCuller
{
public:
    COcclusionCuller();

    /**
     * @brief 컬링 프로세스를 위한 환경을 초기화하고 View/Projection 행렬을 설정.
     * 매 프레임 컬링을 시작하기 전에 호출되어야 함
     */
    void InitializeCuller(const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix);

     /**
     * @brief 오클루전 컬링의 전체 프로세스를 실행하고 최종 가시 오브젝트 목록을 반환
     * @param AllStaticMeshes 프러스텀 컬링을 통과한 모든 스태틱 메시 목록
     * @param CameraPos 현재 카메라 위치
     * @return 렌더링되어야 할 UPrimitiveComponent 목록
     */
    TArray<UPrimitiveComponent*> PerformCulling(const TArray<UPrimitiveComponent*>& AllStaticMeshes, const FVector& CameraPos);

    // Constants
    static constexpr int Z_BUFFER_WIDTH = 256;
    static constexpr int Z_BUFFER_HEIGHT = 256;
    static constexpr int Z_BUFFER_SIZE = Z_BUFFER_WIDTH * Z_BUFFER_HEIGHT;

private:
    /**
    * @brief 카메라에서 과도하게 가까운 애들 제외
    * @param AllCandidates 가까운 곳의 Occluders 후보
    * @return 오클루더로 선정된 UPrimitiveComponent 목록
    */
    TArray<UPrimitiveComponent*> SelectOccluders(const TArray<UPrimitiveComponent*>& AllCandidates, const FVector& CameraPos);

    void RasterizeOccluders(const TArray<UPrimitiveComponent*>& SelectedOccluders, const FVector& CameraPos);

    /**
     * @brief World Pos > Clip Space > Screen Coordinate
     * @return 스크린 좌표 (X, Y)와 NDC Depth (Z)를 담은 FVector
     */
    FVector Project(const FVector& WorldPos) const;

    /**
     * @brief 해당 메시 컴포넌트가 Z-Buffer에 의해 가려지는지 테스트합니다.
     * @return 바운딩 박스의 코너 중 하나라도 보이면 true를 반환합니다.
     */
    bool IsMeshVisible(const struct FWorldAABBData& AABBData);

    struct BatchProjectionResult BatchProject4(const struct BatchProjectionInput& Input) const;


    /**
     * @brief CPU Z-Buffer에 삼각형을 래스터라이징, 깊이 테스트 수행
     */
    void RasterizeTriangle(const FVector& P1, const FVector& P2, const FVector& P3, TArray<float>& ZBuffer);

    /**
     * @brief PrimitiveComponent의 AABB를 12개의 삼각형 정점으로 변환
     */
    TArray<FVector> ConvertAABBToTriangles(class UPrimitiveComponent* PrimitiveComp);

    TArray<float> CPU_ZBuffer;
    FMatrix CurrentViewProj;

    TArray<struct FWorldAABBData> CachedAABBs;
    TArray<FVector> Triangles;
    TArray<UPrimitiveComponent*> FilteredOccluders;    
    TArray<UPrimitiveComponent*> VisibleMeshComponents;
    uint32 Frame = 0;

    // For SIMD
    alignas(16) float SampleCoords[3][20];     // [0]=X, [1]=Y, [2]=Z
    alignas(16) int32 TempPixelCoords[2][4];   // [0]=X, [1]=Y  
    alignas(16) float TempScreenZ[4];
};

struct FWorldAABBData
{
    UPrimitiveComponent* Prim;
    FVector Min;
    FVector Max;
    FVector Center; // 자주 사용되는 Center 값도 저장하여 계산 오버헤드 제거
};

struct alignas(16) BatchProjectionInput
{
    __m128 WorldX, WorldY, WorldZ; // 4개 점의 X, Y, Z 좌표
};

struct alignas(16) BatchProjectionResult
{
    __m128 ScreenX, ScreenY, ScreenZ; // 4개 점의 스크린 좌표
    __m128i PixelX, PixelY;          // 정수형 픽셀 좌표
    __m128 InBoundsMask;             // 화면 경계 내부 마스크
};
