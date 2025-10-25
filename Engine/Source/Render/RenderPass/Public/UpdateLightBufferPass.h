#pragma once
#include "Render/RenderPass/Public/RenderPass.h"
#include "Render/RenderPass/Public/StaticMeshPass.h"

class UDirectionalLightComponent;
class USpotLightComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * @brief Shadow Map을 베이킹하는 RenderPass
 * Light의 관점에서 장면을 렌더링하여 Shadow Map 텍스처를 생성합니다.
 */
class FUpdateLightBufferPass : public FRenderPass
{
public:
    FUpdateLightBufferPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel,
        ID3D11VertexShader* InVS, ID3D11PixelShader* InPS, ID3D11InputLayout* InLayout);
    virtual ~FUpdateLightBufferPass();

    virtual void Execute(FRenderingContext& Context) override;
    virtual void Release() override;

    /**
     * @brief Shadow Map을 베이킹합니다.
     * Light별로 viewport를 설정하고 장면을 렌더링하여 shadow map을 생성합니다.
     */
    void BakeShadowMap(FRenderingContext& Context);

    /**
     * @brief 특정 메시를 렌더링합니다.
     */
    void RenderPrimitive(UStaticMeshComponent* MeshComp);

    /**
     * @brief Light View/Projection Matrix를 반환합니다.
     */
    const FMatrix& GetLightViewMatrix() const { return LightViewMatrix; }
    const FMatrix& GetLightProjectionMatrix() const { return LightProjectionMatrix; }

    // Hot reload용 setter
    void SetVertexShader(ID3D11VertexShader* InVS) { ShadowMapVS = InVS; }
    void SetPixelShader(ID3D11PixelShader* InPS) { ShadowMapPS = InPS; }
    void SetInputLayout(ID3D11InputLayout* InLayout) { ShadowMapInputLayout = InLayout; }
    
    const FShadowMapConstants& GetCascadedShadowMapConstants() const { return CascadedShadowMapConstants; }

private:
    // Shadow Map Shaders
    ID3D11VertexShader* ShadowMapVS = nullptr;
    ID3D11PixelShader* ShadowMapPS = nullptr;
    ID3D11InputLayout* ShadowMapInputLayout = nullptr;

    // Shadow Map Viewports
    D3D11_VIEWPORT DirectionalShadowViewport;
    D3D11_VIEWPORT SpotShadowViewport;
    D3D11_VIEWPORT PointShadowViewport;

    // Light View/Projection Matrices (StaticMeshPass에서 사용)
    FMatrix LightViewMatrix = FMatrix::Identity();
    FMatrix LightProjectionMatrix = FMatrix::Identity();

    // Light 전용 Camera 상수 버퍼 (ConstantBufferCamera를 덮어쓰지 않기 위해)
    ID3D11Buffer* LightCameraConstantBuffer = nullptr;

    FShadowMapConstants CascadedShadowMapConstants;
};
