#pragma once
#include "Render/RenderPass/Public/RenderingContext.h"

class UPipeline;

/**
 * @brief 특정 Primitive Type별로 달라지는 RenderPass를 관리하고 실행하도록 하는 기본 인터페이스
 */
class FRenderPass
{
public:
    FRenderPass(UPipeline* InPipeline, ID3D11Buffer* InConstantBufferCamera, ID3D11Buffer* InConstantBufferModel)
        : Pipeline(InPipeline), ConstantBufferCamera(InConstantBufferCamera), ConstantBufferModel(InConstantBufferModel) {}

    virtual ~FRenderPass() = default;

    /**
     * @brief 프레임마다 실행할 렌더 함수
     * @param Context 프레임 렌더링에 필요한 모든 정보를 담고 있는 객체
     */
    virtual void Execute(FRenderingContext& Context) = 0;

    /**
     * @brief 생성한 객체들 해제
     */
    virtual void Release() = 0;

protected:
    UPipeline* Pipeline;
    ID3D11Buffer* ConstantBufferCamera;
    ID3D11Buffer* ConstantBufferModel;
};
